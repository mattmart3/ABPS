#ifndef NETWORK_H
#define NETWORK_H

/* Initialization and release. */
int net_init_ipv4_shared_instance(char *address, int port);
int net_init_ipv6_shared_instance(char *ifname, char *address, int port);
void net_release_shared_instance(void);
int net_check_ip_instance_and_version();
int net_get_shared_descriptor(void);
int net_sendmsg(const char *message, int message_length, uint32_t *identifier);

#endif
