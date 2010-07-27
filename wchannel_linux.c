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

#include "posh.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "wstatus.h"
#include "wchannel.h"
#include "wlock.h"
#include "jmlist.h"
#include "debug.h"

typedef struct _msgentry_t {
	void *ptr;
	unsigned int size;
} msgentry_t;

typedef struct _msgbuf_t {
	msgentry_t **list_ptr;
   	unsigned int size;
} *msgbuf_t;

struct _wchannel_t {
	wchannel_opt_t chan_opt;
	int sock;
	struct _msgbuf_t message_buffer;
};

wstatus _msgbuf_free(msgbuf_t msg_buf);
wstatus _wchannel_udp_free(wchannel_t channel);
wstatus _wchannel_udp_create(wchannel_opt_t *chan_opt,wchannel_t *channel);
wstatus _wchannel_udp_send(wchannel_t channel,char *dest,void *msg_ptr,unsigned int msg_size,unsigned int *msg_used);
wstatus _wchannel_udp_recv(wchannel_t channel,void *msg_ptr,unsigned int msg_size,unsigned int *msg_used);

bool unloading = false;
bool loaded = false;

/*
   _wchannel_free_entry

   Helper function to free a channel data structure. The way of freeing a channel
   depends on the type of channel.
*/
wstatus
_wchannel_free_entry(wchannel_t channel)
{
	wstatus ws;

	dbgprint(MOD_WCHANNEL,__func__,"called with channel=%p",channel);

	if( !channel ) {
 		dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) channel argument is invalid",channel);
		goto return_fail;
	}

	switch(channel->chan_opt.type)
	{
		case WCHANNEL_TYPE_SOCKUDP:
			ws = _wchannel_udp_free(channel);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) failed to free this channel",channel);
				goto return_fail;
			}
			break;
		case WCHANNEL_TYPE_SOCKTCP:
		case WCHANNEL_TYPE_PIPE:
		case WCHANNEL_TYPE_FIFO:
		default:
			dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) invalid or unsupported channel type");
			goto return_fail;
	}

	dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) freeing channel structure",channel);
	free(channel);

	dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) returning with success.",channel);
	return WSTATUS_SUCCESS;

return_fail:
	dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) returning with failure.",channel);
	return WSTATUS_FAILURE;
}

