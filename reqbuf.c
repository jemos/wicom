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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "reqbuf.h"
#include "wchannel.h"
#include "modmgr.h"

struct _reqbuf_t {
	void *param;
	REQBUFREADCB read_cb;
	reqbuf_type_list type;
	void *buffer_ptr;
	size_t buffer_size;
	size_t buffer_used;
};

wstatus
reqbuf_load(reqbuf_load_t *load)
{
	load = load;
	DBGRET_SUCCESS(MOD_REQBUF);
}

wstatus
reqbuf_unload(void)
{
	DBGRET_SUCCESS(MOD_REQBUF);
}

/*
   reqbuf_wchannel_read_cb

   Callback function for reading from a wchannel, passed in opaque parameter when
   the request buffer was created.
*/
wstatus
reqbuf_wchannel_read_cb(void *param,void *chunk_ptr,unsigned int chunk_size, unsigned int *chunk_used)
{
	wchannel_t wch = (wchannel_t)param;
	wstatus ws;
	unsigned int bytes_used = 0;

	ws = wchannel_receive(wch,chunk_ptr,chunk_size,&bytes_used);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_REQBUF,__func__,"wchannel receive failed (ws=%s)",wstatus_str(ws));
		DBGRET_FAILURE(MOD_REQBUF);
	}

	dbgprint(MOD_REQBUF,__func__,"received successfully %u bytes from wch=%p",bytes_used,wch);
	*chunk_used = bytes_used;
	dbgprint(MOD_REQBUF,__func__,"updated chunk_used value to %u",*chunk_used);

	DBGRET_SUCCESS(MOD_REQBUF);
}

/*
   reqbuf_create

   Creates reqbuf_t data structure, filling it with param and read
   callback.
*/
wstatus
reqbuf_create(REQBUFREADCB read_cb,void *param,reqbuf_type_list type,reqbuf_t *rb)
{
	reqbuf_t new_rb = 0;

	dbgprint(MOD_REQBUF,__func__,"called with param=%p, read_cb=%p, type=%d, rb=%p",
			param,read_cb,type,rb);

	if( !rb ) {
		dbgprint(MOD_REQBUF,__func__,"invalid rb argument (rb=0)");
		goto return_fail;
	}

	/* allocate data structure */

	new_rb = (reqbuf_t)malloc(sizeof(struct _reqbuf_t));
	if( !new_rb ) {
		dbgprint(MOD_REQBUF,__func__,"malloc failed (size=%d)",sizeof(struct _reqbuf_t));
		goto return_fail;
	}
	dbgprint(MOD_REQBUF,__func__,"allocated new request buffer data structure (ptr=%p)",new_rb);
	
	/* fill request buffer data structure */
	new_rb->type = type;
	new_rb->read_cb = read_cb;
	new_rb->param = param;
	new_rb->buffer_ptr = malloc(REQBUF_INIT_SIZE);
	new_rb->buffer_size = REQBUF_INIT_SIZE;
	new_rb->buffer_used = 0;

	dbgprint(MOD_REQBUF,__func__,"request buffer data structure was initialized (ptr=%p)",new_rb);

	*rb = new_rb;
	dbgprint(MOD_REQBUF,__func__,"updated rb value to %p",*rb);

	DBGRET_SUCCESS(MOD_REQBUF);

return_fail:
	if(new_rb)
	{
		if( new_rb->buffer_ptr )
			free(new_rb->buffer_ptr);

		free(new_rb);
	}

	DBGRET_FAILURE(MOD_REQBUF);
}

