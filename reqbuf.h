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
   Request Buffer - Implementation

   Requests can be exchanged using specific mediums which includes
   sockets. The unity of reception is normally byte sized meaning
   whenever one uses IO functions of file descriptors it doesn't mean
   it receives the full message / data structure in one read.

   This means the data exchange might be fragmented and its where
   the request buffer commes in. Its simply a buffer between the
   wchannel used for data transport and the code that understands
   only full requests.

   Request buffers should be easy to use, the module has the
   following functions exported:

   reqbuf_load(..)		initialize the module
	arguments:
		- parameters of initialization of this module.
	returns:
		- nothing.

   reqbuf_unload(..)	deinitialize the module
	arguments:
		- none.
	returns:
		- nothing.

   reqbuf_create()		creates a request buffer
	arguments:
		- callback function used whenever reqbuf needs to read data.
		- opaque parameter (void*) that will be passed to callback.
	returns:
		- reqbuf_t opaque data structure pointer.

   reqbuf_read(req)		reads a single request
    arguments:
		- reqbuf_t opaque data structure pointer which was returned
		by the reqbuf_create function.
		- request_t data structure pointer. this will be allocated
		by the reqbuf_read function and must be freed by the caller.
	returns:
		- fills the request_t pointer in case of success, returns
		failure otherwise.

   reqbuf_status()		returns the status of the reqbuffer
    arguments:
		- reqbuf_t opaque data structure pointer which was returned
		by the reqbuf_create function.
		- reqbuf_status_t data structure pointer to be filled.
	returns:
		- reqbuf_status_t data structure filled in case of success,
		otherwise returns failure.

   reqbuf_destroy()		destroys the request buffer.
    arguments:
		- reqbuf_t opaque data structure pointer which was returned
		by the reqbuf_create function.
	returns:
		- nothing.

   Each request buffer data structure has a memory block associated
   which is allocated initially and might grow depending on the
   request sizes.

   There might exist default callback functions defined in reqbuffer
   module for reception of data. For instance,
   reqbuf_wchannel_read_cb which reads directly from one wchannel.
   Remember that the param (void*) passed in reqbuf creation will
   be passed to these callback functions, in case of the wchannel
   callback, the opaque pointer actually is a wchannel_t data
   structure pointer.

   Usage description is:
   1) create the request buffer which has a param and read
      callback associated.

   2) read the buffer using reqbuf_read, this function will block
      until a full request data structure is received.
      To read data it will call the callback defined during the reqbuf
      creation and pass the opaque param pointer (void*) to it also.

   3) reqbuf_read is the only function that actually reads from
      the wchannel. No other function uses the wchannel.

   4) reqbuf_status might be called to know for example the size
      of the memory block and if there's any bytes used (meaning
	  that a fragmented request is there).

   5) when the reqbuf is not needed anymore, use reqbuf_destroy
      to free any data structure associated to it. If the buffer
	  contains data, they will be discarded.
*/

#ifndef _REQBUF_H
#define _REQBUF_H

#include "modmgr.h"
#include "wstatus.h"

#define REQBUF_INIT_SIZE 1024
#define REQBUF_INC_SIZE 512

typedef struct _reqbuf_t *reqbuf_t;

typedef struct _reqbuf_load_t {
	void *junk;
} reqbuf_load_t;

typedef struct _reqbuf_status_t {
	unsigned int buffer_size;
	unsigned int buffer_used;
} reqbuf_status_t;


typedef enum _reqbuf_type_list
{
	REQBUF_TYPE_TEXT,
	REQBUF_TYPE_BINARY
} reqbuf_type_list;

typedef wstatus (*REQBUFREADCB)(void *param,void *chunk_ptr,unsigned int chunk_size,unsigned int *chunk_used);

wstatus reqbuf_load(reqbuf_load_t *load);
wstatus reqbuf_unload(void);
wstatus reqbuf_create(REQBUFREADCB read_cb,void *param,reqbuf_type_list type,reqbuf_t *rb);
wstatus reqbuf_read(reqbuf_t rb,request_t *req);
wstatus reqbuf_status(reqbuf_t rb,reqbuf_status_t *rb_status);
wstatus reqbuf_destroy(reqbuf_t rb);

#endif

