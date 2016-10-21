/*****************************************************************************/
/* Transmission Error Detector test application                              */
/* This program is a part of the ABPS software suite.                        */
/*                                                                           */
/* Copyright (C) 2015                                                        */
/* Gabriele Di Bernardo, Alessandro Mengoli, Matteo Martelli                 */
/*                                                                           */
/* This program is free software; you can redistribute it and/or             */
/* modify it under the terms of the GNU General Public License               */
/* as published by the Free Software Foundation; either version 2            */
/* of the License, or any later version.                                     */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with this program; if not, write to the Free Software               */
/* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,*/
/* USA.                                                                      */
/*****************************************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>

#include "network.h"
#include "tederror.h"
#include "utils.h"
#include "structs.h"
#include "consts.h"
#include "hashtable.h"


#define USAGE "[OPTIONS] LOCAL_BIND_PORT REMOTE_ADDRESS REMOTE_PORT\n\n\
OPTIONS:\n\
    -h, --help           Print this help.\n\
    -6, --ipv6           Use IPv6 (IPv4 default).\n\
                         Interface argument MUST be specified (-i).\n\
    -b, --bind_iface     Bind the socket to the specified network interface.\n\
                         Interface argument MUST be specified (-i). \n\
    -i, --iface=[t:]dev  Specify the network interface to be used.\n\
                         More interfaces can be specified repeating this option.\n\
                         Adding 't:' before the interface name, TED notifications\n\
                         will be enabled for this device which MUST be a mac80211\n\
                         device and this app MUST run on a TED patched kernel.\n\
    -d, --debug          Enable debug messages.\n\
\n\
HANDOVER DECISION OPTIONS:\n\
    -r, --retry_th       Retry count threshold: the number of retries after a wifi\n\
                         frame must be considered as risky.\n\
    -w, --cwin           Critic check window: the number of packets that defines\n\
                         the size of a check window.\n\
    -R, --ratio_th       The critic ratio (risk pkts / total pkts in cwin) threshold.\n\
                         When this limit is exceeded, the other NIC(s) are enabled.\n\
                         It must be a float value.\n\
    -T, --tolerance      The ratio tolerance. If the ratio (risk pkts / total pkts in cwin)\n\
                         is under this tolerance, the packets are sent only through\n\
                         the TED enabled interface (if any). It must be a float value.\n\
TEST MODE OPTIONS:\n\
    -t, --test           Run in test mode.\n\
    -n, --npkts=N_PKTS   Specify how many packets must be sent (test mode only).\n\
    -s, --size=PKT_SIZE  Specify how big (bytes) a packet must be (test mode only).\n"


int epoll_init()
{
	int epollfd;

	epollfd = epoll_create1(0);
	if (epollfd == -1) 
		return -1;

	return epollfd;
}

int epoll_add_socket(int epollfd, int sd, int iface_type)
{
	struct epoll_event ev;
	ev.events = EPOLLIN;
	
	/* Monitor socket errors only for the wifi interface */
	if (iface_type == IFACE_TYPE_TED)
		ev.events |= EPOLLERR;

	ev.data.fd = sd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1)
		return -1;
	return 0;
}

/* Compose a new simple message of a fixed size. */
char * new_msg(void)
{
	char *content;

	/* Init the message content space */
	content = (char *)malloc(conf.msg_length + 1 * sizeof(char));
	if (content == NULL)
		exit_err("%s: malloc failed", __func__);

	/* Fill the message content */
	utils_gen_random(content, conf.msg_length);
	
	return content;
}



static int compare_frags(const void *frag1, const void *frag2)
{
	struct ted_info_s *a, *b;
	a = *((struct ted_info_s **)frag1);
	b = *((struct ted_info_s **)frag2);
		
	return a->frag_offset - b->frag_offset;
}


int try_recompose(struct msg_info_s *msg_origin)
{

	int i;
	int tot_len;
	float acked_avg, retry_count_avg;

	/* Sort fragments by the fragment offsets */
	qsort(msg_origin->frags, msg_origin->n_frags,
	      sizeof(struct ted_info_s *), compare_frags);
	
	tot_len = 0;
	acked_avg = retry_count_avg = 0.0;
	if (msg_origin->frags[msg_origin->n_frags - 1]->more_frag == 0) {
		for (i = 0; i < msg_origin->n_frags; i++) { 
			tot_len += msg_origin->frags[i]->frag_length;
			acked_avg += (float)msg_origin->frags[i]->status;	
			retry_count_avg += (float)msg_origin->frags[i]->retry_count;
		}

		acked_avg /= (float)msg_origin->n_frags;
		retry_count_avg /= (float)msg_origin->n_frags;

	}
	
	if (tot_len != msg_origin->size + UDP_HEADER_SIZE) {
		printf("recomposition failed.\n");
		return 0;
	}

	
	printf("recomposition succeeded.\n"
	       "acked_avg: %.2f retry_count_avg: %.2f\n",
	       acked_avg, retry_count_avg);

	return 1;

}