/*
   _reqbuf_req_is_complete

   Helper function to find if a request is complete, otherwise its incomplete,
   data is missing yet. If request buffer type is TEXT, each request ends with
   a null char, if the request buffer type is BINARY it depends on the request
   stype. If stype is BINARY the request size is fixed, if stype is TEXT the
   request ends with the null char of data.text.raw[].
*/
wstatus
_reqbuf_req_is_complete(void *req_ptr,unsigned int max_size,reqbuf_type_list type,bool *complete_flag)
{
	unsigned int i;
	void *aux_ptr;
	char *char_ptr;
	request_t aux_req;

	dbgprint(MOD_REQBUF,__func__,"called with req_ptr=%p, max_size=%u, type=%d, complete_flag=%p",
			req_ptr,max_size,type,complete_flag);

	if( !req_ptr ) {
		dbgprint(MOD_REQBUF,__func__,"invalid req_ptr argument (req_ptr=0)");
		DBGRET_FAILURE(MOD_REQBUF);
	}

	if( !max_size ) {
		dbgprint(MOD_REQBUF,__func__,"expecting a non-empty request (max_size=0)");
		DBGRET_FAILURE(MOD_REQBUF);
	}

	if( !complete_flag ) {
		dbgprint(MOD_REQBUF,__func__,"invalid complete_flag argument (complete_flag=0)");
		DBGRET_FAILURE(MOD_REQBUF);
	}

	switch(type)
	{
		case REQBUF_TYPE_TEXT:
			char_ptr = (char*)req_ptr;
			for( i = 0 ; i < max_size ; i++ ) {
				if( V_REQENDCHAR(char_ptr[i]) )
					goto is_complete;
			}
			break;
		case REQBUF_TYPE_BINARY:
			aux_req = (request_t)req_ptr;
			aux_ptr = (void*)aux_req->data.text.raw;
			if( aux_req->stype == REQUEST_STYPE_TEXT )
		   	{
				/* The request ends when the REQENDCHAR is found in data.text.raw,
				   dont forget to check for buffer limits (max_size) which is relative
				   to aux_ptr = req_ptr and not to data.text.raw */
				for( i = 0 ; i < (max_size - ((char*)aux_ptr - (char*)req_ptr)) ; i++ ) {
					if( V_REQENDCHAR(aux_req->data.text.raw[i]) )
						goto is_complete;
				}
				/* No REQENDCHAR was found within the buffer. */
			} else if( aux_req->stype == REQUEST_STYPE_BIN )
			{
				/* the request end is well defined because the request size is fixed
				   in sizeof(struct _request_t)). Just check if at least one request
				   fits in the used bytes of the buffer. */
				if( max_size >= sizeof(struct _request_t) )
					goto is_complete;
			} else
			{
				dbgprint(MOD_REQBUF,__func__,"invalid or unsupported request stype (%d)",aux_req->stype);
				DBGRET_FAILURE(MOD_REQBUF);
			}
			break;
		default:
			dbgprint(MOD_REQBUF,__func__,"invalid or unsupported reqbuf type (%d)",type);
			DBGRET_FAILURE(MOD_REQBUF);
	}

	dbgprint(MOD_REQBUF,__func__,"request is incomplete");
	*complete_flag = false;
	dbgprint(MOD_REQBUF,__func__,"updated complete_flag value to %d",*complete_flag);
	DBGRET_SUCCESS(MOD_REQBUF);
	
is_complete:
	dbgprint(MOD_REQBUF,__func__,"request is complete");
	*complete_flag = true;
	dbgprint(MOD_REQBUF,__func__,"updated complete_flag value to %d",*complete_flag);
	DBGRET_SUCCESS(MOD_REQBUF);
}

/*
   _reqbuf_find_req_size

   Helper function to detect request total size. This can only be used if its
   sure that the request is complete (no byte is missing).
*/
wstatus
_reqbuf_find_req_size(void *req_ptr,unsigned int max_size,reqbuf_type_list type,unsigned int *req_size)
{
	unsigned int i;
	void *aux_ptr;
	char *char_ptr;
	request_t aux_req;

	dbgprint(MOD_REQBUF,__func__,"called with req_ptr=%p, max_size=%u, type=%d, req_size=%p",
			req_ptr,max_size,type,req_size);

	if( !req_ptr ) {
		dbgprint(MOD_REQBUF,__func__,"invalid req_ptr argument (req_ptr=0)");
		DBGRET_FAILURE(MOD_REQBUF);
	}

	if( !max_size ) {
		dbgprint(MOD_REQBUF,__func__,"expecting a non-empty request (max_size=0)");
		DBGRET_FAILURE(MOD_REQBUF);
	}

	if( !req_size ) {
		dbgprint(MOD_REQBUF,__func__,"invalid req_size argument (req_size=0)");
		DBGRET_FAILURE(MOD_REQBUF);
	}

	switch(type)
	{
		case REQBUF_TYPE_TEXT:
			char_ptr = (char*)req_ptr;
			for( i = 0 ; i < max_size ; i++ ) {
				if( V_REQENDCHAR(char_ptr[i]) ) {
					i++; goto end_of_request;
				}
			}
			break;
		case REQBUF_TYPE_BINARY:
			aux_req = (request_t)req_ptr;
			aux_ptr = (void*)aux_req->data.text.raw;
			if( aux_req->stype == REQUEST_STYPE_TEXT )
		   	{
				/* The request ends when the REQENDCHAR is found in data.text.raw,
				   dont forget to check for buffer limits (max_size) which is relative
				   to aux_ptr = req_ptr and not to data.text.raw */
				for( i = 0 ; i < (max_size - ((char*)aux_ptr - (char*)req_ptr)) ; i++ ) {
					if( V_REQENDCHAR(aux_req->data.text.raw[i]) ) {
						i++; goto end_of_request;
					}
				}
				/* No REQENDCHAR was found within the buffer. */
			} else if( aux_req->stype == REQUEST_STYPE_BIN )
			{
				/* the request end is well defined because the request size is fixed
				   in sizeof(struct _request_t)). Just check if at least one request
				   fits in the used bytes of the buffer. */
				i = sizeof(struct _request_t);
				if( max_size >= sizeof(struct _request_t) )
					goto end_of_request;
			} else
			{
				dbgprint(MOD_REQBUF,__func__,"invalid or unsupported request stype (%d)",aux_req->stype);
				DBGRET_FAILURE(MOD_REQBUF);
			}
			break;
		default:
			dbgprint(MOD_REQBUF,__func__,"invalid or unsupported reqbuf type (%d)",type);
			DBGRET_FAILURE(MOD_REQBUF);
	}

	dbgprint(MOD_REQBUF,__func__,"request is incomplete");
	*req_size = 0;
	dbgprint(MOD_REQBUF,__func__,"updated req_size value to %u",*req_size);
	DBGRET_SUCCESS(MOD_REQBUF);
	
end_of_request:
	dbgprint(MOD_REQBUF,__func__,"request is complete");
	*req_size = i;
	dbgprint(MOD_REQBUF,__func__,"updated req_size value to %u",*req_size);
	DBGRET_SUCCESS(MOD_REQBUF);
}