/*
   _wchannel_udp_free

   Helper function to free an UDP channel data structure. This function assumes the channel
   argument was already validated. Should return success when the UDP data structure is
   freed successfully.
*/
wstatus
_wchannel_udp_free(wchannel_t channel)
{
	dbgprint(MOD_WCHANNEL,__func__,"called with channel=%p",channel);

	if( channel->chan_opt.debug_opts == WCHANNEL_MESSAGE_BUFFER )
	{
		dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) message buffer will be freed (p=%p)",
				channel,channel->message_buffer);

		_msgbuf_free(&channel->message_buffer);
		
		dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) message buffer freed (p=%p)",
				channel,channel->message_buffer);
	}

	dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) closing socket descriptor",channel);
	
	close(channel->sock);
	
	dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) socket fd=%d was closed",
			channel,channel->sock);

	DBGRET_SUCCESS(MOD_WCHANNEL);
}
/*
   _wchannel_udp_create

   Handler function to create an UDP socket with options specified
   by chan_opt. The created socket should be saved in channel parameter.
*/
wstatus
_wchannel_udp_create(wchannel_opt_t *chan_opt,wchannel_t *channel)
{
	int sock,ecode;
	struct addrinfo hints,*result,*rp;
	wchannel_t new_channel;
	struct sockaddr_in *psin;

	dbgprint(MOD_WCHANNEL,__func__,"called with chan_opt=%p and channel=%p",chan_opt,channel);

	/* validate arguments */
	if( !chan_opt ) {
		dbgprint(MOD_WCHANNEL,__func__,"missing channel options structure (chan_opt=0)");
		goto return_fail_early;
	}
	
	if( !channel ) {
		dbgprint(MOD_WCHANNEL,__func__,"missing channel argument (channel=0)");
		goto return_fail_early;
	}

	if( chan_opt->type != WCHANNEL_TYPE_SOCKUDP ) {
		dbgprint(MOD_WCHANNEL,__func__,"unexpected call to this function (sock type is != udp)");
		goto return_fail_early;
	}

	/* create socket */

	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = 0;

	ecode = getaddrinfo(chan_opt->host_src, chan_opt->port_src, &hints, &result);
	if( ecode != 0 )
	{
		dbgprint(MOD_WCHANNEL,__func__,"unable to get address info for binding host (%s)",
				gai_strerror(ecode));
		goto return_fail_early;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if( sock < 0 )
			continue;

		/* socket created successfuly */
		freeaddrinfo(result);
		goto sock_created;
	}

	freeaddrinfo(result);
	dbgprint(MOD_WCHANNEL,__func__,"unable to find a valid bind address (host_src=%s, port_src=%s)",
			(chan_opt->host_src ? chan_opt->host_src : "NULL"),
		   (chan_opt->port_src ? chan_opt->port_src : "NULL") );
	goto return_fail_early;

sock_created:
	dbgprint(MOD_WCHANNEL,__func__,"socket created successfully (sock=%d)",sock);

	psin = (struct sockaddr_in*)rp->ai_addr;
	dbgprint(MOD_WCHANNEL,__func__,"binding of the socket to host=%s and port=%d",
			inet_ntoa(psin->sin_addr),htons(psin->sin_port));
	ecode = bind(sock,rp->ai_addr,rp->ai_addrlen);
	if( ecode < 0 ) {
		/* this might happen if for example the port is already taken by other pid */
		dbgprint(MOD_WCHANNEL,__func__,"bind of socket failed (host=%s, port=%d)",
				inet_ntoa(psin->sin_addr),htons(psin->sin_port));
		goto return_fail_sock;
	}
	dbgprint(MOD_WCHANNEL,__func__,"bind to host=%s and port=%d was successful",
			inet_ntoa(psin->sin_addr),htons(psin->sin_port));

	/* allocate new channel_t structure to store in internal channel list */
	dbgprint(MOD_WCHANNEL,__func__,"allocating memory for new channel_t struct");
	new_channel = (wchannel_t) malloc(sizeof(struct _wchannel_t));
	dbgprint(MOD_WCHANNEL,__func__,"memory allocated for new channel_t (p=%p)",new_channel);
	memset(new_channel,0,sizeof(struct _wchannel_t));
	dbgprint(MOD_WCHANNEL,__func__,"memory of new channel_t cleared");

	dbgprint(MOD_WCHANNEL,__func__,"copying channel options into new channel_t (p=%p)",new_channel);
	memcpy(&new_channel->chan_opt,chan_opt,sizeof(wchannel_opt_t));
	dbgprint(MOD_WCHANNEL,__func__,"copying socket handle into new channel_t (p=%p)",new_channel);
	new_channel->sock = sock;

	dbgprint(MOD_WCHANNEL,__func__,"updating channel argument");
	*channel = new_channel;
	dbgprint(MOD_WCHANNEL,__func__,"new channel (%p) value is %p",*channel);

	dbgprint(MOD_WCHANNEL,__func__,"returning with success.");
	return WSTATUS_SUCCESS;

return_fail_sock:
	dbgprint(MOD_WCHANNEL,__func__,"closing socket %d",sock);
	close(sock);

return_fail_early:
	dbgprint(MOD_WCHANNEL,__func__,"returning with failure.");
	return WSTATUS_FAILURE;
}

