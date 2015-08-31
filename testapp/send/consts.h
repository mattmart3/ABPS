#ifndef CONSTS_H
#define CONSTS_H

#define DEFAULT_N_PACKETS 5000
#define DEFAULT_MSG_LENGTH 1050 

#define ABPS_CMSG_TYPE 111
#define MAX_EPOLL_EVENTS 10

#define SOCKET_ERROR   ((int)-1)
#define SIZEBYTE 70000

#define MSGERR_BUFF_LEN 2048
#define MAX_FRAGS 8 /* Frags array default size, it may be increasing re-allocating */

#define UDP_HEADER_SIZE 8

#endif