/*
   reqbuf_read

   Function that actually reads a request from the request buffer. If the buffer doesn't
   contain any request, it calls the read_cb for reading more data until a full request
   is found inside the buffer. When a request is found it is taken from the buffer and
   all the data is shifted, buffer_used is updated.

   The reqbuf type here is important because the type of reqbuf determines the way
   this function detects the requests in the buffer.
*/
wstatus
reqbuf_read(reqbuf_t rb,request_t *req)
{
	wstatus ws;
	request_t new_req;
	void *req_ptr;
	unsigned int req_size;
	bool complete_flag;
	unsigned int chunk_used, i, j;

	dbgprint(MOD_REQBUF,__func__,"called with rb=%p, req=%p",rb,req);

	for(;;)
	{
		if( rb->buffer_used )
	   	{
			/* process data inside buffer.. */
			req_ptr = rb->buffer_ptr;
			ws = _reqbuf_req_is_complete(req_ptr,rb->buffer_used,rb->type,&complete_flag);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_REQBUF,__func__,"unable to use _reqbuf_req_is_complete on rb=%p",rb);
				goto return_fail;
			}
			dbgprint(MOD_REQBUF,__func__,"request complete flag is %d",complete_flag);

			if( complete_flag != true ) {
				/* try to get more data and come back later */
				goto get_more_data;
			}

			ws = _reqbuf_find_req_size(req_ptr,rb->buffer_used,rb->type,&req_size);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_REQBUF,__func__,"unable to find request size on rb=%p",rb);
				goto return_fail;
			}
			dbgprint(MOD_REQBUF,__func__,"detected request size as %u bytes long",req_size);

			if( rb->type == REQBUF_TYPE_TEXT )
				goto req_type_text;
			else if( rb->type == REQBUF_TYPE_BINARY )
				goto req_type_bin;

			dbgprint(MOD_REQBUF,__func__,"invalid or unsupported request buffer type (%d)",rb->type);
			goto return_fail;

req_type_text:
			ws = req_from_string(req_ptr,&new_req);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_REQBUF,__func__,"unable to create text request "
						"(_req_from_string failed, ws=%s)",wstatus_str(ws));
				goto return_fail;
			}
			dbgprint(MOD_REQBUF,__func__,"created text request successfully (ptr=%p)",new_req);
			*req = new_req;
			dbgprint(MOD_REQBUF,__func__,"updated req argument value to %p",*req);

			goto shift_chunk;

req_type_bin:

			/* if the request is in binary form, it means it is actually the data structure,
			   simply allocate and copy the memory. */
			new_req = (request_t)malloc(req_size);
			if( !new_req ) {
				dbgprint(MOD_REQBUF,__func__,"malloc failed (size=%u)",req_size);
				goto return_fail;
			}
			dbgprint(MOD_REQBUF,__func__,"allocated request data structure successfully (ptr=%p)",new_req);
			memcpy(new_req,req_ptr,req_size);
			dbgprint(MOD_REQBUF,__func__,"copied request data into new request data structure OK");

			*req = new_req;
			dbgprint(MOD_REQBUF,__func__,"updated req argument value to %p",*req);

