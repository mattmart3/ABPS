//
//  sendclientapp.c
//  
//
//  Created by Gabriele Di Bernardo on 10/05/15.
//
//

#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <netinet/in.h>

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

/* Compose a new simple message of a fixed size. */
char * new_msg(void)
{
	char *content;

	/* Init the message content space */
	content = (char *)malloc(conf.msg_length * sizeof(char));
	if (content == NULL)
		utils_exit_error("%s: malloc failed", __func__);

	/* Fill the message content */
	memset(content, 'c', conf.msg_length - 1);
	content[conf.msg_length - 1] = '\0';

	return content;
}

/* Send a new message requesting an identifier for TED. Check net_sendmsg(). */
void send_new_msg()
{
	uint32_t identifier;
	char *buffer;

	/* Compose a new message and sent it */
	buffer = new_msg();
	net_sendmsg(buffer, strlen(buffer), &identifier);
	printf("-----------------------------------------------------"
	       "----------------\n Sent pkt ID: %d\nsize: %ld\n", 
	       identifier, strlen(buffer));

	free(buffer);
}

/* Receive all the ted notification error that are pending in the errqueue */
void recv_ted_errors()
{
	ErrMsg *error_message = alloc_init_ErrMsg();

	/* Receive all the notifications in the errqueue.
	 * tederror_recv_nowait returns false if the errqueue is empty. */
	while (tederror_recv_nowait(error_message)) {
		struct ted_info_s ted_info;

		/* Application does not use local notification info. */
		tederror_check_ted_info(error_message, &ted_info);

		printf("-------------------------------------------------"
		       "--------------------\n\t\t\tTED notified ID: %d \n",
		       ted_info.msg_id);

		printf("\t\t\tmsg_id: %d, retry_count: %d, acked: %d\n"
		       "\t\t\tmore_frag: %d, frag_len: %d, frag_off: %d\n",
		       ted_info.msg_id, ted_info.retry_count, ted_info.status,
		       ted_info.more_frag, ted_info.frag_length, ted_info.frag_offset);

	}	

	free(error_message);

}


int main(int argc, char **argv)
{

	char *usage, *address;
	int port;
	int i, idx; 
	int epollfd, nfds;
	struct epoll_event events[MAX_EPOLL_EVENTS];

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
				send_new_msg();
				idx++;
			}

			/* If there are pending TED error messages 
			 * in the errqueue, receive them and print TED infos. */
			if (events[i].events & EPOLLERR) {
				recv_ted_errors();
			}

		}
	}

	net_release_shared_instance();

	return 0;
}