/*
   _wchannel_udp_send

   Helper function to send a message using an UDP socket and destination
   specified in dest string. Format of the destiny string is "<host> <port>"
   there is a space separating host and port.

   Should return success if the message is sent successfully, failure otherwise.
*/
wstatus
_wchannel_udp_send(wchannel_t channel,char *dest,void *msg_ptr,unsigned int msg_size,unsigned int *msg_used)
{
	char *phost,*pport;
	char *dest_dup;
	struct addrinfo hints,*result,*rp;
	int sret,ecode;

	dbgprint(MOD_WCHANNEL,__func__,"called with channel=%p, dest=%s, msg_ptr=%p, msg_size=%u, msg_used=%p",
			channel,dest,msg_ptr,msg_size,msg_used);

	
	/* parse dest string */

	if( !dest && (channel->chan_opt.host_dst && channel->chan_opt.port_dst) )
	{
		phost = channel->chan_opt.host_dst;
		pport = channel->chan_opt.port_dst;
	} else if( dest )
	{
		dest_dup = strdup(dest);
		char *pchar = dest_dup;
		while( (*pchar != ' ') && (*pchar != '\0') )
			pchar++;
		
		if( *pchar == '\0' )
		{
			dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) dest string format is invalid, missing port information");
			goto return_fail;
		} else
		{
			phost = dest_dup;
			pport = pchar + 1;
			*pchar = '\0';
		}
	} else
	{
		dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) couldn't find any destination information",channel);
		goto return_fail;
	}
	dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) detected host=%s and port=%s in dest string",
			channel,phost,pport);

	/* create addr structure for this destination */
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = 0;

	ecode = getaddrinfo(phost,pport, &hints, &result);
	if( ecode != 0 )
	{
		dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) unable to get address info for destination host (%s)",
				channel,gai_strerror(ecode));
		goto return_fail;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		sret = sendto(channel->sock,msg_ptr,msg_size,0,(struct sockaddr*)rp->ai_addr,rp->ai_addrlen);
		if( sret < 0 )
			continue;

		/* socket created successfuly */
		freeaddrinfo(result);
		goto msg_sent;
	}

	/* failed to sent */
	freeaddrinfo(result);
	dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) failed to send message to host %s:%s (%s)",
			channel,phost,pport,strerror(errno));
	goto return_fail;

msg_sent:
	dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) message sent successfully (bytes_sent=%d)",channel,sret);

	if( msg_used ) {
		dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) updating msg_used argument",channel);
		*msg_used = (unsigned int)sret;
		dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) new msg_used value is %u",channel,*msg_used);
	}
	dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) msg_used is 0 so won't update it",channel);

	DBGRET_SUCCESS(MOD_WCHANNEL);

return_fail:
	
	DBGRET_FAILURE(MOD_WCHANNEL);
}

/*
   _wchannel_udp_recv

   Helper function to be used with sockets to receive data. This function is synchronous
   meaning that it will hang waiting for data if there's no data available.
*/
wstatus
_wchannel_udp_recv(wchannel_t channel,void *msg_ptr,unsigned int msg_size,unsigned int *msg_used)
{
	ssize_t recv_bytes;

	dbgprint(MOD_WCHANNEL,__func__,"called with channel=%p, msgptr=%p, msgsize=%u, msgused=%p",
			channel,msg_ptr,msg_size,msg_used);

	recv_bytes = recv(channel->sock,msg_ptr,msg_size,0);
	if( recv_bytes < 0 ) {
		dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) failed to receive data from socket (%s)",strerror(errno));
		goto return_fail;
	} else if ( recv_bytes == 0 ) {
		dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) peer closed half-side of connection",channel);
		goto return_fail;
	}

	if( msg_used ) {
		dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) updating msg_used argument to %u",channel,recv_bytes);
		*msg_used = (unsigned int)recv_bytes;
		dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) msg_used was updated to %u",channel,*msg_used);
	} else {
		dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) msg_used is null so its not going to be used",channel);
	}

	DBGRET_SUCCESS(MOD_WCHANNEL);

return_fail:
	DBGRET_FAILURE(MOD_WCHANNEL);
}

/*
   _msgbuf_clear

   Helper function to clear the message buffer..the right way.
*/
wstatus
_msgbuf_clear(msgbuf_t msg_buf)
{
	dbgprint(MOD_WCHANNEL,__func__,"called with msgbuf_ptr=%p, msgbuf_size=%u",msg_buf->list_ptr,msg_buf->size);

	for( unsigned int i = 0 ; i < msg_buf->size ; i++ )
		msg_buf->list_ptr[i] = NULL;

	dbgprint(MOD_WCHANNEL,__func__,"returning with success.");
	return WSTATUS_SUCCESS;
}

