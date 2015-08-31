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
