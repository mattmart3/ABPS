/*****************************************************************************/
/* Transmission Error Detector test application                              */
/* This program is a part of the ABPS software suite.                        */
/*                                                                           */
/* Copyright (C) 2015                                                        */
/* Gabriele Di Bernardo, Matteo Martelli                                     */
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
#include <glib.h>
#include <unistd.h>

#include "network.h"
#include "tederror.h"
#include "utils.h"
#include "structs.h"
#include "consts.h"


#define USAGE "[OPTIONS] ADDRESS PORT\n\n\
OPTIONS:\n\
    -h, --help                Print this help.\n\
    -6, --ipv6                Use IPv6 (IPv4 default).\n\
                                  Interface argument MUST be specified (-i).\n\
    -i, --iface=NIC_NAME      Declare the network interface associated\n\
                                  to the specified ip address.\n\
    -n, --npkts=N_PKTS        Specify how many packets must be sent.\n\
    -s, --size=PKT_SIZE       Specify how big (bytes) a packet must be.\n"


int epoll_init(int sd)
{
	int epollfd;
	struct epoll_event ev;

	epollfd = epoll_create1(0);
	if (epollfd == -1) 
		return -1;

	ev.events = (EPOLLOUT | EPOLLERR | EPOLLET);
	ev.data.fd = net_get_shared_descriptor(); 
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1)
		return -1;

	return epollfd;
}

void gen_random(char *s, const int len) {
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";

	for (int i = 0; i < len; ++i) {
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}

	s[len] = 0;
}

/* Compose a new simple message of a fixed size. */
char * new_msg(void)
{
	char *content;

	/* Init the message content space */
	content = (char *)malloc(conf.msg_length * sizeof(char));
	if (content == NULL)
		utils_exit_error("%s: malloc failed", __func__);

	/* Fill the message content */
	gen_random(content, conf.msg_length - 1);
	
	
	return content;
}

/* Send a new message requesting an identifier for TED. Check net_sendmsg(). */
void send_new_msg(GHashTable *ht)
{
	uint32_t identifier;
	gint *hash_key;
	struct msg_info_s *info;
	char *buffer;

	/* Compose a new message and sent it */
	buffer = new_msg();
	net_sendmsg(buffer, strlen(buffer), &identifier);
	printf("-----------------------------------------------------"
	       "----------------\n\t\t\t\t\tSent pkt ID: %d\n\t\t\t\t\tsize: %ld\n", 
	       identifier, strlen(buffer));
	//printf("%s\n", buffer);

	hash_key = g_new0(gint, 1);
	*hash_key = identifier;

	info = g_new0(struct msg_info_s, 1);
	info->size = strlen(buffer);
	info->id = identifier;
	info->n_frags = 0;
	info->last_frag_received = 0;

	g_hash_table_insert(ht, hash_key, info);

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
	
	/* TODO: check the payload hash too. */
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
                       GHashTable *hash_table, gconstpointer key)
{
	int i;

	for (i = 0; i < msg_origin->n_frags; i++)
		free(msg_origin->frags[i]);
	

	g_hash_table_remove(hash_table, key);

}
/* Receive all the ted notification error that are pending in the errqueue */
void recv_ted_errors(GHashTable *ht)
{
	struct err_msg_s *error_message;

	/* Receive all the notifications in the errqueue.
	 * tederror_recv_nowait returns false if the errqueue is empty. */
	while (tederror_recv_nowait(&error_message) > 0) {
		struct ted_info_s *ted_info;
		struct msg_info_s *msg_origin;
		gint *key;

		ted_info = (struct ted_info_s *)malloc(sizeof(struct ted_info_s));
		if (ted_info == NULL) 
			utils_exit_error("malloc error in %s\n", __func__);

		/* Application does not use local notification info. */
		tederror_check_ted_info(error_message, ted_info);

		printf("-------------------------------------------------"
		       "--------------------\nTED notified ID: %d \n",
		       ted_info->msg_id);

		printf("msg_id: %d, retry_count: %d, acked: %d\n"
		       "more_frag: %d, frag_len: %d, frag_off: %d\n",
		       ted_info->msg_id, ted_info->retry_count, ted_info->status,
		       ted_info->more_frag, ted_info->frag_length, ted_info->frag_offset);

		//printf("XXX %s\n", ted_info->msg_pload);
		key = g_new0(gint, 1);
		*key = ted_info->msg_id;

		msg_origin = g_hash_table_lookup(ht, key);
		if (msg_origin == NULL) {
			utils_print_error("No key %d in the ht\n", key);

		} else if (ted_info->more_frag == 0 &&
		           ted_info->frag_offset == 0) {
			/* Not fragmented */
			hash_table_remove(msg_origin, ht, key);

		} else {
			msg_origin->frags[msg_origin->n_frags] = ted_info;
			if (msg_origin->n_frags > MAX_FRAGS)
				/* TODO: increase and reallocate the array */;

			msg_origin->n_frags++;

			/* If this is the last fragment, or if it is already
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


int main(int argc, char **argv)
{

	char *usage, *address;
	int port;
	int i, idx; 
	int epollfd, nfds;
	struct epoll_event events[MAX_EPOLL_EVENTS];
	GHashTable *hashtb;

	asprintf(&usage, "Usage: %s %s", basename(argv[0]), USAGE);
	
	/* Initialize configuration */
	utils_default_conf();

	/* Arguments validation */
	if (utils_get_opt(argc, argv) == -1 || argc - optind < 2) {

		utils_exit_error("Some required argument is missing.\n\n" 
		                 "%s", usage);
	} else if (conf.ip_vers == IPPROTO_IPV6 && conf.iface_name == NULL) { 

		utils_exit_error("Interface MUST be specified while using IPv6 "
		                 "(see '-i' option).\n\n%s", usage);
	} else {
		address = argv[optind++];
		port = atoi(argv[optind++]);
	}

	free(usage);

	if (conf.ip_vers == IPPROTO_IPV6) 
		net_init_ipv6_shared_instance(conf.iface_name, address, port);
	else 
		net_init_ipv4_shared_instance(address, port);


	/* Init epoll for handling events of the shared socket descriptor */
	epollfd = epoll_init(net_get_shared_descriptor());
	if (epollfd == -1)
		utils_exit_error("init epoll failed on sd");
	
	hashtb = g_hash_table_new(g_int_hash, g_int_equal);

	idx = 0;
	/* Main epoll loop. Wait for events on the socket descriptor.
	 * If an EPOLLOUT event is triggered a new message can be sent.
	 * If an EPOLLERR event is triggered some ted error message 
	 * may be present in the errqueue. */
	for (;;) {

		nfds = epoll_wait(epollfd, events, MAX_EPOLL_EVENTS, -1);
		if (nfds == -1)
			utils_exit_error("epoll_wait error\n");
		
		for (i = 0; i < nfds; i++) {

			/* Send a new message if the socket is ready for writing */
			if (events[i].events & EPOLLOUT && idx < conf.n_packets) {
				send_new_msg(hashtb);
				idx++;
			}

			/* If there are pending TED error messages 
			 * in the errqueue, receive them and print TED infos. */
			if (events[i].events & EPOLLERR) {
				recv_ted_errors(hashtb);
			}

		}
	}

	g_hash_table_destroy(hashtb);
	net_release_shared_instance();

	return 0;
}