/*
   _msgbuf_entry_alloc

   Helper function to allocate a new msg buffer entry.
*/
wstatus
_msgbuf_entry_alloc(msgentry_t **new_entry)
{
	dbgprint(MOD_WCHANNEL,__func__,"called with new_entry=%p",new_entry);

	msgentry_t *entry;

	entry = (msgentry_t*) malloc(sizeof(msgentry_t));
	if( !entry ) {
		dbgprint(MOD_WCHANNEL,__func__,"malloc failed");
		goto return_fail;
	}
	dbgprint(MOD_WCHANNEL,__func__,"memory allocated successfully (p=%p)",entry);

	dbgprint(MOD_WCHANNEL,__func__,"clearing new entry structure");
	memset(entry,0,sizeof(msgentry_t));

	dbgprint(MOD_WCHANNEL,__func__,"updating new_entry to %p",entry);
	*new_entry = entry;

	DBGRET_SUCCESS(MOD_WCHANNEL);
return_fail:
	DBGRET_FAILURE(MOD_WCHANNEL);
}

/*
   _msgbuf_insert

   Helper function to insert a new message buffer entry. Message buffers have a static size
   so whenever a new entry is inserted, the last one pops out (discarded).
   The new_entry string pointer will 
*/
wstatus
_msgbuf_insert(msgbuf_t msg_buf,void *msg_ptr,unsigned int msg_size)
{
	msgentry_t *ptail_entry;
	wstatus ws;
	msgentry_t *new_entry;
	unsigned int i;

	dbgprint(MOD_WCHANNEL,__func__,"called with msg_buf=%p, msg_ptr=%p, msg_size=%u",
			msg_buf,msg_ptr,msg_size);

	/* validate arguments */

	if( !msg_buf ) {
		dbgprint(MOD_WCHANNEL,__func__,"missing msg_buf argument (msg_buf=0)");
		goto return_fail;
	}

	if( !msg_ptr ) {
		dbgprint(MOD_WCHANNEL,__func__,"missing msg_ptr argument (msg_ptr=0)");
		goto return_fail;
	}

	if( !msg_size ) {
		dbgprint(MOD_WCHANNEL,__func__,"message size is null");
		goto return_fail;
	}

	ws = _msgbuf_entry_alloc(&new_entry);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WCHANNEL,__func__,"(msgbuf=%p) failed to allocate new message entry",msg_buf);
		goto return_fail;
	}
	dbgprint(MOD_WCHANNEL,__func__,"(msgbuf=%p) allocated new message entry successfully (entry=%p)",msg_buf,new_entry);

	dbgprint(MOD_WCHANNEL,__func__,"(msgbuf=%p) filling new message entry fields (entry=%p, ptr=%p, size=%u)",
			msg_buf,new_entry,msg_ptr,msg_size);
	new_entry->ptr = msg_ptr;
	new_entry->size = msg_size;
	dbgprint(MOD_WCHANNEL,__func__,"(msgbuf=%p) new entry information is ptr=%p and size=%u",
		msg_buf,new_entry->ptr,new_entry->size);

	dbgprint(MOD_WCHANNEL,__func__,"(msgbuf=%p) accessing tail entry (%d)",msg_buf,msg_buf->size);
	ptail_entry = msg_buf->list_ptr[msg_buf->size - 1];
	if( ptail_entry ) {
		/* we've a tail entry, should free it from memory */
		dbgprint(MOD_WCHANNEL,__func__,"(msgbuf=%p) freeing tail entry (entry=%p, ptr=%p, size=%u)",
				msg_buf,ptail_entry,ptail_entry->ptr,ptail_entry->size);

		free(ptail_entry->ptr);
		free(ptail_entry);

		dbgprint(MOD_WCHANNEL,__func__,"(msgbuf=%p) tail entry was freed successfully");
	}
	
	/* shift all entries one position */
	dbgprint(MOD_WCHANNEL,__func__,"(msgbuf=%p) shifiting message buffer",msg_buf);
	for( i = 0 ; i < (msg_buf->size-1) ; i++ )
	{
		msg_buf->list_ptr[i+1]->ptr = msg_buf->list_ptr[i]->ptr;
		msg_buf->list_ptr[i+1]->size = msg_buf->list_ptr[i]->size;
	}
	dbgprint(MOD_WCHANNEL,__func__,"(msgbuf=%p) shifting completed successfully",msg_buf);

	dbgprint(MOD_WCHANNEL,__func__,"(msgbuf=%p) updating head entry to new ptr=%p",msg_buf,new_entry);
	msg_buf->list_ptr[0] = new_entry;

	dbgprint(MOD_WCHANNEL,__func__,"returning with success.");
	return WSTATUS_SUCCESS;

