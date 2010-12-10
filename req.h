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
   Description: set of functions related to requests, most of the functions
   are private. There are functions to allocate, modify and free the
   request data.

*/

#ifndef _REQUEST_H
#define _REQUEST_H

#include "wstatus.h"
#include "debug.h"
#include "jmlist.h"
#include "wlock.h"
#include "nvpair.h"

#ifndef MAX
#define MAX(a,b) (a > b ? a : b)
#endif

#ifndef MIN
#define MIN(a,b) (a < b ? a : b)
#endif

#define MAXREQID 65535

#define REQIDSIZE (sizeof("65535")-1)
#define REQTYPESIZE 1
#define REQMODSIZE 64
#define REQCODESIZE 64

typedef enum _request_stype_list {
	REQUEST_STYPE_BIN,
	REQUEST_STYPE_PIPE,
	REQUEST_STYPE_TEXT
} request_stype_list;

typedef enum _request_type_list {
	REQUEST_TYPE_REQUEST,
	REQUEST_TYPE_REPLY
} request_type_list;

typedef enum _text_token_t {
	TEXT_TOKEN_ID,
	TEXT_TOKEN_TYPE,
	TEXT_TOKEN_MODSRC,
	TEXT_TOKEN_MODDST,
	TEXT_TOKEN_CODE,
	TEXT_TOKEN_NVL
} text_token_t;

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
#define V_REQENDCHAR(x) (x == '\0')

#define REQERROR_DESCNAME "errorMsg"
#define REQERROR_MODUNFOUND "Destination module %s was not found in modmgr module list."

typedef enum _request_lookup_result {
	REQUEST_NV_FOUND,
	REQUEST_NV_NOT_FOUND
} request_lookup_result;

typedef enum _req_validation_t {
	REQUEST_VALIDATION_UNDEF,
	REQUEST_IS_VALID,
	REQUEST_INVALID_SEP,
	REQUEST_INVALID_ID,
	REQUEST_INVALID_TYPE,
	REQUEST_INVALID_MODSRC,
	REQUEST_INVALID_MODDST,
	REQUEST_INVALID_CODE,
	REQUEST_INVALID_NV,
	REQUEST_INVALID_END
} req_validation_t;

typedef enum _nv_status_t {
	NV_STATUS_UNDEF,
	NV_IS_VALID,
	NV_INVALID_NAME,
	NV_INVALID_VALUE,
	NV_INVALID_END,
	NV_INVALID_SEP
} nv_status_t;

typedef enum _token_status_t {
	TOKEN_IS_VALID,
	TOKEN_IS_INVALID
} token_status_t;

/* internal functions */
wstatus _req_from_text_to_bin(request_t req,request_t *req_bin);
wstatus _req_from_pipe_to_bin(request_t req,request_t *req_bin);
wstatus _req_nv_value_info(char *value_ptr,char **value_start,char **value_end,uint16_t *value_size);
wstatus _req_nv_name_info(char *name_ptr,char **name_end,uint16_t *name_size);
wstatus _req_text_token_seek(const char *req_text,text_token_t token,char **token_ptr);
wstatus _req_text_nv_parse(char *nv_ptr,char **name_start,unsigned int *name_size,char **value_start,unsigned int *value_size,nvpair_fflag_list *fflags);
wstatus _req_text_nv_validate(const char *nv_ptr,nvpair_iflag_list *iflags);
wstatus _req_text_get_nv(const request_t req,const char *look_name_ptr,unsigned int look_name_size,nvpair_t *nvpp);
wstatus _req_text_get_nv_info(const struct _request_t *req,const char *look_name_ptr,unsigned int look_name_size,nvpair_info_t nvpi);
wstatus _req_from_bin_to_text(request_t req,request_t *req_text);
wstatus _req_from_pipe_to_text(request_t req,request_t *req_text);
wstatus _req_bin_get_nv(const request_t req,const char *look_name_ptr,unsigned int look_name_size,nvpair_t *nvpp);
wstatus _req_bin_get_nv_info(const struct _request_t *req,const char *look_name_ptr,unsigned int look_name_size,nvpair_info_t nvpi);
wstatus _req_validate_id(char **token_ptr,const unsigned int token_max_size,token_status_t *token_status);
wstatus _req_validate_type(char **token_ptr,const unsigned int token_max_size,token_status_t *token_status);
wstatus _req_validate_mod(char **token_ptr,const unsigned int token_max_size,token_status_t *token_status);
wstatus _req_validate_code(char **token_ptr,const unsigned int token_max_size,token_status_t *token_status);
wstatus _req_validate_nv(char **nv_ptr,const unsigned int nv_max_size,nv_status_t *nvpair_status);

/* functions that modify existing request */
wstatus req_insert_nv(request_t req,char *name,char *value);
wstatus req_add_nvp_z(const char *name_ptr,const char *value_ptr,request_t req);
wstatus req_remove_nv(request_t req,char *name);

/* functions to get informations/data from the request */
wstatus req_get_nv(const request_t req,const char *name_ptr,unsigned int name_size,nvpair_t *nvpp);
wstatus req_get_nv_count(const struct _request_t *req,unsigned int *nv_count);
wstatus req_get_nv_info(const struct _request_t *req,const char *look_name_ptr,unsigned int look_name_size,nvpair_info_t nvpi);
wstatus req_validate(const char *req_text,const unsigned int req_size,req_validation_t *req_validation);

/* functions that output new request data structure */
wstatus req_from_string(const char *raw_text,request_t *req_text);
wstatus req_to_text(request_t req,request_t *req_text);
wstatus req_to_bin(request_t req,request_t *req_bin);

/* clean up functions */
wstatus req_free(request_t req);

/* debugging functions */
wstatus req_dump(request_t req);
wstatus req_diff(request_t req1_ptr,char *req1_label,request_t req2,char *req2_label);
const char* req_validate_str(req_validation_t req_validation);

#endif