struct extsock_s *get_esock(struct extsock_s esocks[], int n, int sd)
{
	int i;
	for (i = 0; i < n; i++)
		if (esocks[i].sd == sd)
			return &(esocks[i]);

	return NULL;
}

int queues_have_space(struct extsock_s esocks[], int n, int direction)
{
	int i;
	for (i = 0; i < n; i++) 
		if (esocks[i].queue_cnt[direction] >= MAX_QUEUE_SIZE)
			return 0;

	return 1;

}

void empty_idle_queues(struct extsock_s esocks[], int n, int ted_dev_only)
{
	int i;
	for (i = 0; i < n; i++) {
		if (esocks[i].iface.type != IFACE_TYPE_TED && ted_dev_only) { 
			esocks[i].queue_cnt[0] = 0;
			esocks[i].queue_cnt[1] = 0;
		}
	}
}

void hash_table_remove(struct msg_info_s *msg_origin, hashtable *ht, int key)
{
	int i;

	for (i = 0; i < msg_origin->n_frags; i++)
		free(msg_origin->frags[i]);
	

	HASH((*ht), key, NULL);

}
/* Receive all the ted notification error that are pending in the errqueue */
void recv_errors(hashtable *ht, struct extsock_s *esock,
		 struct extsock_s *esocks, int n_esocks, int *ted_dev_only)
{
	struct err_msg_s *error_message;

	/* Receive all the notifications in the errqueue.
	 * net_recv_err returns false if the errqueue is empty. */
	while (net_recv_err(&error_message, esock->sd) > 0) {
		struct ted_info_s *ted_info;
		struct msg_info_s *msg_origin;
		int key, there_is_ted;
		uint16_t rtp_id;
		float risk_ratio;

		ted_info = (struct ted_info_s *)malloc(sizeof(struct ted_info_s));
		if (ted_info == NULL) 
			exit_err("malloc error in %s\n", __func__);

		there_is_ted = tederror_check_ted_info(error_message, ted_info);

		if (!there_is_ted) {
			printf("Not a ted notification, do nothing\n");
			continue;
		}

		
		/* We have two counters: drop (which takes also those with high retransmission) 
		 * and sent packets which are considered in a transmission window.
		 * If the risk/safe ratio is above a certain threshold we can 
		 * activate the second network interface */
		if (!ted_info->status || (ted_info->retry_count > conf.retry_th)) {
			esock->pkt_risk++;
		} else {
			esock->pkt_ok++;
		}

		risk_ratio = ((float)esock->pkt_risk)/conf.cwin; 

		if (risk_ratio > (float)conf.ratio_th) {
			printf("Critical risk ratio %f, enabling support device\n",
			      risk_ratio);
			*ted_dev_only = 0;
			esock->pkt_risk = esock->pkt_ok = 0;
		}	

		/* When we reach the end of the window, disable the other device(s) if 
		 * the ted dev is stable (0 risk) and restart the counters. */
		if (esock->pkt_risk + esock->pkt_ok > conf.cwin) {
			if (risk_ratio <= conf.tolerance && !(*ted_dev_only)) {
				int i = 0;
				printf("Ted wifi device should be stable now.\n"
				       "Deactivating support device(s).\n");
				*ted_dev_only = 1;
				/* Empty the queues of the support device(s) */
				for (i = 0; i < n_esocks; i++) {
					if (esock->sd != esocks[i].sd)
						esocks[i].queue_cnt[DIR_INT2EXT] = 0; 
				}
			}
			esock->pkt_risk = esock->pkt_ok = 0;
		}

		key = ted_info->msg_id;

		msg_origin = (struct msg_info_s *)GET_HASH((*ht), key, NULL);
		if (msg_origin == NULL) {
			exit_err("%s:No key %d in the ht\n", __func__, key);

		}
		
		utils_get_rtpid(msg_origin->buf, msg_origin->size, &rtp_id);
		print_dbg("-------------------------------------------------"
		       "--------------------\n"
		       "TED, msg_id: %d, retry_count: %d, acked: %d, rtp_id: %d, risk_ratio: %f\n"
		       "more_frag: %d, frag_len: %d, frag_off: %d\n",
		       ted_info->msg_id, ted_info->retry_count, ted_info->status, rtp_id, risk_ratio,
		       ted_info->more_frag, ted_info->frag_length, ted_info->frag_offset);
		
		if (ted_info->more_frag == 0 && ted_info->frag_offset == 0) {
			/* Not fragmented */
			hash_table_remove(msg_origin, ht, key);
		} else {
			msg_origin->frags[msg_origin->n_frags] = ted_info;
			if (msg_origin->n_frags > MAX_FRAGS)
				/* TODO: increase and reallocate the array */;

			msg_origin->n_frags++;

			/* If this is the last fragment, or if it has already
			 * been received, try the recomposition */
			if (ted_info->more_frag == 0)
				msg_origin->last_frag_received = 1;

			/* If the recomposition works fine, 
			 * remove the entry from the hashtable. */
			if (msg_origin->last_frag_received &&
			    try_recompose(msg_origin))
				hash_table_remove(msg_origin, ht, key);
		}
		free(error_message);
	}	
}