return_fail:
	dbgprint(MOD_WCHANNEL,__func__,"returning with failure.");
	return WSTATUS_FAILURE;
}

/*
   _msgbuf_free

   Helper function to free a message buffer. A message buffer as you know
   is a vector of pointers to msgentry_t structures. These structures also
   have pointers to allocated memory.
*/
wstatus
_msgbuf_free(msgbuf_t msg_buf)
{
	msgentry_t *entry;

	dbgprint(MOD_WCHANNEL,__func__,"called with msgbuf=%p",msg_buf);

	if( !msg_buf ) {
		dbgprint(MOD_WCHANNEL,__func__,"missing msgbuf argument (msgbuf=0)");
		goto return_fail;
	}

	for( unsigned int i = 0 ; i < msg_buf->size ; i++ )
	{
		dbgprint(MOD_WCHANNEL,__func__,"(msgbuf=%p) accessing message buffer entry %u",msg_buf,i);
		if( msg_buf->list_ptr[i] ) {
			entry = msg_buf->list_ptr[i];
			if( entry->ptr ) {
				dbgprint(MOD_WCHANNEL,__func__,"(msgbuf=%p) freeing entry (%u,%p) ptr=%p",msg_buf,i,entry,entry->ptr); 
				free(entry->ptr);
			}
			dbgprint(MOD_WCHANNEL,__func__,"(msgbuf=%p) freeing entry (%u,%p)",msg_buf,i,entry);
			free(entry);
			msg_buf->list_ptr[i] = 0;
		}
	}

	dbgprint(MOD_WCHANNEL,__func__,"(msgbuf=%p) all entries were freed, now freeing msgbuf structure",msg_buf);
	free(msg_buf);

	DBGRET_SUCCESS(MOD_WCHANNEL);
return_fail:
	DBGRET_FAILURE(MOD_WCHANNEL);
}

