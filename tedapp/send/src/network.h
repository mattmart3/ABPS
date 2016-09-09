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

#include "structs.h"

int net_create_socket(int ip_vers, int sock_type, char *address, 
		      int port, char *ifname, int ifbind);
ssize_t net_recv_msg(char *buffer, int length, int sd);
ssize_t net_send_msg(char *buffer, int length, int sd);
ssize_t net_send_ted_msg(char *buffer, int length, uint32_t *id_pointer, int sd);
int net_recv_err(struct err_msg_s **error_message, int sd);

#endif