/* Forward a message to the world through the "external" socket. */
void send_to_ext(hashtable *ht, struct extsock_s *esock, int *ted_dev_only)
{
	uint32_t ted_id;
	uint16_t rtp_id;
	struct msg_info_s *info;
	char *buf;
	size_t len;
	ssize_t sent;
	int i, sd, *cnt;
	struct msg_s *queue;

	buf = NULL;
	sd = esock->sd;
	queue = esock->queue[DIR_INT2EXT];
	cnt = &(esock->queue_cnt[DIR_INT2EXT]);
	if (*cnt > MAX_QUEUE_SIZE) {
		exit_err("%s: too many msgs in the queue.\n"
			 "This shouldn't happen.\n", __func__);
	}

	i = 0;
	/* Continue to send until there are no more messages in the queue. */
	while (*cnt > 0) {
		if (conf.test) {
			*cnt = 1;
			buf = new_msg();
			if ((len = strlen(buf)) != (size_t)conf.msg_length)
				exit_err("%s: bad buffer length\n", __func__);
		} else {
			len = queue[i].length;
			buf = queue[i].buffer;
		}

		if (esock->iface.type == IFACE_TYPE_TED) {
			sent = net_send_ted_msg(buf, len, &ted_id, sd);
			if (sent <= 0) {
				/* Enable the support device(s) if this one is down
				 * and empty the queue. */
				/* TODO check this, does it work? */
				if ((errno == ENETDOWN || errno == ENETUNREACH)) {
					*ted_dev_only = 0;
					*cnt = 0;
				}

				break;
			}

			utils_get_rtpid(buf, sent, &rtp_id);
			print_dbg("--------------------------------------------"
				  "-------------------------\n\t\t\t\t\t"
				  "sent_ext from: %s, size: %zd, rtp_id: %d, ted_id: %d, ts: %"PRId64"\n", 
				  esock->iface.name, sent, rtp_id, ted_id, utils_get_timestamp());

			info = (struct msg_info_s *)malloc(sizeof(struct msg_info_s));
			if (info == NULL)
				exit_err("%s: malloc failed", __func__);

			info->size = sent;
			info->id = ted_id;
			info->n_frags = 0;
			info->last_frag_received = 0;

			memcpy((void *)info->buf, (void *)buf, sent); 

			HASH((*ht), (int)ted_id, info);

		} else if (!*ted_dev_only) {
			sent = net_send_msg(buf, len, sd);
			if (sent <= 0) {
				/* Disable this dev if down and empty its queue */
				if ((errno == ENETDOWN || errno == ENETUNREACH)) {
					*ted_dev_only = 1;
					*cnt = 0;
				}
				break;
			}
			utils_get_rtpid(buf, sent, &rtp_id);
			print_dbg("--------------------------------------------"
				  "-------------------------\n\t\t\t\t\t"
				  "sent_ext from: %s, size: %zd, rtp_id: %d, ts: %"PRId64"\n", 
				  esock->iface.name, sent, rtp_id, utils_get_timestamp());
		}
		/* Decrease the queue counter and increase the counter of 
		 * sent message */
		(*cnt)--;
		i++;

	}

	if (sent < 0 && errno != EAGAIN && errno != ENETDOWN && errno != ENETUNREACH) {
		print_err("%s: Sendmsg error (%s).\n", __func__, strerror(errno));
	}
	
	/* If there are still messages to be sent, move them at the top
	 * of the queue. */
	if (*cnt > 0) {
		int j;
		for (j = i; i < j + *cnt; i++) {
			queue[i - j] = queue[i];
		}
	}

	if (conf.test && buf)
		free(buf);
}