shift_chunk:
			/* now we need to shift the data, if the req_size == buffer_used means the
			   buffer contained only one request and nothing more, we can skip the shifting
			   in that case. */

			if( req_size == rb->buffer_used )
				goto skip_shift;
			
			/* shift all the remaining bytes to the beginning of the buffer */
			for( i = req_size, j = 0 ; i < rb->buffer_used ; i++, j++ )
				((char*)rb->buffer_ptr)[j] = ((char*)rb->buffer_ptr)[i];

skip_shift:
			rb->buffer_used -= req_size;
			DBGRET_SUCCESS(MOD_REQBUF);
		}

get_more_data:
		if( rb->buffer_used == rb->buffer_size )
	   	{
			dbgprint(MOD_REQBUF,__func__,"increasing buffer size from %u to %u",
					rb->buffer_size, rb->buffer_size + REQBUF_INC_SIZE );

			/* increase buffer size */
			rb->buffer_ptr = realloc(rb->buffer_ptr,rb->buffer_size + REQBUF_INC_SIZE);
			if( ! rb->buffer_ptr ) {
				dbgprint(MOD_REQBUF,__func__,"realloc failed (size=%u)",rb->buffer_size + REQBUF_INC_SIZE);
				goto return_fail;
			}
		}
		dbgprint(MOD_REQBUF,__func__,"buffer is %u bytes long, with %u bytes free",rb->buffer_size,
				rb->buffer_size - rb->buffer_used);

		assert(rb->read_cb != 0);
		ws = rb->read_cb(rb->param,(char*)rb->buffer_ptr + rb->buffer_used,
				rb->buffer_size - rb->buffer_used, &chunk_used);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_REQBUF,__func__,"read_cb (%p) failed (ws=%s)",rb->read_cb,ws);
			goto return_fail;
		}
		dbgprint(MOD_REQBUF,__func__,"read more %u bytes successfully",chunk_used);
		if( chunk_used > (rb->buffer_size - rb->buffer_used) ) {
			dbgprint(MOD_REQBUF,__func__,"number of bytes read is above buffer free space, check your read_cb code");
			goto return_fail;
		}
		rb->buffer_used += chunk_used;
		dbgprint(MOD_REQBUF,__func__,"new buffer usage value is %u, buffer has %u free bytes now",
				rb->buffer_used, rb->buffer_size - rb->buffer_used);
		continue;
	}

	DBGRET_SUCCESS(MOD_REQBUF);
return_fail:
	DBGRET_FAILURE(MOD_REQBUF);
}

wstatus
reqbuf_status(reqbuf_t rb,reqbuf_status_t *rb_status)
{
	dbgprint(MOD_REQBUF,__func__,"called with rb=%p, rb_status=%p",rb,rb_status);

	if( !rb ) {
		dbgprint(MOD_REQBUF,__func__,"invalid rb argument (rb=0)");
		DBGRET_FAILURE(MOD_REQBUF);
	}

	if( !rb_status ) {
		dbgprint(MOD_REQBUF,__func__,"invalid rb_status argument (rb_status=0)");
		DBGRET_FAILURE(MOD_REQBUF);
	}

	rb_status->buffer_size = rb->buffer_size;
	rb_status->buffer_used = rb->buffer_used;
	dbgprint(MOD_REQBUF,__func__,"filled rb_status successfully (buffer_size=%u, buffer_used=%u)",
			rb_status->buffer_size, rb_status->buffer_used);

	DBGRET_SUCCESS(MOD_REQBUF);
}

wstatus
reqbuf_destroy(reqbuf_t rb)
{
	dbgprint(MOD_REQBUF,__func__,"called with rb=%p",rb);

	if( !rb ) {
		dbgprint(MOD_REQBUF,__func__,"invalid rb argument (rb=0)");
		DBGRET_FAILURE(MOD_REQBUF);
	}

	if( rb->buffer_ptr ) {
		dbgprint(MOD_REQBUF,__func__,"freeing allocated buffer ptr=%p",rb->buffer_ptr);
		free(rb->buffer_ptr);
		rb->buffer_ptr = 0;
	}

	dbgprint(MOD_REQBUF,__func__,"freeing request buffer data structure rb=%p",rb);
	free(rb);

	DBGRET_SUCCESS(MOD_REQBUF);
}

