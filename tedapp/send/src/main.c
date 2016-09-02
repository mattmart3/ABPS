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

#include "network.h"
#include "tederror.h"
#include "utils.h"
#include "structs.h"
#include "consts.h"
#include "hashtable.h"


#define USAGE "[OPTIONS] ADDRESS PORT\n\n\
OPTIONS:\n\
    -h, --help                Print this help.\n\
    -6, --ipv6                Use IPv6 (IPv4 default).\n\
                                  Interface argument MUST be specified (-i).\n\
    -b, --bind_iface          Bind the socket to the specified network interface.\n\
                                  Interface argument MUST be specified (-i). \n\
    -i, --iface=[w:]NIC_NAME  Specify the network interface to be used.\n\
                              More interfaces can be specified repeating this option.\n\
                              Adding 'w:' before the interface name,\n\
                              it will be considered as a wlan interface.\n\
    -n, --npkts=N_PKTS        Specify how many packets must be sent.\n\
    -s, --size=PKT_SIZE       Specify how big (bytes) a packet must be.\n"


int epoll_init()
{
	int epollfd;

	epollfd = epoll_create1(0);
	if (epollfd == -1) 
		return -1;

	return epollfd;
}

int epoll_add_socket(int epollfd, struct socket_s *ext_sock)
{
	struct epoll_event ev;
	ev.events = (EPOLLOUT | EPOLLET);
	
	/* Monitor socket errors only for the wifi interface */
	if (ext_sock->iface.type == IFACE_TYPE_WLAN)
		ev.events |= EPOLLERR;

	ev.data.fd = ext_sock->sd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1)
		return -1;
	return 0;
}

void gen_random(char *s, const int len) {
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";

	int i;
	for (i = 0; i < len; ++i) {
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}

	/* Zero terminate the string */
	s[len] = 0;
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
	gen_random(content, conf.msg_length);
	
	return content;
}