/*
   wchannel_create

   Channel options are passed in wchannel_opt_t structure, the channel
   if successful created is returned in channel pointer. This value is
   only valid if the function returns success.

   There are various types of channels, please refer to the dev guide
   for more information and examples about these.

   To the client, channel is an opaque pointer to a structure, the
   definition of the structure is only available in wchannel_X.c this
   way the client code doesn't have access to the wchannel structure.
*/
wstatus
wchannel_create(wchannel_opt_t *chan_opt,wchannel_t *channel)
{
	wstatus ws;
	wchannel_t new_channel;

	dbgprint(MOD_WCHANNEL,__func__,"called with chan_opt=%p and channel=%p",chan_opt,channel);

	if( !loaded ) {
		dbgprint(MOD_WCHANNEL,__func__,"module was not loaded yet");
		dbgprint(MOD_WCHANNEL,__func__,"returning with failure.");
		return WSTATUS_FAILURE;
	}

	if( unloading ) {
		dbgprint(MOD_WCHANNEL,__func__,"cannot call this function when module is unloading");
		dbgprint(MOD_WCHANNEL,__func__,"returning with failure.");
		return WSTATUS_FAILURE;
	}

	/* validate arguments */
	if( !chan_opt ) {
		dbgprint(MOD_WCHANNEL,__func__,"missing channel options structure (chan_opt=0)");
		goto return_fail_early;
	}
	
	if( !channel ) {
		dbgprint(MOD_WCHANNEL,__func__,"missing channel argument (channel=0)");
		goto return_fail_early;
	}

	switch(chan_opt->type)
	{
		case WCHANNEL_TYPE_SOCKUDP:
			dbgprint(MOD_WCHANNEL,__func__,"calling _wchannel_udp_create function");
			ws = _wchannel_udp_create(chan_opt,&new_channel);
			dbgprint(MOD_WCHANNEL,__func__,"function has returned ws=%u",ws);

			if( ws != WSTATUS_SUCCESS )
				goto return_fail_early;

			dbgprint(MOD_WCHANNEL,__func__,"UDP channel created (channel=%p)",new_channel);

			break;
		case WCHANNEL_TYPE_SOCKTCP:
		case WCHANNEL_TYPE_PIPE:
		case WCHANNEL_TYPE_FIFO:
		default:
			dbgprint(MOD_WCHANNEL,__func__,"invalid or unsupported channel type specified (%d)",
					chan_opt->type);
			goto return_fail_early;
	}

	/* setup debugging */

	switch(chan_opt->debug_opts)
	{
		case WCHANNEL_DUMP_CALLBACK:
			if( !chan_opt->dump_cb ) {
				dbgprint(MOD_WCHANNEL,__func__,
						"(new_channel=%p) need to fill dump callback function address (proc=0)",
						new_channel);
				goto return_fail_channel;
			}

			dbgprint(MOD_WCHANNEL,__func__,"(new_channel=%p) using dump callback for channel debugging (proc=%p)",
					new_channel,chan_opt->dump_cb);
			break;
		case WCHANNEL_MESSAGE_BUFFER:
			dbgprint(MOD_WCHANNEL,__func__,"(new_channel=%p) using message buffer for channel debugging (size=%u)",
					new_channel,chan_opt->buffer_size);
			new_channel->message_buffer.list_ptr = (msgentry_t**) malloc(sizeof(msgentry_t*)*chan_opt->buffer_size);
			if( !new_channel->message_buffer.list_ptr ) {
				dbgprint(MOD_WCHANNEL,__func__,"(new_channel=%p) failed to create message buffer (size=%u)",
						new_channel,chan_opt->buffer_size);
				goto return_fail_channel;
			}
			dbgprint(MOD_WCHANNEL,__func__,"(new_channel=%p) allocated message buffer successfully (p=%p)",
					new_channel,new_channel->message_buffer.list_ptr);

			dbgprint(MOD_WCHANNEL,__func__,"(new_channel=%p) clearing message buffer",new_channel);
			_msgbuf_clear(&new_channel->message_buffer);
			break;
		case WCHANNEL_NO_DEBUG:
			dbgprint(MOD_WCHANNEL,__func__,"(new_channel=%p) debugging disabled in this channel",new_channel);
			break;
		default:
			dbgprint(MOD_WCHANNEL,__func__,"(new_channel=%p) invalid or unsupported channel debugging options (opt=%d)",
					new_channel,chan_opt->debug_opts);
			goto return_fail_channel;
	}

	dbgprint(MOD_WCHANNEL,__func__,"(new_channel=%p) updating channel pointer to %p",new_channel,new_channel);
	*channel = new_channel;
	dbgprint(MOD_WCHANNEL,__func__,"(new_channel=%p) updated channel pointer to %p successfully",new_channel,*channel);

	dbgprint(MOD_WCHANNEL,__func__,"(new_channel=%p) returning with success.",new_channel);
	return WSTATUS_SUCCESS;

return_fail_channel:
	/* free allocated channel */
	close(new_channel->sock);
	free(new_channel);

return_fail_early:
	dbgprint(MOD_WCHANNEL,__func__,"returning with failure.");
	return WSTATUS_FAILURE;
}

