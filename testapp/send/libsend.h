#ifndef LIBSEND_H
#define LIBSEND_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h> /* SOL_UDP define. */
#include <arpa/inet.h>
#include <linux/errqueue.h>
#include <errno.h>

#define SOCKET_ERROR   ((int)-1)

#define SIZEBYTE 70000

typedef unsigned int u32;


#define HAVE_IP_RECVERR 1

#if HAVE_IP_RECVERR

	#include <sys/uio.h>
#else
	#warning "HAVE_IP_RECVERR not defined"
	printf ("setsockopt() IP_PROTO IP_RECVERR not defined\n");
#endif

typedef struct struct_errormessage
{
	union
	{
		struct sockaddr_in6 ipv6_name;
		struct sockaddr_in  ipv4_name;
	} name;

	unsigned int namelen;

	struct cmsghdr *c;

	struct sock_extended_err *ee;

	char control[512];

	int	len_control;

	char errmsg[64 + 768];

	int	len_errmsg;

	struct msghdr msg[1];

	int	lenrecv;
	int	myerrno;

	int is_ipv6;

} ErrMsg;


ErrMsg* alloc_init_ErrMsg(void);

/* Initialization and release. */

int net_init_ipv4_shared_instance(char *address, int port);
int net_init_ipv6_shared_instance(char *ifname, char *address, int port);
void net_release_shared_instance(void);
int net_check_ip_instance_and_version();
int net_get_shared_descriptor(void);
int net_sendmsg(const char *message, int message_length, uint32_t *identifier);

#endif
