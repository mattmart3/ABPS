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