void send_to_int(int sd, struct extsock_s *esocks, int n_esocks)
{
	int i, *cnt, sent;
	struct msg_s *queue;
	for (i = 0; i < n_esocks; i++) {

		queue = esocks[i].queue[DIR_EXT2INT];
		cnt = &(esocks[i].queue_cnt[DIR_EXT2INT]);
		if (*cnt > MAX_QUEUE_SIZE) {
			exit_err("%s: too many msgs in the queue.\n"
				 "This shouldn't happen.\n", __func__);
		}

		/* Continue to send until there are no more messages in the queue. */
		for (i = 0, sent = 1; *cnt > 0 && sent > 0; i++, (*cnt)--)
			sent = net_send_msg(queue[i].buffer, queue[i].length, sd);
		
		if (sent < 0 && errno != EAGAIN) {
			print_err("%s: Sendmsg error (%s).\n",
				  __func__, strerror(errno));
		}
	
		/* If there are still messages to be sent, move them at the top
		 * of the queue. */
		if (*cnt > 0) {
			int j;
			for (j = i; i < j + *cnt; i++) {
				queue[i - j] = queue[i];
			}
		}

	}
}

/* Read and push the incoming messages in the backward queue (ext to int).
 * Contintue to read until there are no more messages to read or 
 * we exeed the buffer queue size. */
void recv_from_ext(struct extsock_s *esock)
{
	char *buf;
	int *cnt, sd;
	ssize_t ret = 0;
	struct msg_s *queue;

	sd = esock->sd;
	queue = esock->queue[DIR_EXT2INT];
	cnt = &(esock->queue_cnt[DIR_EXT2INT]);

	do {
		buf = queue[*cnt].buffer;
		ret = net_recv_msg(buf, MAX_BUFF_SIZE, sd);
		if (ret > 0)
			queue[*cnt].length = ret;

	} while (ret > 0 && (++(*cnt)) < (MAX_QUEUE_SIZE - 1));

	if (ret < 0 && errno != EAGAIN)
		print_err("%s: recvfrom failed with error (%s).\n",
			  __func__, strerror(errno));
}

/* Read and push the incoming messages in the forward queue (int to ext)
 * of every external socket. Continue to read until there are no more messages
 * to read or we exeed the buffer queue size of one of the external socket. */
void recv_from_int(int sd, struct extsock_s *esocks, int n_esocks, int ted_dev_only)
{
	char buf[MAX_BUFF_SIZE];
	int i, *cnt, space;
	ssize_t ret = 0;
	struct msg_s *queue;
	
	while ((space = queues_have_space(esocks, n_esocks, DIR_INT2EXT)) &&
	       (ret = net_recv_msg(buf, MAX_BUFF_SIZE, sd)) > 0) {
		
		if (ret > 0) {
			print_dbg("-----------------------------------------------------"
				  "----------------\nlocalhost: recvd pkt, size: %zd\n", 
				  ret);

			for (i = 0; i < n_esocks; i++) {
				if (ted_dev_only && 
				    esocks[i].iface.type != IFACE_TYPE_TED)
					continue;

				queue = esocks[i].queue[DIR_INT2EXT];
				cnt = &(esocks[i].queue_cnt[DIR_INT2EXT]); 
				memcpy(queue[*cnt].buffer, buf, ret);
				queue[*cnt].length = ret;
				(*cnt)++;
			}
		}
	} 
	if (!space) {
		print_dbg("%s: Not all external sockets have space.\n"
			  "Pause reading.\n", __func__);
		empty_idle_queues(esocks, n_esocks, ted_dev_only);
	}

	if (ret < 0 && errno != EAGAIN) {
		print_err("%s: recvfrom failed with error (%s).\n",
			  __func__, strerror(errno));
	}
}

