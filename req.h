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
#define V_NAMECHAR(x) isalnum(x)
#define V_ENCPREFIX(x) (x == NVP_ENCODED_PREFIX)
#define V_REQENDCHAR(x) (x == '\0')

#define REQERROR_DESCNAME "errorMsg"
#define REQERROR_MODUNFOUND "Destination module %s was not found in modmgr module list."

typedef enum _request_lookup_result {
	REQUEST_NV_FOUND,
	REQUEST_NV_NOT_FOUND
} request_lookup_result;

typedef enum _request_validate_result {
	REQUEST_VALID,
	REQUEST_INVALID_NAME,
	REQUEST_INVALID_VALUE
} request_validate_result;

wstatus _req_from_text_to_bin(request_t req,request_t *req_bin);
wstatus _req_from_pipe_to_bin(request_t req,request_t *req_bin);
wstatus _req_nv_value_info(char *value_ptr,char **value_start,char **value_end,uint16_t *value_size);
wstatus _req_nv_name_info(char *name_ptr,char **name_end,uint16_t *name_size);
wstatus _req_text_token_seek(const char *req_text,text_token_t token,char **token_ptr);
wstatus _req_text_nv_parse(char *nv_ptr,char **name_start,unsigned int *name_size,char **value_start,unsigned int *value_size,value_format_list *value_format);
wstatus _req_text_nv_validate(const char *nv_ptr,nvpair_info_flag_list *nvpi_flags);
wstatus _req_text_get_nv(const request_t req,const char *look_name_ptr,unsigned int look_name_size,nvpair_t *nvpp);
wstatus _req_text_get_nv_info(const struct _request_t *req,const char *look_name_ptr,unsigned int look_name_size,nvpair_info_t nvpi);
wstatus _req_from_bin_to_text(request_t req,request_t *req_text);
wstatus _req_from_pipe_to_text(request_t req,request_t *req_text);
wstatus _req_bin_get_nv(const request_t req,const char *look_name_ptr,unsigned int look_name_size,nvpair_t *nvpp);
wstatus _req_bin_get_nv_info(const struct _request_t *req,const char *look_name_ptr,unsigned int look_name_size,nvpair_info_t nvpi);
wstatus _req_add_nvp_z(const char *name_ptr,const char *value_ptr,request_t req);

wstatus _req_to_pipe(request_t req,request_t *req_pipe);
wstatus _req_to_text(request_t req,request_t *req_text);
wstatus _req_to_bin(request_t req,request_t *req_bin);
wstatus _req_build_bin(char *src,char *dst,request_type_list type,int id,jmlist nvl,request_t *req);
wstatus _req_dump(request_t req);
wstatus _req_lookup_src(request_t req,char *src);
wstatus _req_lookup_dst(request_t req,char *dst);
wstatus _req_lookup_id(request_t req,uint16_t *id);
wstatus _req_lookup_type(request_t req,request_type_list *type);
wstatus _req_get_nv(const request_t req,const char *name_ptr,unsigned int name_size,nvpair_t *nvpp);
wstatus _req_get_nv_count(const struct _request_t *req,unsigned int *nv_count);
wstatus _req_get_nv_info(const struct _request_t *req,const char *look_name_ptr,unsigned int look_name_size,nvpair_info_t nvpi);
wstatus _req_lookup_nv(const request_t req,const char *name_ptr,unsigned int name_size,nvpair_t *nvpp);
wstatus _req_insert_nv(request_t req,char *name,char *value);
wstatus _req_remove_nv(request_t req,char *name);
wstatus _req_dump(request_t req);
wstatus _req_diff(request_t req1_ptr,char *req1_label,request_t req2,char *req2_label);
wstatus _req_from_string(const char *raw_text,request_t *req_text);
wstatus _req_free(request_t req);

#endif