/*
   wchannel_send

   Sends the buffer_size bytes through the channel. The destination
   string format depends on the channel type:
SOCKET:
	dest = [<host>[ <port>]]
	if <port> is not used, port specified in chan_opt will be used.
	if <host> and port are not used, the host and port specified in chan_opt will be used.
PIPE:
	...
FIFO:
	...

	If buffer_used != 0, an integer will be written on success indicating
	the number of bytes sent. This doesn't tell anything about bytes reception tho.

Implementation:
	1) check for arguments
	2) lookup channel in internal channel list
	3) validate dest, build sockaddr from dest string
	4) send buffer

Return Values:
	WSTATUS_FAILURE message failed to sent, it can be considered that no message was sent.
	WSTATUS_SEMIFAIL message was sent, but wchannel was unable to store it on message history
	(check the debugging output for more information on why and where).
	WSTATUS_SUCCESS when message was sent and was stored on message history (if active) successfully.
*/
wstatus
wchannel_send(wchannel_t channel,char *dest,void *msg_ptr,unsigned int msg_size,unsigned int *msg_used)
{
	unsigned int bytes_sent = 0;
	wstatus ws;

	dbgprint(MOD_WCHANNEL,__func__,"called with channel=%p, msg_ptr=%p, msg_size=%u, msg_used=%p",
			channel,msg_ptr,msg_size,msg_used);

	if( !loaded ) {
		dbgprint(MOD_WCHANNEL,__func__,"module was not loaded yet");
		dbgprint(MOD_WCHANNEL,__func__,"returning with failure.");
		return WSTATUS_FAILURE;
	}

	if( unloading ) {
		dbgprint(MOD_WCHANNEL,__func__,"cannot call this function when module is unloading");
		dbgprint(MOD_WCHANNEL,__func__,"returning with failure.");
		return WSTATUS_FAILURE;
	}

	if( !channel ) {
		dbgprint(MOD_WCHANNEL,__func__,"missing channel argument (channel=0)");
		goto return_fail;
	}

	if( !dest || !strlen(dest)) {
		dbgprint(MOD_WCHANNEL,__func__,"missing argument dest (dest=0 or empty dest)");
		goto return_fail;
	}

	if( !msg_ptr || !msg_size ) {
		dbgprint(MOD_WCHANNEL,__func__,"missing message to send or message size is zero");
		goto return_fail;
	}

	/* now ready to send */

	switch(channel->chan_opt.type)
	{
		case WCHANNEL_TYPE_SOCKUDP:
			dbgprint(MOD_WCHANNEL,__func__,"calling helper function wchannel_udp_send");
			ws = _wchannel_udp_send(channel,dest,msg_ptr,msg_size,&bytes_sent);
			dbgprint(MOD_WCHANNEL,__func__,"helper function returned ws=%d",ws);
			break;
		case WCHANNEL_TYPE_SOCKTCP:
		case WCHANNEL_TYPE_PIPE:
		case WCHANNEL_TYPE_FIFO:
		default:
			dbgprint(MOD_WCHANNEL,__func__,"invalid or unsupported channel type %d "
					"(check your channel pointer!)",channel->chan_opt.type);
			goto return_fail;
	}

	/* check helper return value */
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WCHANNEL,__func__,"failed to send message");
		goto return_fail;
	}

	/* update msg_used if its not null */
	if( msg_used ) {
		dbgprint(MOD_WCHANNEL,__func__,"updating msg_used argument with %u",bytes_sent);
		*msg_used = bytes_sent;
		dbgprint(MOD_WCHANNEL,__func__,"new msg_used value is %u",*msg_used);
	}

	/* handle the message history */
	switch(channel->chan_opt.debug_opts)
	{
		case WCHANNEL_MESSAGE_BUFFER:
		
			/* insert message in message history of the channel */
			dbgprint(MOD_WCHANNEL,__func__,"calling helper function msgbuf_insert with msgbuf=%p msgptr=%p msgsize=%u",
					&channel->message_buffer,msg_ptr,msg_size);

			ws = _msgbuf_insert(&channel->message_buffer,msg_ptr,msg_size);

			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_WCHANNEL,__func__,"msgbuf_insert failed (ws=%d)",ws);
				goto return_semifail;
			}

			dbgprint(MOD_WCHANNEL,__func__,"added message to message history successfully");
			break;

		case WCHANNEL_DUMP_CALLBACK:
			
			/* call the user defined callback function */
			if( !channel->chan_opt.dump_cb ) {
				dbgprint(MOD_WCHANNEL,__func__,"debug opts = dump_cb but dump_cb=0");
				goto return_semifail;
			}

			dbgprint(MOD_WCHANNEL,__func__,"calling dump_cb=%p with dest=%s, msg_ptr=%p, msg_size=%u and msg_used=%u",
					channel->chan_opt.dump_cb,dest,msg_ptr,msg_size,bytes_sent);

			channel->chan_opt.dump_cb(dest,msg_ptr,msg_size,bytes_sent);

			dbgprint(MOD_WCHANNEL,__func__,"dump_cb returned successfully");
			break;

		case WCHANNEL_NO_DEBUG:
			dbgprint(MOD_WCHANNEL,__func__,"channel has debug off");
			break;
		default:
			dbgprint(MOD_WCHANNEL,__func__,"invalid or unsupported debug option %d"
					" (check your channel pointer!)",channel->chan_opt.debug_opts);
			goto return_semifail;
	}

	DBGRET_SUCCESS(MOD_WCHANNEL);

return_semifail:
	dbgprint(MOD_WCHANNEL,__func__,"returning semifail.");
	return WSTATUS_SEMIFAIL;

return_fail:
	DBGRET_FAILURE(MOD_WCHANNEL);
}

