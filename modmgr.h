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
   Module Manager - Implementation

   The objects this module will have to deal are:
    - requests
	- wchannels
	- modules
	- request buffer

   When the module loads, it creates a thread which will receive
   and process the requests. It will also create another thread
   which will create a wchannel which will receive remote requests.

   _request_processor
		1 create fast wchannel (reqchannel) for reception of binary requests
		2 wait for wchannel
		3 request received, send it to correct module if its found.
		4 goto 2

   _remote_request_receiver
		1 create UDP wchannel, binded to wicom.modmgr_bind_port
		2 wait for wchannel
		3 convert request.str to request.pipe
		4 send request to reqchannel
		5 goto 2

   modmgr_mod_request
		1 convert request from request.bin to request.pipe
		2 send request to reqchannel
		3 return

   To reduce the complexity caused by multithreading and shared
   objects, the idea is to use a pipe or unix socket type channel
   to inject requests into the receiving thread. After the request
   is converted to binary, its send through the pipe. When a call
   is made (DCR) to modmgr_mod_request the binary request is send
   through the pipe. This pipe is being read by the request processing
   thread.
  
   Helper functions:

   Requests can be in three forms, helper functions to convert between
   these forms will be handy.

    _req_to_pipe(req)
	_req_to_text(req)
	_req_to_bin(req)
	_req_build_pipe(from,to,nvpairs)
	_req_build_text(from,to,nvpairs)
	_req_build_bin(from,to,nvpairs)
	_req_dump(req)
	_req_lookup_src(req) = src
	_req_lookup_dst(req) = dst
	_nvp_lookup_nv(req,name) = value
	_nvp_insert_nv(req,name,value)
	_nvp_remove_nv(req,name,value)

	Request data structures:
	request_t
	  - type = {bin, text, pipe}
	  - data_size
	  - data[] = union
	  - reply_lock (lock for waiting for reply)
	  - reply_nvl (name value list for reply)
	

   Each module data structure should contain the following informations:
	 - name
	 - communication type (DCR, SSR)
	 - communication data (host, port, callback address)
	 - description
	 - version
	 - dependency list
   Module data structure helper functions:

	_mod_alloc
	_mod_free
	_mod_

   Procedures from actions:

	i) mod_request(from,to,nvpairs)
	    1) build pipe request from (from,to,nvpairs) arguments
		2) send request to request processor pipe.
		3) wait for req.reply_lock
		4) if source.DCR, call source.callback(to,from,req.reply_nvl)
		5) if source.SSR, build text request using (to,from,req.reply_nvl)
	       send request to source.host, source.port using a modmgr wch
		   (modmgr request sender channel, common to all modules).
		6) request was sent, or not, free req.
		7) return

	ii) _request_processor()
		This thread is always running and waiting for pipe channel where
		requests are injected. Its responsible for processing the requests,
		which usually requires forwarding the request to the right module
		and waiting for the answer. If some error occurs before the request
		is forward, it should build a reply with error information and send
		the reply to the pipe.
		1) pipe returns data
		x) insert data into pipe req_buffer
		x) got any request (req-start to req-end)? get it from req_buffer
		2) lookup destiny module data structure
		3.1) if mod_dst.DCR, convert req to req_bin, send request dst.callback
		3.2) if mod_dst.SSR, convert req to req_text, send request to
		     mod_dst.host, mod_dst.port
		4) goto 1)

	iv) _remote_request_receiver()
		This thread receives data from the modmgr socket (UDP channel listening)
		and sends the request to request processor pipe.
		1) read from wchannel
		2) got data, insert into socket req_buffer
		3) got any request (req-start to req-end)? get it from req_buffer
		4) convert request to req_pipe
		5) insert request into request processor pipe
		6) goto 1)


	Difference between requests and replies: each request has a type associated,
	it can be request type and reply type (future might bring other types also).
	In the context of modmgr forwarder, replies which are to be forwarded and
	fail, don't return any error to the sender. In case of requests being send
	and because of some reason fail (destiny module is not loaded for example)
	then modmgr is able to send a reply with error information to that request.
	This is one case where modmgr is the one making the reply instead of the real
	destiny module.

	For now modmgr will not keep any information of requests traffic, although
	in future it might be a good idea to add an internal list of requests with
	no reply so that they can be timed out and the caller would receive an error
	message with TIMEOUT of the request. With the use of the request processor
	thread were all the requests and reply pass, it will be more easy to add that
	functionality to modmgr.

	Request Buffer is an useful data structure that will allow requests to fragment.
	Its not quaranted that a single request comes in a single UDP packet or PIPE read,
	also the request might come fragmented in two UDP packets. The implementation of
	this data structure uses a memory buffer that might grow if all space is used.
	It will not, however, shrink the buffer.

	request_buffer functions:
		_reqbuf_create(reqbuf_t *rb,type = {pipe, text})
		_reqbuf_insert(reqbuf_t rb,void *data_ptr,unsigned int data_size)
		_reqbuf_read_request(reqbuf_t rb,req_bin)
		_reqbuf_destroy(reqbuf_t rb)

	request buffer data structure:
		reqbuf_t
			- void *data_ptr;
			- uint data_size; // = bytes of buffer used
			- void *buffer_ptr; // normally is equal to data_ptr...
			- uint buffer_cap; // maximum bytes the buffer can old
	
	