/* Epoll event handler. We have 5 different event cases: 
 * 1 there is some message to be read by the local socket.
 * 2 there is some message we can send to the world by one of the 
 * 	external edge socket.
 * 3 there is some message to be read from the world by one of the
 * 	external edge socket.
 * 4 there is some message we can send back to the local application through
 * 	the local socket.
 * 5 there is some error to be read in any of the socket queue. Only the TED
 * 	error notifications will be taken in count.
 *   */
void epoll_event_handler(int epollfd, struct epoll_event *events, int ev_idx,
			 struct extsock_s *esocks, int n_esocks,
			 int int_sd, hashtable *hashtb, int *ted_dev_only)
{
	/* Send a new message if the socket is ready for writing */
	struct extsock_s *esock;
	struct epoll_event ev;
	if (events[ev_idx].events & EPOLLOUT) {

		if (events[ev_idx].data.fd == int_sd) {
			send_to_int(int_sd, esocks, n_esocks);
			
		} else {
			esock = get_esock(esocks, n_esocks, events[ev_idx].data.fd);

			if (!esock) {
				print_err("%s: EPOLLOUT, unexpected socket id\n"
					  "skipping\n", __func__);
				return;
			}
			send_to_ext(hashtb, esock, ted_dev_only);
		}
		/* Stop this socket to be awakened until
		 * we read something from the external sockets. */
		ev.events = EPOLLIN;
		ev.data.fd = events[ev_idx].data.fd;
		epoll_ctl(epollfd, EPOLL_CTL_MOD, ev.data.fd, &ev);
	}

	if (events[ev_idx].events & EPOLLIN) {
		if (events[ev_idx].data.fd == int_sd) {
			/* Read and enqueue the messages from 
			 * the internal socket. */
			recv_from_int(int_sd, esocks, n_esocks, *ted_dev_only);

			/* Wake all the external sockets when they are
			 * able to send out messages. */
			int i;
			for (i = 0; i < n_esocks; i++) {
				/* Only those sockets with non empty queue. */
				if (esocks[i].queue_cnt[DIR_INT2EXT] > 0) {
					ev.events = EPOLLIN | EPOLLOUT;
					ev.data.fd = esocks[i].sd;
					epoll_ctl(epollfd, EPOLL_CTL_MOD, ev.data.fd, &ev);
				}
			}
		} else {
			esock = get_esock(esock, n_esocks, events[ev_idx].data.fd);
			if (!esock) {
				print_err("%s: EPOLLIN, unexpected socket id\n"
					  "skipping\n", __func__);
				return;
			}

			/* Read and enqueue the messages */
			recv_from_ext(esock);

			/* Wake the internal socket when it's able to send out
			 * messages. */
			ev.events = EPOLLIN | EPOLLOUT;
			ev.data.fd = int_sd;
			epoll_ctl(epollfd, EPOLL_CTL_MOD, ev.data.fd, &ev);
		}
	}

	/* If there are pending TED error messages 
	 * in the errqueue, receive them and print TED infos.
	 * We expect only the socket corresponding to the wifi 
	 * interface here. */
	if (events[ev_idx].events & EPOLLERR) {
		esock = get_esock(esocks, n_esocks, events[ev_idx].data.fd);
		recv_errors(hashtb, esock, esocks, n_esocks, ted_dev_only);
	}
}