wstatus
wchannel_receive(wchannel_t channel,void *buffer_ptr,unsigned int buffer_size,unsigned int *buffer_used)
{
	dbgprint(MOD_WCHANNEL,__func__,"called with channel=%p, buffer_ptr=%p, buffer_size=%u, buffer_used=%p",
			channel,buffer_ptr,buffer_size,buffer_used);

	if( !loaded ) {
		dbgprint(MOD_WCHANNEL,__func__,"module was not loaded yet");
		dbgprint(MOD_WCHANNEL,__func__,"returning with failure.");
		return WSTATUS_FAILURE;
	}

	if( unloading ) {
		dbgprint(MOD_WCHANNEL,__func__,"cannot call this function when module is unloading");
		dbgprint(MOD_WCHANNEL,__func__,"returning with failure.");
		return WSTATUS_FAILURE;
	}
	return WSTATUS_UNIMPLEMENTED;
}

/*
   wchannel_destroy

   Function to destroy a previously created channel. 
   Its important to note that after the channel is removed (_wchannel_free_entry) if an
   error occurs, this function will not undo the removal of the entry! It will return
   failure in a sense that not all operations went OK.
*/
wstatus
wchannel_destroy(wchannel_t channel)
{
	wstatus ws;

	dbgprint(MOD_WCHANNEL,__func__,"called with channel=%p",channel);

	if( !loaded ) {
		dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) module was not loaded yet",channel);
		dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) returning with failure.",channel);
		return WSTATUS_FAILURE;
	}

	if( unloading ) {
		dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) cannot call this function when module is unloading",channel);
		dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) returning with failure.",channel);
		return WSTATUS_FAILURE;
	}

	dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) calling wchannel_free_entry",channel);
	ws = _wchannel_free_entry(channel);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) wchannel_free_entry failed",channel);
		goto return_fail;
	}

	dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) wchannel_free_entry returned success",channel);
	dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) returning with success.",channel);
	return WSTATUS_SUCCESS;

return_fail:
	dbgprint(MOD_WCHANNEL,__func__,"(channel=%p) returning with failure.",channel);
	return WSTATUS_FAILURE;
}

/*
   wchannel_load

   Loads wchannel module. This is required before the module can
   be used by the client. If any function of wchannel is called
   before calling wchannel_load they all return failure status.
*/
wstatus
wchannel_load(wchannel_load_t load)
{
	dbgprint(MOD_WCHANNEL,__func__,"called");

	if( loaded ) {
		dbgprint(MOD_WCHANNEL,__func__,"module is loaded already");
		goto return_fail;
	}

	if( unloading ) {
		dbgprint(MOD_WCHANNEL,__func__,"module is still unloading");
		goto return_fail;
	}

	loaded = true;
	dbgprint(MOD_WCHANNEL,__func__,"updated loaded flag to %d",loaded);

	dbgprint(MOD_WCHANNEL,__func__,"returning with success.");
	return WSTATUS_SUCCESS;

return_fail:
	dbgprint(MOD_WCHANNEL,__func__,"returning with failure.");
	return WSTATUS_FAILURE;
}

/*
   wchannel_unload

   Unload wchannel module. This should disable creation of new channels
   and should free any existing module.
*/
wstatus
wchannel_unload(void)
{
	dbgprint(MOD_WCHANNEL,__func__,"called");

	if( !loaded ) {
		dbgprint(MOD_WCHANNEL,__func__,"module was not loaded, cannot call this function");
		goto return_fail;
	}

	if( unloading ) {
		dbgprint(MOD_WCHANNEL,__func__,"function called in paralel (previous call hasn't finished yet)");
		goto return_fail;
	}

	unloading = true;
	dbgprint(MOD_WCHANNEL,__func__,"unloading flag changed to %d",unloading);

	/* Here it should do the cleanup of data structures */

	loaded = false;
	dbgprint(MOD_WCHANNEL,__func__,"updated loaded flag to %d",loaded);

	unloading = false;
	dbgprint(MOD_WCHANNEL,__func__,"updated unloading flag to %d",unloading);

	dbgprint(MOD_WCHANNEL,__func__,"returning with success.");
	return WSTATUS_SUCCESS;

return_fail:
	dbgprint(MOD_WCHANNEL,__func__,"returning with failure.");
	return WSTATUS_FAILURE;
}