*/

#ifndef _MODMGR_H
#define _MODMGR_H

#include "wstatus.h"
#include "debug.h"
#include "jmlist.h"
#include "wlock.h"
#include "nvpair.h"
#include "req.h"

typedef struct _modmgr_load_t {
	char *bind_hostname;
	char *bind_port;
} modmgr_load_t;

/*
 * MODULE REGISTRATION DECLARATIONS
 */

#define MODNAMESIZE REQMODSIZE
#define MODDESCSIZE 256
#define MODVERSIONSIZE 32
#define MODAUTHORSIZE 128
#define MODEMAILSIZE 128
#define MODHOSTSIZE 128
#define MODPORTSIZE 32

typedef enum _modreg_comm_type_list {
	MODREG_COMM_UNDEF,
	MODREG_COMM_DCR,
	MODREG_COMM_SSR
} modreg_comm_type_list;

typedef void (*REQPROCESSORCALLBACK)(const request_t req);

typedef struct _modreg_t {
	struct _basic {
		char name[MODNAMESIZE];
		char description[MODDESCSIZE];
		char version[MODVERSIONSIZE];
		char author_name[MODAUTHORSIZE];
		char author_email[MODEMAILSIZE];
	} basic;
	struct _communication {
		modreg_comm_type_list type;
		union _xpto {
			struct _dcr {
				REQPROCESSORCALLBACK reqproc_cb;
			} dcr;
			struct _ssr {
				char host[MODHOSTSIZE];
				char port[MODPORTSIZE];
			} ssr;
		} data;
	} communication;
} *modreg_t;

typedef enum _modreg_validation_result {
	MODREG_PARAM_INVALID_CHARACTER_FOUND,
	MODREG_PARAM_CHARACTERS_ARE_VALID
} modreg_validation_result;

#define modreg_val_str(x) ( x == MODREG_PARAM_INVALID_CHARACTER_FOUND ? "MODREG_PARAM_INVALID_CHARACTER_FOUND" : "MODREG_PARAM_CHARACTERS_ARE_VALID" )

#define MRV_NAMECHAR(x) (V_MODCHAR(x))
#define MRV_DESCCHAR(x) (V_QVALUECHAR(x))
#define MRV_VERSIONCHAR(x) (V_QVALUECHAR(x))
#define MRV_AUTHORNCHAR(x) (V_QVALUECHAR(x))
#define MRV_AUTHORECHAR(x) (V_QVALUECHAR(x))

/*
 * PUBLIC FUNCTIONS AVAILABLE FROM MODMGR
 */

wstatus modmgr_lookup(const char *mod_name,const struct _modreg_t **modp);
wstatus modmgr_load(modmgr_load_t load);
wstatus modmgr_unload(void);

wstatus _request_send(const request_t req,const struct _modreg_t *mod);

#endif