int main(int argc, char **argv)
{
	char *usage, *ext_address;
	int int_port, ext_port;
	int i; 
	int epollfd, nfds;
	int sd;
	int int_sd; /* The internal socket file descriptor */
	struct epoll_event events[MAX_EPOLL_EVENTS];
	struct extsock_s esocks[MAX_IFACES];
	int n_esocks;
	int wlan_sock_id;
	int ted_dev_only;

	setvbuf(stdout, NULL, _IONBF, 0);

	hashtable *hashtb;

	ext_address = NULL;
	ext_port = int_port = -1;
	n_esocks = 0;
	wlan_sock_id = -1;
	ted_dev_only = 0;
	memset(esocks, 0, sizeof(struct extsock_s) * MAX_IFACES);

	asprintf(&usage, "Usage: %s %s", basename(argv[0]), USAGE);
	
	/* Initialize configuration */
	utils_default_conf();

	/* Arguments validation */
	if (utils_get_opt(argc, argv) == -1 || argc - optind < 3) {

		exit_err("Some required argument is missing.\n\n%s", usage);

	} else if (conf.ip_vers == IPPROTO_IPV6 && conf.nifaces == 0) { 

		exit_err("Interface(s) MUST be specified while using IPv6 "
			 "(see '-i' option).\n\n%s", usage);

	} else if (conf.bind_iface && conf.nifaces == 0) {

		exit_err("The interface(s) which the socket(s) should bind to, "
			 "MUST be specified (see '-i' option).\n\n%s", usage);
	} else {
		int_port = atoi(argv[optind++]);
		ext_address = argv[optind++];
		ext_port = atoi(argv[optind++]);
	}

	free(usage);

	/* Create and initialize an "external" socket (which communicates with 
	 * the outside world) per each interface. For now we can handle only
	 * one wlan interface. */
	if (conf.nifaces > 0) {
		for (i = 0; i < conf.nifaces ; i++) {
			printf("%s %d %d\n", conf.ifaces[i].name, 
				conf.ifaces[i].name_length, conf.ifaces[i].type);

			/* Check which one is the wifi interface */
			if (conf.ifaces[i].type == IFACE_TYPE_TED) {
				if (wlan_sock_id >= 0) 
					exit_err("%s: Only one wlan ted iface is allowed.\n",
						 __func__);
				wlan_sock_id = i;
				ted_dev_only = 1;
			}
				
			sd = net_create_socket(conf.ip_vers, SOCK_TYPE_EXTERNAL,
					       ext_address, ext_port,
					       conf.ifaces[i].name, conf.bind_iface);
			if (sd < 0)
				exit_err("%s: Can't create socket\n", __func__);

			esocks[i].sd = sd;
			esocks[i].queue_cnt[0] = 0;
			esocks[i].queue_cnt[1] = 0;
			esocks[i].pkt_risk = 0;
			esocks[i].pkt_ok = 0;
			esocks[i].iface = conf.ifaces[i];
			n_esocks++;
		}
		if (n_esocks != conf.nifaces) 
			exit_err("%s: Sockets count and ifaces count mismatch.\n",
				 __func__);
	} else {
		sd = net_create_socket(conf.ip_vers, SOCK_TYPE_EXTERNAL,
				       ext_address, ext_port, NULL, 0);
		if (sd < 0)
			exit_err("%s: Can't create socket\n", __func__);
		esocks[0].sd = sd;
		n_esocks = 1;
		
		/* If there is no interface specified, assume the wifi interface
		 * is used */
		printf("I hope you are routing out through a wifi device, "
		       "or this doesn't make any sense.\n\n");

		esocks[0].iface.type = IFACE_TYPE_TED;
	}

	print_dbg("retry th:%d\ncwin:%d\nratio_th:%f\ntolerance:%f\n",
		  conf.retry_th, conf.cwin, conf.ratio_th, conf.tolerance);
	/* Create the internal socket and bind it to localhost in order to
	 * let it receive the incoming messages from the local application. */
	int_sd = net_create_socket(conf.ip_vers, SOCK_TYPE_INTERNAL, NULL, int_port, NULL, 0);

	/* Init epoll for handling events of the shared socket descriptor */
	epollfd = epoll_init();
	if (epollfd == -1)
		exit_err("%s: Init epoll failed on sd", __func__);

	for (i = 0; i < n_esocks; i++) {
		print_dbg("Registering socket for iface %s\n", esocks[i].iface.name);
		if (epoll_add_socket(epollfd, esocks[i].sd, esocks[i].iface.type) < 0) {
			exit_err("%s: Can't register socket %d in epoll\n",
				  __func__, esocks[i].sd);
		}
	}
	
	epoll_add_socket(epollfd, int_sd, 0); 

	hashtb = (hashtable *)malloc(sizeof(hashtable));
	HASH_INIT((*hashtb), HASH_SIZE_DEFAULT);

	/* Main epoll loop. Wait for events on the socket descriptor.
	 * If an EPOLLOUT event is triggered a new message can be sent.
	 * If an EPOLLERR event is triggered some ted error message 
	 * may be present in the errqueue. */

	for (;;) {

		nfds = epoll_wait(epollfd, events, MAX_EPOLL_EVENTS, -1);
		if (nfds == -1)
			exit_err("epoll_wait error\n");
		
		for (i = 0; i < nfds; i++)
			 epoll_event_handler(epollfd, events, i, 
					     esocks, n_esocks, int_sd, hashtb,
					     &ted_dev_only);
	}

	HASH_FINI((*hashtb));
	free(hashtb);

	return 0;
}
