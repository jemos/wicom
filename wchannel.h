/*
	This file is part of wicom.

	wicom is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	wicom is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with wicom.  If not, see <http://www.gnu.org/licenses/>.

	Copyright (C) 2009 Jean-François Mousinho <jean.mousinho@ist.utl.pt>
	Centro de Informatica do IST - Universidade Tecnica de Lisboa 
*/
/*
   Module Description

   This module is an interface between wicom and comunication Sockets.
   It should have only 4 basic functions exported: create, send, recv
   and destroy.

   It should be possible to bind the socket to a specific port and to
   specify interface, protocol and ip version to use. Default is ipv6.

   Hosts should be refered by hostname instead of IP addresses.

   No information of the host that sent the message is returned to the
   client application (this includes host address and port used to send
   the packet).

   It will be very helpful to have a way of tracing the messages
   exchanged between modules for debugging and troubleshooting issues.
   When creating the channel, the client might fill the options structure
   with the informations of how big will be buffer be (in messages).
   To consult the messages in the buffer, the client calls a specific
   function to peek messages from the message buffer. It might be
   possible also to configure the channel so that each message
   received or sent is dumped to the stdout.

   Example of usage:

   wchannel_t wch;
   wchannel_opt_t wch_opt = { .type = WCHANNEL_TYPE_SOCKUDP,
		.debug_opt = WCHANNEL_NO_DEBUG };
   char *host_src = "localhost";
   
   memcpy(wch_opt.host_src,host_src,sizeof(host_src));

   ws = wchannel_create(&wch_opt,&wch);
   if( ws != WSTATUS_SUCCESS ) { ... }

   ws = wchannel_send(wch,"10.0.0.1:1234","HELLO WORLD",10,0);
   if( ws != WSTATUS_SUCCESS ) { ... }

   ws = wchannel_destroy(wch);

*/

#ifndef _WCHANNEL_H
#define _WCHANNEL_H

#include "posh.h"
#include "wstatus.h"

typedef enum _wchannel_type_list
{
	WCHANNEL_TYPE_SOCKTCP,
	WCHANNEL_TYPE_SOCKUDP,
	WCHANNEL_TYPE_PIPE,
	WCHANNEL_TYPE_FIFO
} wchannel_type_list;

typedef enum _wchannel_debug_opts
{
	WCHANNEL_NO_DEBUG,
	WCHANNEL_DUMP_CALLBACK,
	WCHANNEL_MESSAGE_BUFFER
} wchannel_debug_opts;

typedef struct _wchannel_load_t
{
	void *junk;
} wchannel_load_t;

typedef wstatus (*WCHANNELDUMPCB)(char *dest,void *msg_ptr,unsigned int msg_size, unsigned int msg_used);

/* channel options */
typedef struct _wchannel_opt_t
{
	wchannel_type_list type;
	char *host_src;
	char *host_dst;
	char *port_src;
	char *port_dst;
	wchannel_debug_opts debug_opts;
	WCHANNELDUMPCB dump_cb;
	unsigned int buffer_size;
} wchannel_opt_t;

typedef struct _wchannel_t *wchannel_t;

wstatus wchannel_create(wchannel_opt_t *chan_opt,wchannel_t *channel);
wstatus wchannel_send(wchannel_t channel,char *dest,void *msg_ptr,unsigned int msg_size,unsigned int *msg_used);
wstatus wchannel_receive(wchannel_t channel,void *msg_ptr,unsigned int msg_size,unsigned int *msg_used);
wstatus wchannel_destroy(wchannel_t channel);

#endif

