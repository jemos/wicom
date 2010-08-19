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

#define MAXREQID 65535

#define REQIDSIZE (sizeof("65535")-1)
#define REQTYPESIZE 1
#define REQMODSIZE 64
#define REQCODESIZE 64

#ifdef req
#error AAAAAAAA
#endif

typedef enum _request_stype_list {
	REQUEST_STYPE_BIN,
	REQUEST_STYPE_PIPE,
	REQUEST_STYPE_TEXT
} request_stype_list;

typedef enum _request_type_list {
	REQUEST_TYPE_REQUEST,
	REQUEST_TYPE_REPLY
} request_type_list;

typedef enum _value_format_list {
	VALUE_FORMAT_UNQUOTED = 0,
	VALUE_FORMAT_QUOTED = 1,
	VALUE_FORMAT_ENCODED = 2
} value_format_list;

/* nvpair_t: this data structure must have well defined sizes */
typedef struct _nvpair_t
{
	char *name_ptr;
	uint16_t name_size;
	void *value_ptr;
	uint16_t value_size;
} *nvpair_t;

typedef struct _req_data_bin {
	request_type_list type;
	int id;
	char src[REQMODSIZE];
	char dst[REQMODSIZE];
	char code[REQCODESIZE];
	jmlist nvl;
} req_data_bin;

/* req_data_pipe: this data structure must have well defined sizes */
typedef struct _req_data_pipe {
	uint16_t nvp_number;
	struct _nvpair_t nvp_data[1];
} req_data_pipe;

typedef struct _req_data_text {
	char raw[1];
} req_data_text;

typedef struct _request_t {
	request_stype_list stype;
	unsigned int data_size;
	wlock_t reply_lock;
	jmlist reply_nvl;
	union _data {
		req_data_bin bin;
		req_data_pipe pipe;
		req_data_text text;
	} data;
	/* don't add fields below data.. */
} *request_t;

#define rtype_str(x) (x == REQUEST_TYPE_REQUEST ? "REQUEST" : "REPLY")
#define V_RIDCHAR(x) isdigit(x)
#define V_MODCHAR(x) isalnum(x)
#define V_CODECHAR(x) (isalnum(x) || (x == '.') || (x == '_') || (x == '-'))
#define V_TOKSEPCHAR(x) (x == ' ')
#define V_REPLYCHAR(x) ( (x == 'R') || (x == 'r') )
#define V_NAMECHAR(x) isalnum(x)
#define V_ENCPREFIX(x) (x == '#')
#define V_NVSEPCHAR(x) (x == '=')
#define V_REQENDCHAR(x) (x == '\0')
#define V_VALUECHAR(x) (isalnum(x) || (x == '.') || (x == ':') || (x == '_') || (x == '-')) 
#define V_QVALUECHAR(x) (V_VALUECHAR(x) || (x == ' '))
#define V_EVALUECHAR(x) (isdigit(x) || (x == 'A') || (x == 'B') || (x == 'C') || (x == 'D') || (x == 'E') || (x == 'F'))
#define V_QUOTECHAR(x) (x == '"')

/*
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
*/

typedef enum _request_lookup_result {
	REQUEST_NV_FOUND,
	REQUEST_NV_NOT_FOUND
} request_lookup_result;

typedef enum _request_validate_result {
	REQUEST_VALID,
	REQUEST_INVALID_NAME,
	REQUEST_INVALID_VALUE
} request_validate_result;

wstatus _nvp_alloc(uint16_t name_size,uint16_t value_size,nvpair_t *nvp);
wstatus _nvp_fill(char *name_ptr,uint16_t name_size,void *value_ptr,uint16_t value_size,nvpair_t nvp);
wstatus _nvp_free(nvpair_t nvp);
wstatus _nvp_value_format(const char *value_ptr,const unsigned int value_size,value_format_list *value_format);
wstatus _nvp_value_encode(const char *value_ptr,const unsigned int value_size,char **value_encoded);
wstatus _nvp_value_encoded_size(const char *value_ptr,const unsigned int value_size,unsigned int *encoded_size);


wstatus _req_to_pipe(request_t req,request_t *req_pipe);
wstatus _req_to_text(request_t req,request_t *req_text);
wstatus _req_to_bin(request_t req,request_t *req_bin);
wstatus _req_build_bin(char *src,char *dst,request_type_list type,int id,jmlist nvl,request_t *req);
wstatus _req_dump(request_t req);
wstatus _req_lookup_src(request_t req,char *src);
wstatus _req_lookup_dst(request_t req,char *dst);
wstatus _req_lookup_id(request_t req,uint16_t *id);
wstatus _req_lookup_type(request_t req,request_type_list *type);
wstatus _req_lookup_nv(request_t req,char *value_ptr,int value_size,request_lookup_result *lookup_result);
wstatus _req_validate_nv(request_t req,char *name,char *value,request_validate_result *validate_result);
wstatus _req_insert_nv(request_t req,char *name,char *value);
wstatus _req_remove_nv(request_t req,char *name);
wstatus _req_dump(request_t req);
wstatus _req_diff(request_t req1_ptr,char *req1_label,request_t req2,char *req2_label);
wstatus _req_from_string(const char *raw_text,request_t *req_text);


#ifndef MAX
#define MAX(a,b) (a > b ? a : b)
#endif

#ifndef MIN
#define MIN(a,b) (a < b ? a : b)
#endif

#endif