/* Send a new message requesting an identifier for TED. Check net_sendmsg(). */
void send_new_msg(hashtable *ht, struct socket_s *ext_sock)
{
	uint32_t identifier;
	struct msg_info_s *info;
	char *buffer;
	int buffer_len, sent_bytes;

	/* Compose a new message and send it */
	buffer = new_msg();
	buffer_len = (int)strlen(buffer);
	if (buffer_len != conf.msg_length)
		exit_err("%s: bad buffer length\n", __func__);

	if (ext_sock->iface.type == IFACE_TYPE_WLAN) {
		sent_bytes = net_send_ted_msg(buffer, buffer_len, &identifier, ext_sock->sd);
		if (sent_bytes < 0) {
			print_err("%s: Sendmsg error (%s).\n",
				  __func__, strerror(errno));
		} else if (sent_bytes != buffer_len) {
			print_err("%s: Lost bytes? msg size: %d, sent bytes %d\n",
				  __func__, buffer_len, sent_bytes); 
		} else {

			printf("----------------------------------WIFI---------------"
			       "----------------\n\t\t\t\t\t%s: sent pkt ID: %d\n\t\t\t\t\tsize: %d\n", 
		       ext_sock->iface.name, identifier, sent_bytes);

			info = (struct msg_info_s *)malloc(sizeof(struct msg_info_s));
			if (info == NULL)
				exit_err("%s: malloc failed", __func__);

			info->size = buffer_len;
			info->id = identifier;
			info->n_frags = 0;
			info->last_frag_received = 0;

			HASH((*ht), identifier, info);
		}
	} else {
		sent_bytes = net_send_msg(buffer, buffer_len, ext_sock->sd);
		if (sent_bytes < 0) {
			print_err("%s: Sendmsg error (%s).\n",
				  __func__, strerror(errno));
		} else if (sent_bytes != buffer_len) {
			print_err("%s: Lost bytes? msg size: %d, sent bytes %d\n",
				  __func__, buffer_len, sent_bytes); 
		} else {
			printf("-----------------------------------------------------"
			       "----------------\n\t\t\t\t\t%s: sent pkt, size: %d\n", 
			       ext_sock->iface.name, sent_bytes);
		}
	}
	free(buffer);
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


void hash_table_remove(struct msg_info_s *msg_origin,
                       hashtable *ht, int key)
                       /*GHashTable *hash_table, gconstpointer key)*/
{
	int i;

	for (i = 0; i < msg_origin->n_frags; i++)
		free(msg_origin->frags[i]);
	

	//g_hash_table_remove(hash_table, key);
	HASH((*ht), key, NULL);

}
/* Receive all the ted notification error that are pending in the errqueue */
void recv_errors(hashtable *ht, struct socket_s *ext_sock)
{
	struct err_msg_s *error_message;

	/* Receive all the notifications in the errqueue.
	 * tederror_recv_nowait returns false if the errqueue is empty. */
	while (net_recv_err(&error_message, ext_sock->sd) > 0) {
		struct ted_info_s *ted_info;
		struct msg_info_s *msg_origin;
		int key, there_is_ted;

		ted_info = (struct ted_info_s *)malloc(sizeof(struct ted_info_s));
		if (ted_info == NULL) 
			exit_err("malloc error in %s\n", __func__);

		there_is_ted = tederror_check_ted_info(error_message, ted_info);

		if (!there_is_ted) {
			printf("Not a ted notification, do nothing\n");
			continue;
		}

		printf("-------------------------------------------------"
		       "--------------------\nTED notified ID: %d \n",
		       ted_info->msg_id);

		printf("msg_id: %d, retry_count: %d, acked: %d\n"
		       "more_frag: %d, frag_len: %d, frag_off: %d\n",
		       ted_info->msg_id, ted_info->retry_count, ted_info->status,
		       ted_info->more_frag, ted_info->frag_length, ted_info->frag_offset);

		//key = g_new0(gint, 1);
		//*key = ted_info->msg_id;
		key = ted_info->msg_id;

		//msg_origin = g_hash_table_lookup(ht, key);
		msg_origin = GET_HASH((*ht), key, NULL);
		if (msg_origin == NULL) {
			exit_err("%s:No key %d in the ht\n", __func__, key);

		} else if (ted_info->more_frag == 0 &&
		           ted_info->frag_offset == 0) {
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

struct socket_s *get_sock_struct(struct socket_s socks[], int n, int sd)
{
	int i;
	for (i = 0; i < n; i++)
		if (socks[i].sd == sd)
			return &(socks[i]);

	return NULL;
}

int main(int argc, char **argv)
{
	char *usage, *address;
	int port;
	int i, idx; 
	int epollfd, nfds;
	int sd;
	struct epoll_event events[MAX_EPOLL_EVENTS];
	struct socket_s ext_socks[MAX_IFACES];
	int n_ext_socks;
	int wlan_sock_id;

	hashtable *hashtb;

	address = NULL;
	port = -1;
	n_ext_socks = 0;
	wlan_sock_id = -1;
	memset(&ext_socks, 0, sizeof(ext_socks) * MAX_IFACES);

	asprintf(&usage, "Usage: %s %s", basename(argv[0]), USAGE);
	
	/* Initialize configuration */
	utils_default_conf();

	/* Arguments validation */
	if (utils_get_opt(argc, argv) == -1 || argc - optind < 2) {

		exit_err("Some required argument is missing.\n\n%s", usage);

	} else if (conf.ip_vers == IPPROTO_IPV6 && conf.nifaces == 0) { 

		exit_err("Interface(s) MUST be specified while using IPv6 "
		         "(see '-i' option).\n\n%s", usage);

	} else if (conf.bind_iface && conf.nifaces == 0) {

		exit_err("The interface(s) which the socket(s) should bind to, "
			 "MUST be specified (see '-i' option).\n\n%s", usage);
	} else {
		address = argv[optind++];
		port = atoi(argv[optind++]);
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
			if (conf.ifaces[i].type == IFACE_TYPE_WLAN) {
				if (wlan_sock_id >= 0) {
					exit_err("%s: Only one wlan iface is allowed.\n",
					         __func__);
				} else {
					wlan_sock_id = i;
				}
			}
				
			sd = net_create_socket(conf.ip_vers, address, port,
					       conf.ifaces[i].name, conf.bind_iface);
			if (sd < 0)
				exit_err("%s: Can't create socket\n", __func__);

			ext_socks[i].sd = sd;
			ext_socks[i].iface = conf.ifaces[i];
			n_ext_socks++;
		}
		if (n_ext_socks != conf.nifaces) 
			exit_err("%s: Sockets count and ifaces count mismatch.\n",
				 __func__);
	} else {
		sd = net_create_socket(conf.ip_vers, address, port, NULL, 0);
		if (sd < 0)
			exit_err("%s: Can't create socket\n", __func__);
		ext_socks[0].sd = sd;
		n_ext_socks = 1;
		
		/* If there is no interface specified, assume the wifi interface
		 * is used */
		printf("I hope you are routing out through a wifi device, "
		       "or this doesn't make any sense.\n\n");

		ext_socks[0].iface.type = IFACE_TYPE_WLAN;
	}

	/* Init epoll for handling events of the shared socket descriptor */
	epollfd = epoll_init();
	if (epollfd == -1)
		exit_err("%s: Init epoll failed on sd", __func__);

	for (i = 0; i < n_ext_socks; i++) {
		if (epoll_add_socket(epollfd, &(ext_socks[i])) < 0) {
			exit_err("%s: Can't register socket %d in epoll\n",
				  __func__, ext_socks[i].sd);
		}

	}
	
	hashtb = (hashtable *)malloc(sizeof(hashtable));
	HASH_INIT((*hashtb), HASH_SIZE_DEFAULT);

	idx = 0;
	/* Main epoll loop. Wait for events on the socket descriptor.
	 * If an EPOLLOUT event is triggered a new message can be sent.
	 * If an EPOLLERR event is triggered some ted error message 
	 * may be present in the errqueue. */
	struct socket_s *ext_sock;
	for (;;) {

		nfds = epoll_wait(epollfd, events, MAX_EPOLL_EVENTS, -1);
		if (nfds == -1)
			exit_err("epoll_wait error\n");
		
		for (i = 0; i < nfds; i++) {

			/* Send a new message if the socket is ready for writing */
			if (events[i].events & EPOLLOUT && idx < conf.n_packets) {
				ext_sock = get_sock_struct(ext_socks, n_ext_socks,
							   events[i].data.fd);
				if (!ext_sock) {
					print_err("%s: EPOLLOUT, unexpected socket id, skipping\n",
						  __func__);
					continue;
				}
				send_new_msg(hashtb, ext_sock);
				idx++;
			}

			/* If there are pending TED error messages 
			 * in the errqueue, receive them and print TED infos.
			 * We expect only the socket corresponding to the wifi 
			 * interface here. */
			if (events[i].events & EPOLLERR) {
				ext_sock = get_sock_struct(ext_socks, n_ext_socks,
							   events[i].data.fd);
				recv_errors(hashtb, ext_sock);
			}
		}
	}

	HASH_FINI((*hashtb));
	free(hashtb);

	return 0;
}
