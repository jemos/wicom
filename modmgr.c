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
	ISSUES:
	
		- the unloading procedure is not totally thread safe, I'm using
		  an unloading flag (bool) for now but this is not thread-safe.

*/
#include "wstatus.h"
#include "debug.h"
#include "modmgr.h"
#include "string.h"
#include "stdlib.h"
#include "jmlist.h"
#include <ctype.h>
#include <assert.h>
#include "wchannel.h"
#include "wthread.h"
#include "reqbuf.h"

typedef enum _text_token_t {
	TEXT_TOKEN_ID,
	TEXT_TOKEN_TYPE,
	TEXT_TOKEN_MODSRC,
	TEXT_TOKEN_MODDST,
	TEXT_TOKEN_CODE,
	TEXT_TOKEN_NVL
} text_token_t;

/*
   This structure contains interface objects used between the
   request processor thread and the modmgr module.
*/
typedef struct _request_proc_data_t
{
	bool unload_flag;
	bool initialized_flag;
	bool finished_flag;
	wstatus ret_status;
	wchannel_t recv_wch;
	wthread_t wthread;
} request_proc_data_t;

extern wstatus reqbuf_wchannel_read_cb(void *param,void *chunk_ptr,unsigned int chunk_size, unsigned int *chunk_used);

wstatus _req_from_text_to_bin(request_t req,request_t *req_bin);
wstatus _req_from_pipe_to_bin(request_t req,request_t *req_bin);
wstatus _req_nv_value_info(char *value_ptr,char **value_start,char **value_end,uint16_t *value_size);
wstatus _req_nv_name_info(char *name_ptr,char **name_end,uint16_t *name_size);
wstatus _req_text_token_seek(const char *req_text,text_token_t token,char **token_ptr);
wstatus _req_text_nv_parse(char *nv_ptr,char **name_start,unsigned int *name_size,char **value_start,unsigned int *value_size,value_format_list *value_format);
wstatus _req_text_get_nv(const request_t req,const char *look_name_ptr,unsigned int look_name_size,nvpair_t *nvpp);
wstatus _req_from_bin_to_text(request_t req,request_t *req_text);
wstatus _req_from_pipe_to_text(request_t req,request_t *req_text);
wstatus _req_bin_get_nv(const request_t req,const char *look_name_ptr,unsigned int look_name_size,nvpair_t *nvpp);
wstatus _req_add_nvp_z(const char *name_ptr,const char *value_ptr,request_t req);
void _modmgr_reqproc_cb(const request_t req);
wstatus _modreg_alloc(modreg_t *new_mod);
wstatus _modreg_free(const struct _modreg_t *mod);

wstatus _nvp_dup(const nvpair_t nvp,nvpair_t *new_nvp);

/* this module variables */
static jmlist mod_list = 0; /* modreg_t */
static request_proc_data_t thread_reqproc_data;
static bool unloading = false;
static bool loaded = false;

wstatus
_nvp_alloc(uint16_t name_size,uint16_t value_size,nvpair_t *nvp)
{
	nvpair_t new_nvp;

	dbgprint(MOD_MODMGR,__func__,"called with name_size=%u, value_size=%u, nvp=%p",
			name_size,value_size,nvp);

	if( !name_size ) {
		dbgprint(MOD_MODMGR,__func__,"invalid argument name_size, size > 0 is required");
		goto return_fail;
	}

	if( !nvp ) {
		dbgprint(MOD_MODMGR,__func__,"invalid argument nvp (nvp=0)");
		goto return_fail;
	}

	/* got everything we need to allocate the new nvp */

	new_nvp = (nvpair_t) malloc(sizeof(struct _nvpair_t));
	if( !new_nvp ) {
		dbgprint(MOD_MODMGR,__func__,"malloc failed");
		goto return_fail;
	}

	/* initialize stucture */

	new_nvp->name_ptr = 0;
	new_nvp->name_size = 0;
	new_nvp->value_ptr = 0;
	new_nvp->value_size = 0;

	new_nvp->name_ptr = (char*)malloc(name_size);
	if( !new_nvp->name_ptr ) {
		dbgprint(MOD_MODMGR,__func__,"malloc failed");
		goto return_fail_malloc;
	}
	dbgprint(MOD_MODMGR,__func__,"allocated %u bytes for name of nvpair=%p",
			name_size,new_nvp);

	new_nvp->name_size = name_size;
	dbgprint(MOD_MODMGR,__func__,"changed nvpair (%p) name size to %u",
			new_nvp,new_nvp->name_size);

	if( value_size )
	{
		new_nvp->value_ptr = malloc(value_size);
		if( !new_nvp->value_ptr ) {
			dbgprint(MOD_MODMGR,__func__,"malloc failed");
			goto return_fail_malloc;
		}
		dbgprint(MOD_MODMGR,__func__,"allocated %d bytes for value of nvpair=%p",
				value_size,new_nvp);

		new_nvp->value_size = value_size;
		dbgprint(MOD_MODMGR,__func__,"changed nvpair (%p) value size to %u",
				new_nvp,new_nvp->value_size);
	}

	dbgprint(MOD_MODMGR,__func__,"updating argument nvp to %p",nvp);
	*nvp = new_nvp;
	dbgprint(MOD_MODMGR,__func__,"argument nvp updated successfully to %p",*nvp);

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail_malloc:
	if( new_nvp->name_ptr ) {
		free(new_nvp->name_ptr);
	}

	if( new_nvp->value_ptr ) {
		free(new_nvp->value_ptr);
	}

	free(new_nvp);

return_fail:
	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _nvp_fill

   Helper function to fill a name-value pair data structure.
*/
wstatus
_nvp_fill(const char *name_ptr,const uint16_t name_size,const void *value_ptr,const uint16_t value_size,nvpair_t nvp)
{
	char *decoded_ptr = 0;

	dbgprint(MOD_MODMGR,__func__,"called with nvp=%p, name_ptr=%p, name_size=%u, value_ptr=%p, value_size=%u",
			nvp,name_ptr,name_size,value_ptr,value_size);

	if( !nvp ) {
		dbgprint(MOD_MODMGR,__func__,"invalid argument nvp (nvp=0)");
		goto return_fail;
	}

	if( !name_ptr || !name_size ) {
		dbgprint(MOD_MODMGR,__func__,"invalid argument name_ptr or name_size=0");
		goto return_fail;
	}

	/* compare name_size and value_size with the values in nvp */
	
	if( nvp->name_size != name_size ) {
		dbgprint(MOD_MODMGR,__func__,"inconsistent values of name_size (nvp->name_size=%u, name_size=%u)",
				nvp->name_size,name_size);
		goto return_fail;
	}

	if( nvp->value_size != value_size ) {
		dbgprint(MOD_MODMGR,__func__,"inconsistent values of value_size (nvp->value_size=%u, value_size=%u)",
				nvp->value_size,value_size);
		goto return_fail;
	}

	dbgprint(MOD_MODMGR,__func__,"copying %u bytes of the name to nvp structure (%p)",name_size,nvp);
	memcpy(nvp->name_ptr,name_ptr,name_size);
	dbgprint(MOD_MODMGR,__func__,"copied %d bytes successfully of the name to the nvp structure (%p)",name_size,nvp);

	if( !nvp->value_size ) {
		DBGRET_SUCCESS(MOD_MODMGR);
	}

	dbgprint(MOD_MODMGR,__func__,"copying %u bytes of the value to nvp structure (%p)",value_size,nvp);
	memcpy(nvp->value_ptr,value_ptr,value_size);
	dbgprint(MOD_MODMGR,__func__,"copied %d bytes successfully of the value to the nvp structure (%p)",value_size,nvp);

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:
	if( decoded_ptr )
		free(decoded_ptr);

	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _nvp_free

   Helper function to free the name-value pair data structure from memory.
*/
wstatus
_nvp_free(nvpair_t nvp)
{
	dbgprint(MOD_MODMGR,__func__,"called with nvp=%p");

	if( !nvp ) {
		dbgprint(MOD_MODMGR,__func__,"invalid argument nvp (nvp=0)");
		goto return_fail;
	}

	if( nvp->name_ptr ) {
		dbgprint(MOD_MODMGR,__func__,"(nvp=%p) freeing name_ptr=%p",nvp,nvp->name_ptr);
		free(nvp->name_ptr);
	}

	if( nvp->value_ptr ) {
		dbgprint(MOD_MODMGR,__func__,"(nvp=%p) freeing value_ptr=%p",nvp,nvp->value_ptr);
		free(nvp->value_ptr);
	}

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:
	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _req_text_src

   Helper function to get source module from the text request. Request format is:
   <req-id><type> <mod_source> <mod_dest> <req-code-id>|reply-code-id> argNameN=argValueN<NULL>

   It is necessary that src points to a character buffer with length >= (REQMODSIZE+1).
   It is assumed that the request format was validated before calling this function, although
   the function validates the tokens before module source token and also module source token.
*/
wstatus
_req_text_src(request_t req,char *src)
{
	char reqsrc[REQMODSIZE+1];
	char *src_ptr,*aux_ptr;
	int i, j;
	wstatus ws;

	dbgprint(MOD_MODMGR,__func__,"called with req=%p, src=%p",req,src);

	if( !req ) {
		dbgprint(MOD_MODMGR,__func__,"invalid req argument (req=0)");
		goto return_fail;
	}

	if( !src ) {
		dbgprint(MOD_MODMGR,__func__,"invalid src argument (src=0)");
		goto return_fail;
	}

	if( req->stype != REQUEST_STYPE_TEXT ) {
		dbgprint(MOD_MODMGR,__func__,"invalid request to be used with this function");
		goto return_fail;
	}

	ws = _req_text_token_seek(req->data.text.raw,TEXT_TOKEN_MODSRC,&src_ptr);
	if( ws != WSTATUS_SUCCESS ) {
		DBGRET_FAILURE(MOD_MODMGR);
	}

	aux_ptr = src_ptr;
	for( i = 0, j = 0 ; !V_REQENDCHAR(aux_ptr[i]) ; i++ )
	{
		if( V_TOKSEPCHAR(aux_ptr[i]) )
			goto finished_token;

		if( V_MODCHAR(aux_ptr[i]) )
	   	{
			if( j >= REQMODSIZE ) {
				dbgprint(MOD_MODMGR,__func__,"(req=%p) source module name length is overlimit (limit is %d)",req,REQMODSIZE);
				goto return_fail;
			}
			reqsrc[j++] = aux_ptr[i];
			continue;
		}

		/* invalid character detected */
		dbgprint(MOD_MODMGR,__func__,"(req=%p) invalid/unexpected character in MODSRC token (offset %d)",req,i);
		goto return_fail;
	}

	dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to reach MODSRC token end",req);
	goto return_fail;

finished_token:

	reqsrc[j] = '\0';

	dbgprint(MOD_MODMGR,__func__,"(req=%p) got source module name \"%s\"",req,reqsrc);
	strcpy(src,reqsrc);
	dbgprint(MOD_MODMGR,__func__,"(req=%p) wrote source module name into src=%p (%d bytes long)",
			req,src,strlen(reqsrc));

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:
	DBGRET_FAILURE(MOD_MODMGR);
}


/*
   _req_text_code

   Helper function to get req-code/reply-code from the text request. Request format is:
   <req-id><type> <mod_source> <mod_dest> <req-code-id>|reply-code-id> argNameN=argValueN<NULL>

   It is necessary that code points to a character buffer with length >= (REQMODSIZE+1).
   It is assumed that the request format was validated before calling this function, although
   this function does validate the tokens until the code token and the code token.
*/
wstatus
_req_text_code(request_t req,char *code)
{
	char *code_ptr,*aux_ptr;
	char reqcode[REQCODESIZE+1];
	int i, j;
	wstatus ws;

	dbgprint(MOD_MODMGR,__func__,"called with req=%p, code=%p",req,code);

	if( !req ) {
		dbgprint(MOD_MODMGR,__func__,"invalid req argument (req=0)");
		goto return_fail;
	}

	if( !code ) {
		dbgprint(MOD_MODMGR,__func__,"invalid code argument (code=0)");
		goto return_fail;
	}

	if( req->stype != REQUEST_STYPE_TEXT ) {
		dbgprint(MOD_MODMGR,__func__,"invalid request to be used with this function");
		goto return_fail;
	}

	ws = _req_text_token_seek(req->data.text.raw,TEXT_TOKEN_CODE,&code_ptr);
	if( ws != WSTATUS_SUCCESS ) {
		DBGRET_FAILURE(MOD_MODMGR);
	}

	aux_ptr = code_ptr;
	for( i = 0, j = 0 ; !V_REQENDCHAR(aux_ptr[i]) ; i++ )
	{
		if( V_TOKSEPCHAR(aux_ptr[i]) )
			goto finished_token;

		if( V_CODECHAR(aux_ptr[i]) )
	   	{
			if( j >= REQCODESIZE ) {
				dbgprint(MOD_MODMGR,__func__,"(req=%p) request code has too many characters (limit is %d)",req,REQCODESIZE);
				goto return_fail;
			}
			reqcode[j++] = aux_ptr[i];
			continue;
		}

		/* invalid character detected */
		dbgprint(MOD_MODMGR,__func__,"(req=%p) invalid/unexpected character in CODE token (offset %d)",req,i);
		goto return_fail;
	}

	/* code is a "special case" token because its the last token of the header of the request,
	   name-value pairs are not mandatory so after the code token the request might end. */

//	dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to reach CODE token end",req);
//	goto return_fail;

finished_token:

	reqcode[j] = '\0';

	dbgprint(MOD_MODMGR,__func__,"(req=%p) got request code \"%s\"",req,reqcode);
	strcpy(code,reqcode);
	dbgprint(MOD_MODMGR,__func__,"(req=%p) wrote request code into code=%p (%d bytes long)",
			req,code,strlen(reqcode));

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:
	DBGRET_FAILURE(MOD_MODMGR);
}
/*
   _req_text_dst

   Helper function to get destiny module from the text request. Request format is:
   <req-id><type> <mod_source> <mod_dest> <req-code-id>|reply-code-id> argNameN=argValueN<NULL>

   It is necessary that dst points to a character buffer with length >= (REQMODSIZE+1).
   It is assumed that the request format was validated before calling this function.
*/
wstatus
_req_text_dst(request_t req,char *dst)
{
	char reqdst[REQMODSIZE+1];
	char *dst_ptr, *aux_ptr;
	int i, j;
	wstatus ws;

	dbgprint(MOD_MODMGR,__func__,"called with req=%p, dst=%p",req,dst);

	if( !req ) {
		dbgprint(MOD_MODMGR,__func__,"invalid req argument (req=0)");
		goto return_fail;
	}

	if( !dst ) {
		dbgprint(MOD_MODMGR,__func__,"invalid dst argument (dst=0)");
		goto return_fail;
	}

	if( req->stype != REQUEST_STYPE_TEXT ) {
		dbgprint(MOD_MODMGR,__func__,"invalid request to be used with this function");
		goto return_fail;
	}

	ws = _req_text_token_seek(req->data.text.raw,TEXT_TOKEN_MODDST,&dst_ptr);
	if( ws != WSTATUS_SUCCESS ) {
		DBGRET_FAILURE(MOD_MODMGR);
	}

	aux_ptr = dst_ptr;
	for( i = 0, j = 0 ; !V_REQENDCHAR(aux_ptr[i]) ; i++ )
	{
		if( V_TOKSEPCHAR(aux_ptr[i]) )
			goto finished_token;

		if( V_MODCHAR(aux_ptr[i]) )
	   	{
			if( j >= REQMODSIZE ) {
				dbgprint(MOD_MODMGR,__func__,"(req=%p) destination module name is overlimit (limit is %d)",req,REQMODSIZE);
				goto return_fail;
			}
			reqdst[j++] = aux_ptr[i];
			continue;
		}

		/* invalid character detected */
		dbgprint(MOD_MODMGR,__func__,"(req=%p) invalid/unexpected character in MODDST token",req);
		goto return_fail;
	}

	dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to reach MODDST token end",req);
	goto return_fail;

finished_token:

	reqdst[j] = '\0';

	dbgprint(MOD_MODMGR,__func__,"(req=%p) got destiny module name \"%s\"",req,reqdst);
	strcpy(dst,reqdst);
	dbgprint(MOD_MODMGR,__func__,"(req=%p) wrote destiny module name into dst=%p (%d bytes long)",
			req,dst,strlen(reqdst));

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:
	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _req_text_rid

   Helper function to get request id from the text request. Request format is:
   <req-id><type> <mod_source> <mod_dest> <req-code-id>|reply-code-id> argNameN=argValueN<NULL>

   Does NOT: verify if request id number is bigger than REQIDSIZE. It stops processing chars
   at REQIDSIZE so the id number is truncated eg. "655351" will result in 65535. 

   It is assumed that the request format was validated before calling this function.
*/
wstatus
_req_text_rid(request_t req,uint16_t *rid)
{
	char reqid_char[REQIDSIZE+1];
	uint16_t reqid_int;
	int i;

	dbgprint(MOD_MODMGR,__func__,"called with req=%p, rid=%p",req,rid);

	if( !req ) {
		dbgprint(MOD_MODMGR,__func__,"invalid req argument (req=0)");
		goto return_fail;
	}

	if( !rid ) {
		dbgprint(MOD_MODMGR,__func__,"invalid rid argument (rid=0)");
		goto return_fail;
	}

	if( req->stype != REQUEST_STYPE_TEXT ) {
		dbgprint(MOD_MODMGR,__func__,"invalid request to be used with this function");
		goto return_fail;
	}

	memset(reqid_char,0,sizeof(reqid_char));
	dbgprint(MOD_MODMGR,__func__,"(req=%p) local reqid cleared, starting the parsing",req);

	for( i = 0 ; i < REQIDSIZE ; i++ )
   	{
		if( V_REPLYCHAR(req->data.text.raw[i]) || V_TOKSEPCHAR(req->data.text.raw[i]) )
			break;

		if( !V_RIDCHAR(req->data.text.raw[i]) )
			goto invalid_char;

		reqid_char[i] = req->data.text.raw[i];
	}

	if( reqid_char[0] == '\0' ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to parse request data",req);
		goto return_fail;
	}

	dbgprint(MOD_MODMGR,__func__,"(req=%p) converting reqid from char to int");
	if( !sscanf(reqid_char,"%hu",&reqid_int) ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) failed to convert reqid from char (%s) to int",req,reqid_char);
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"(req=%p) converted reqid from char (%s) to int successfully (%u)",req,reqid_char,reqid_int);
	*rid = reqid_int;
	dbgprint(MOD_MODMGR,__func__,"(req=%p) updated argument rid to new value %u",req,*rid);

	
	DBGRET_SUCCESS(MOD_MODMGR);

invalid_char:
	dbgprint(MOD_MODMGR,__func__,"(req=%p) invalid/unexpected character in ID token (c=%02X)",req,req->data.text.raw[i]);
return_fail:
	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _req_text_type

   Helper function to get request type from the text request. Request format is:
   <req-id><type> <mod_source> <mod_dest> <req-code-id>|reply-code-id> argNameN=argValueN<NULL>

   The <type> is 'R' or 'r' for reply and no char for request.
   
   This function does-not verify request id length.
   It is assumed that the request format was validated before calling this function.
*/
wstatus
_req_text_type(request_t req,request_type_list *type)
{
	char *type_ptr;
	request_type_list reqtype_num;
	wstatus ws;

	dbgprint(MOD_MODMGR,__func__,"called with req=%p, type=%p",req,type);

	if( !req ) {
		dbgprint(MOD_MODMGR,__func__,"invalid req argument (req=0)");
		goto return_fail;
	}

	if( !type ) {
		dbgprint(MOD_MODMGR,__func__,"invalid type argument (type=0)");
		goto return_fail;
	}

	if( req->stype != REQUEST_STYPE_TEXT ) {
		dbgprint(MOD_MODMGR,__func__,"invalid request to be used with this function");
		goto return_fail;
	}

	ws = _req_text_token_seek(req->data.text.raw,TEXT_TOKEN_TYPE,&type_ptr);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to reach TYPE token in request",req);
		goto return_fail;
	}

	if( V_REPLYCHAR(*type_ptr) ) {
		reqtype_num = REQUEST_TYPE_REPLY;
	} else if( V_TOKSEPCHAR(*type_ptr) ) {
		reqtype_num = REQUEST_TYPE_REQUEST;
	} else {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) invalid request type char found after the id",req);
		goto return_fail;
	}

	dbgprint(MOD_MODMGR,__func__,"(req=%p) updating type argument to value %d",req,reqtype_num);
	*type = reqtype_num;
	dbgprint(MOD_MODMGR,__func__,"(req=%p) updated type argument to value %d",req,*type);

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:
	DBGRET_FAILURE(MOD_MODMGR);
}


/*
   _req_text_nv_count

   Helper function to get request name-value pairs count from the text request. Request format is:
   <req-id><type> <mod_source> <mod_dest> <req-code-id>|reply-code-id> argNameN=argValueN<NULL>
                 1            2          3                            4
   This count number is one-based (starts from 1).
   
   - The argument name is alfanumeric. See manpages of isalnum.
   - The argument value may contain only chars from base64 or if also uses space character (0x20),
   the value string should be contained inside brakets " (0x22), eg. lala="xxx yyy".

   It is assumed that the request format was validated before calling this function.
*/
wstatus
_req_text_nv_count(request_t req,unsigned int *nvcount)
{
	char *aux;
	int state;
	unsigned int reqnvcount;
	char *name_start,*value_start;
	unsigned int name_size,value_size;
	wstatus ws;
	value_format_list value_format;

	dbgprint(MOD_MODMGR,__func__,"called with req=%p, nvcount=%p",req,nvcount);

	if( !req ) {
		dbgprint(MOD_MODMGR,__func__,"invalid req argument (req=0)");
		goto return_fail;
	}

	if( !nvcount ) {
		dbgprint(MOD_MODMGR,__func__,"invalid nvcount argument (nvcount=0)");
		goto return_fail;
	}

	if( req->stype != REQUEST_STYPE_TEXT ) {
		dbgprint(MOD_MODMGR,__func__,"invalid request to be used with this function");
		goto return_fail;
	}

	dbgprint(MOD_MODMGR,__func__,"(req=%p) looking up for nvpair start",req);

	aux = (char*)&req->data.text.raw;
	ws = _req_text_token_seek(aux,TEXT_TOKEN_NVL,&aux);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to find nvl begin in request",req);
		goto return_fail;
	}

	dbgprint(MOD_MODMGR,__func__,"(req=%p) found nvpair start at +%d",req,(int)(aux - (char*)&req->data.text.raw));

	if( V_REQENDCHAR(*aux) ) {
		/* this request doesn't have any nvpair */
		reqnvcount = 0;
		goto skip_nvpair_parsing;
	}

	reqnvcount = 0;
	state = 0;

	while(1)
	{
		ws = _req_text_nv_parse(aux,&name_start,&name_size,&value_start,&value_size,&value_format);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"(req=%p) failed to process text request");
			goto return_fail;
		}
		reqnvcount++;

		if( value_start ) {
			aux = value_start + value_size;
			if( V_QUOTECHAR(*aux) )
				aux++;
		} else
		{
			aux = name_start + name_size;
		}

		if( V_REQENDCHAR(*aux) )
			/* end of request was reached */
			break;

		if( *aux != ' ' ) {
			dbgprint(MOD_MODMGR,__func__,"(req=%p) [nv_idx=%u] invalid/unexpected character after nvpair",req,reqnvcount);
			goto return_fail;
		}

		aux++;
	}

skip_nvpair_parsing:
	dbgprint(MOD_MODMGR,__func__,"(req=%p) parsing is finished, detected %u nvpairs",req,reqnvcount);
	*nvcount = reqnvcount;
	dbgprint(MOD_MODMGR,__func__,"(req=%p) updated nvcount argument to new value %u",req,*nvcount);

	DBGRET_SUCCESS(MOD_MODMGR);
return_fail:
	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _req_nv_name_info

   Helper function to get NAME field information. This function uses some MACROS
   associated to special chars of the communications protocol for requests in wicom.
   For more information about them check the doc or modmgr.h file.

   Returns failure when a unexpected character was found in the NAME field.
   If name_ptr points to an empty name field it returns name_end = name_ptr
   and name_size = 0, a valid and successful result.

   It is assumed that the name_ptr points to the request text and this has the
   end character in, otherwise this function might run forever.
*/
wstatus
_req_nv_name_info(char *name_ptr,char **name_end,uint16_t *name_size)
{
	uint16_t i;

	assert(name_ptr != 0);
	assert(name_end != 0);

	*name_end = NULL;
	*name_size = 0;

	for( i = 0 ; ; i++ )
	{
		if( V_REQENDCHAR(name_ptr[i]) || V_NVSEPCHAR(name_ptr[i]) || V_TOKSEPCHAR(name_ptr[i]) )
			goto end_of_name;

		if( !V_NAMECHAR(name_ptr[i]) )
			goto invalid_char;
	}

invalid_char:
	/* found invalid character in NAME field */
	dbgprint(MOD_MODMGR,__func__,"found invalid/unexpected character in name parsing (c=%02X), name_ptr=%p",name_ptr[i],name_ptr);
	goto return_fail;

end_of_name:
	if( !i ) {
		dbgprint(MOD_MODMGR,__func__,"name is empty (no name character found)");
		*name_size = 0;
	}
	*name_end = name_ptr + i - 1;
	*name_size = i;
	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:
	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _req_nv_value_info

   Helper function to obtain VALUE field information. The arguments might require some words,
   value_start and value_end point to the first character and end character of the value string
   *excluding* any delimiting quotes. So for example, v="abc" will make value_start point to 'a'
   and value_end point to 'c', value_size is the length (1 based) of the value in characters.
   In case of the example, should result in value_size = 3.
*/
wstatus
_req_nv_value_info(char *value_ptr,char **value_start,char **value_end,uint16_t *value_size)
{
	unsigned int i = 0;
	char end_char = ' ',c;
	bool quoted = false, encoded = false;

	assert( value_ptr != 0 );
	assert( value_end != 0 );
	assert( value_size != 0 );

	if( V_QUOTECHAR(value_ptr[0]) ) {
		quoted = true;
		end_char = value_ptr[0];
		i = 1;
	} else if( V_ENCPREFIX(value_ptr[0]) ) {
		/* this is an encoded value */
		encoded = true;
		i = 1;
	}

	for( ; ; i++ )
	{
		c = value_ptr[i];

		if( V_REQENDCHAR(c) ) {
			/* reached the end of the request */
			goto end_of_req;
		}

		if( c == end_char ) {
			/* reached the end of the value field */
			goto end_of_value;
		}

		if( encoded && V_EVALUECHAR(c) )
			continue;

		if( quoted && V_QVALUECHAR(c) ) 
			continue;

		if( !quoted && V_VALUECHAR(c) )
			continue;

		goto invalid_char;
	}

	/* should not get here in any way? */
	dbgprint(MOD_MODMGR,__func__,"ups... fix _req_nv_value_info code");
	DBGRET_FAILURE(MOD_MODMGR);

end_of_req:
	if( quoted ) {
		dbgprint(MOD_MODMGR,__func__,"value didn't finished with quote char (%c)",end_char);
		DBGRET_FAILURE(MOD_MODMGR);
	}

	/* un-quoted, this was the last nvpair of the request */

	if( i == 0 ) {
		/* string is like "... name1=" , ugly way of finishing... */
		dbgprint(MOD_MODMGR,__func__,"if there's no value don't use NVPAIR separator char either");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	*value_size = i;
	*value_start = value_ptr;
	*value_end = value_ptr + *value_size - 1;

	dbgprint(MOD_MODMGR,__func__,"finishing with end_of_req, writing value_start=%p, value_end=%p, value_size=%u",
			*value_start,*value_end,*value_size);

	DBGRET_SUCCESS(MOD_MODMGR);

end_of_value:

	if( quoted )
	{
		if( i == 1 ) {
			/* empty value with quotes, eg. name1="" */
			*value_start = value_ptr + 1;
			*value_end = *value_start;
			*value_size = 0;
		} else
		{
			/* value with quotes and non-empty content */
			*value_start = value_ptr + 1;
			*value_end = value_ptr + i - 1; /* value_ptr[i] points to the quote char */
			*value_size = (*value_end - *value_start) + 1;
		}
	} else
	{
		*value_size = i;
		*value_start = value_ptr;
		*value_end = value_ptr + *value_size - 1;
	}

	dbgprint(MOD_MODMGR,__func__,"updated value_start=%p, value_end=%p, value_size=%u",
			*value_start,*value_end,*value_size);

	DBGRET_SUCCESS(MOD_MODMGR);

invalid_char:
	dbgprint(MOD_MODMGR,__func__,"invalid or unexpected character found in VALUE field (c=%02X)",c);
	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _req_nv_value_format

   Helper function to get value format.
*/
wstatus
_req_nv_value_format(const char *value_ptr,value_format_list *value_format)
{
	dbgprint(MOD_MODMGR,__func__,"called with value_ptr=%p, value_format=%p",value_ptr,value_format);

	if( !value_ptr ) {
		dbgprint(MOD_MODMGR,__func__,"invalid value_ptr argument (value_ptr=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( !value_format ) {
		dbgprint(MOD_MODMGR,__func__,"invalid value_format argument (value_format=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( V_ENCPREFIX(*value_ptr) ) {
		*value_format = VALUE_FORMAT_ENCODED;
	} else if ( V_QUOTECHAR(*value_ptr) ) {
		*value_format = VALUE_FORMAT_QUOTED;
	} else if ( V_VALUECHAR(*value_ptr) ) {
		*value_format = VALUE_FORMAT_UNQUOTED;
	} else {
		dbgprint(MOD_MODMGR,__func__,"unable to determine value format (c=%c)",*value_ptr);
		DBGRET_FAILURE(MOD_MODMGR);
	}

	dbgprint(MOD_MODMGR,__func__,"updated value_format value to %d",*value_format);

	DBGRET_SUCCESS(MOD_MODMGR);
}

/*
   _req_text_nv_parse

   Helper function that will parse a name-value pair in text form and return
   some useful information that can be used to convert the name-value pair
   into other form (pipe, binary). This function can also be used for request
   validation and others.

   It is possible that the specific nvpair doesn't have a value associated,
   in that case, value_size is set to 0 (zero). Name size cannot in any case
   be zero, if it happens, this function returns failure.
*/
wstatus
_req_text_nv_parse(char *nv_ptr,char **name_start,unsigned int *name_size,
		char **value_start,unsigned int *value_size,value_format_list *value_format)
{
	wstatus ws;
	char *l_name_end;
	uint16_t l_name_size;
	char *l_value_start;
	char *l_value_end;
	uint16_t l_value_size;
	value_format_list l_value_format;

	dbgprint(MOD_MODMGR,__func__,"called with nv_ptr=%p, name_start=%p, "
			"name_size=%p, value_start=%p, value_size=%p, value_format=%p",
			nv_ptr,name_start,name_size,value_start,value_size,value_format);

	assert(nv_ptr != 0);
	assert(name_start != 0);
	assert(name_size != 0);
	assert(value_start != 0);
	assert(value_size != 0);
	assert(value_format != 0);

	/* start by parsing name token */
	ws = _req_nv_name_info(nv_ptr,&l_name_end,&l_name_size);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"failed to parse NAME token");
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"valid name found ptr=%p, size=%u",nv_ptr,l_name_size);

	*name_start = nv_ptr;
	*name_size = l_name_size;
	dbgprint(MOD_MODMGR,__func__,"updated name_start=%p, name_size=%u",*name_start,*name_size);

	/* verify if is there any value associated to this nvpair */

	if( V_REQENDCHAR(nv_ptr[l_name_size]) || V_TOKSEPCHAR(nv_ptr[l_name_size]) ) {
		l_value_start = 0;
		l_value_size = 0;
		goto no_value;
	}

	if( !V_NVSEPCHAR(nv_ptr[l_name_size]) ) {
		dbgprint(MOD_MODMGR,__func__,"invalid/unexpected char after NAME token (c=%02X)",nv_ptr[l_name_size]);
		goto return_fail;
	}

	/* get value format using specific function */

	ws = _req_nv_value_format(nv_ptr+l_name_size+1,&l_value_format);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"failed to get VALUE token format of this nvpair");
		goto return_fail;
	}

	/* get value using helper function, this will get the full token in raw form,
	   so even if its encoded, it will return the encoded value. */
	ws = _req_nv_value_info(nv_ptr+l_name_size+1,&l_value_start,&l_value_end,&l_value_size);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"failed to parse VALUE token of this nvpair");
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"valid value found ptr=%p, size=%u",l_value_start,l_value_size);

no_value:
	*value_start = l_value_start;
	*value_size = l_value_size;
	*value_format = l_value_format;
	dbgprint(MOD_MODMGR,__func__,"updated value_start=%p, value_size=%u, value_format=%d",
			*value_start,*value_size,*value_format);

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:
	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _req_text_token_seek

   Helper function that returns a pointer to the token (seek_token).
*/
wstatus
_req_text_token_seek(const char *req_text,text_token_t token,char **token_ptr)
{
	text_token_t cur_token;
	char *cur_token_str = 0;
	uint16_t i;

	dbgprint(MOD_MODMGR,__func__,"called with req_text=%p, token=%d, token_ptr=%p",req_text,token,token_ptr);

	assert(req_text != 0);
	assert(token_ptr != 0);

	for( cur_token = TEXT_TOKEN_ID, i = 0 ; !V_REQENDCHAR(req_text[i]) ; i++ )
	{
		if( cur_token == token )
			goto reached_token;

		switch(cur_token)
		{
			case TEXT_TOKEN_ID:
				
				if( V_TOKSEPCHAR(req_text[i]) || V_REPLYCHAR(req_text[i]) )
				{
					/* finished ID token */
					cur_token = TEXT_TOKEN_TYPE;

					/* let it repeat for this char, it might belong to TYPE token */
					i--;
					continue;
				}

				if( V_RIDCHAR(req_text[i]) )
					continue;

				cur_token_str = "ID";
				goto return_fail;
			
			case TEXT_TOKEN_TYPE:

				if( V_REPLYCHAR(req_text[i]) ) {
					/* TYPE=REPLY finished TYPE token */
					cur_token = TEXT_TOKEN_MODSRC;
					continue;
				}

				if( V_TOKSEPCHAR(req_text[i]) ) {
					/* TYPE=REQUEST finished TYPE token */
					cur_token = TEXT_TOKEN_MODSRC;
					continue;
				}

				cur_token_str = "TYPE";
				goto return_fail;
			
			case TEXT_TOKEN_MODSRC:
			
				if( V_TOKSEPCHAR(req_text[i]) )
				{
					cur_token = TEXT_TOKEN_MODDST;
					continue;
				}

				if( V_MODCHAR(req_text[i]) )
					continue;

				cur_token_str = "MODSRC";
				goto return_fail;

			case TEXT_TOKEN_MODDST:

				if( V_TOKSEPCHAR(req_text[i]) )
				{
					cur_token = TEXT_TOKEN_CODE;
					continue;
				}

				if( V_MODCHAR(req_text[i]) )
					continue;

				cur_token_str = "MODDST";
				goto return_fail;

			case TEXT_TOKEN_CODE:

				if( V_TOKSEPCHAR(req_text[i]) )
				{
					cur_token = TEXT_TOKEN_NVL;
					continue;
				}

				if( V_CODECHAR(req_text[i]) )
					continue;

				cur_token_str = "CODE";
				goto return_fail;
			case TEXT_TOKEN_NVL:
			default:
				dbgprint(MOD_MODMGR,__func__,"invalid/unsupported token selected");
				goto return_fail;
		}
	}

	if( cur_token == TEXT_TOKEN_CODE ) {
		/* this request doesn't have any name-value pair */
		cur_token = TEXT_TOKEN_NVL;
		/* token_ptr will point to the null char, take care with that Mr.Client code. */
		goto reached_token;
	}

	dbgprint(MOD_MODMGR,__func__,"unable to reach to the requested token");
	DBGRET_FAILURE(MOD_MODMGR);

reached_token:
	dbgprint(MOD_MODMGR,__func__,"found token at offset +%d",i);
	*token_ptr = (char*)(req_text + i);
	dbgprint(MOD_MODMGR,__func__,"updated token_ptr to %p",*token_ptr);
	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:
	if( cur_token_str )
		dbgprint(MOD_MODMGR,__func__,"invalid/unexpected character (c=%02X) at %u in token %s",
				req_text[i], i, cur_token_str);

	DBGRET_FAILURE(MOD_MODMGR);
}

wstatus
_req_from_pipe_to_bin(request_t req,request_t *req_bin)
{
	return WSTATUS_UNIMPLEMENTED;
}

/*
   _req_nvl_jml_free
   
   Helper callback to free a nvpair data structure, associated to jmlist entry.
   This is a callback to jmlist_parse function.
*/
void _req_nvl_jml_free(void *ptr,void *param)
{
	nvpair_t nvp = (nvpair_t)ptr;

	if( nvp ) {
		_nvp_free(nvp);
	}
}

/*
   _nvp_value_decoded_size

   Helper function to get decoded value size.
*/
wstatus
_nvp_value_decoded_size(const char *value_ptr,const uint16_t value_size,unsigned int *decoded_size)
{
	dbgprint(MOD_MODMGR,__func__,"called with value_ptr=%p, value_size=%u, decoded_size=%p",
			value_ptr,value_size,decoded_size);

	if( !value_size ) {
		dbgprint(MOD_MODMGR,__func__,"value is empty (value_size=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( !decoded_size ) {
		dbgprint(MOD_MODMGR,__func__,"invalid decoded_size argument (decoded_size=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( !(value_size & 1) ) {
		dbgprint(MOD_MODMGR,__func__,"invalid value_size (odd number expected)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	*decoded_size = (value_size - 1)/2;
	dbgprint(MOD_MODMGR,__func__,"updated decoded_size value to %u",*decoded_size);

	DBGRET_SUCCESS(MOD_MODMGR);
}

/*
   _nvp_value_decode

   Helper function to decode a value to its original format.
   The decoded_size argument tells how big is the decoded_ptr buffer.
*/
wstatus
_nvp_value_decode(const char *value_ptr,const uint16_t value_size,char *decoded_ptr,unsigned int decoded_size)
{
	unsigned int i,j,k;
	char hex_str[3];

	dbgprint(MOD_MODMGR,__func__,"called with value_ptr=%p, value_size=%u, decoded_ptr=%p, decoded_size=%u",
			value_ptr,value_size,decoded_ptr,decoded_size);

	if( !(value_size & 1) ) {
		dbgprint(MOD_MODMGR,__func__,"invalid value_size (odd number expected)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	/* i starts at 1 to skip the ENCPREFIXCHAR */
	for( i = 1, j = 0 ; i < value_size ; i+= 2 )
	{
		hex_str[0] = value_ptr[i];
		hex_str[1] = value_ptr[i+1];
		hex_str[2] = '\0';

		if( sscanf(hex_str,"%X",&k) != 1 ) {
			dbgprint(MOD_MODMGR,__func__,"couldn't convert %s to binary",hex_str);
			DBGRET_FAILURE(MOD_MODMGR);
		}

		if( j >= decoded_size ) {
			dbgprint(MOD_MODMGR,__func__,"decoded buffer is too small for value");
			DBGRET_FAILURE(MOD_MODMGR);
		}

		decoded_ptr[j++] = (char) (k & 0xFF);
	}

	DBGRET_SUCCESS(MOD_MODMGR);
}

/*
   _req_from_text_to_pipe

   Helper function to convert a request data structure from text type to pipe type.
   <req-id><type> <mod_source> <mod_dest> <req-code-id>|reply-code-id> argNameN=argValueN<NULL>
*/
wstatus
_req_from_text_to_bin(request_t req,request_t *req_bin)
{
	request_t new_req = 0;
	struct _jmlist_params jmlp = { .flags = JMLIST_LINKED };
	uint16_t reqid;
	request_type_list reqtype;
	unsigned int nv_idx,nvcount;
	nvpair_t nvp;
	jmlist_status jmls;
	wstatus ws;
	char *aux;
	char *name_start,*value_start;
	unsigned int name_size,value_size;
	value_format_list value_format;
	char *decoded_ptr = 0;
	unsigned int decoded_size = 0;

	dbgprint(MOD_MODMGR,__func__,"called with req=%p, req_bin=%p",req,req_bin);

	/* validate arguments */

	if( !req ) {
		dbgprint(MOD_MODMGR,__func__,"invalid req argument (req=0)");
		goto return_fail;
	}

	if( !req_bin ) {
		dbgprint(MOD_MODMGR,__func__,"invalid req_bin argument (req_bin=0)");
		goto return_fail;
	}

	if( req->stype != REQUEST_STYPE_TEXT ) {
		dbgprint(MOD_MODMGR,__func__,"invalid request to be used with this function");
		goto return_fail;
	}

	/* process headers */

	new_req = (request_t)malloc(sizeof(struct _request_t));
	if( !new_req ) {
		dbgprint(MOD_MODMGR,__func__,"malloc failed");
		goto return_fail;
	}
	memset(new_req,0,sizeof(struct _request_t));
	dbgprint(MOD_MODMGR,__func__,"(req=%p) allocated new request data structure (%p)",req,new_req);

	/* create jmlist for the nvp */
	jmls = jmlist_create(&new_req->data.bin.nvl,&jmlp);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) failed to create jmlist (jmls=%d)",req,jmls);
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"(req=%p) created jmlist for nvl successfully (jml=%p)",req,new_req->data.bin.nvl);

	/* start processing text request */

	ws = _req_text_rid(req,&reqid);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to get request id",req);
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"(req=%p) got request id %u",req,reqid);
	new_req->data.bin.id = reqid;

	ws = _req_text_type(req,&reqtype);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to get request type",req);
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"(req=%p) got request type %s",req,
			(reqtype == REQUEST_TYPE_REQUEST ? "REQUEST" : "REPLY"));
	new_req->data.bin.type = reqtype;

	ws = _req_text_src(req,new_req->data.bin.src);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to get request source module",req);
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"(req=%p) got request source module (%s)",req,new_req->data.bin.src);

	ws = _req_text_dst(req,new_req->data.bin.dst);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to get request destiny module",req);
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"(req=%p) got request destination module (%s)",req,new_req->data.bin.dst);

	ws = _req_text_code(req,new_req->data.bin.code);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to get request code",req);
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"(req=%p) got request code (%s)",req,new_req->data.bin.code);

	/* start processing the aditional nvpairs */

	ws = _req_text_nv_count(req,&nvcount);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to get nvcount from request",req);
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"(req=%p) this request has %u nvpairs",req,nvcount);
	
	if( nvcount == 0 )
		goto skip_nvpair_parsing;

	ws = _req_text_token_seek(req->data.text.raw,TEXT_TOKEN_NVL,&aux);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to find nvl begin",req);
		goto return_fail;
	}

	for( nv_idx = 1 ; ; nv_idx++ )
   	{
		/* try to parse nvpair text tokens */
		ws = _req_text_nv_parse(aux,&name_start,&name_size,&value_start,&value_size,&value_format);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"(req=%p) failed to process text request");
			goto return_fail;
		}

		if( value_format == VALUE_FORMAT_ENCODED )
	   	{
			ws = _nvp_value_decoded_size(value_start,value_size,&decoded_size);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_MODMGR,__func__,"(req=%p) failed to get decoded size for nv_idx=%u",req,nv_idx);
				goto return_fail;
			}

			/* allocate buffer that will store the decoded value */

			decoded_ptr = (char*)malloc(decoded_size);
			if( !decoded_ptr ) {
				dbgprint(MOD_MODMGR,__func__,"(req=%p) malloc failed on decoded_size=%u",req,decoded_size);
				goto return_fail;
			}
			dbgprint(MOD_MODMGR,__func__,"(req=%p) allocated buffer for decoded value successful (ptr=%p)",
					req,decoded_ptr);

			ws = _nvp_value_decode(value_start,value_size,decoded_ptr,decoded_size);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to decode value (ws=%s)",req,wstatus_str(ws));
				goto return_fail;
			}
			dbgprint(MOD_MODMGR,__func__,"(req=%p) decoded successfully the value",req);
		}

		/* allocate new nvpair data structure for this nvpair */

		if( value_format == VALUE_FORMAT_ENCODED )
			ws = _nvp_alloc(name_size,decoded_size,&nvp);
		else
			ws = _nvp_alloc(name_size,value_size,&nvp);

		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"(req=%p) failed to allocate a new nvpair (name_size=%u, value_size=%u)",
					req,name_size,value_size);
			goto return_fail;
		}
		dbgprint(MOD_MODMGR,__func__,"(req=%p) allocated new nvpair data structure successfully (p=%p)",req,nvp);

		/* fill the new data structure with the tokens */
		
		if( value_format == VALUE_FORMAT_ENCODED )
			ws = _nvp_fill(name_start,name_size,decoded_ptr,decoded_size,nvp);
		else
			ws = _nvp_fill(name_start,name_size,value_start,value_size,nvp);

		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"(req=%p) failed to fill the new nvpair (p=%p)",req,nvp);
			goto return_fail;
		}
		dbgprint(MOD_MODMGR,__func__,"(req=%p) new nvpair ready for insertion in nvpair list",req);

		/* we can now free the decoded buffer if it was used */
		
		if( decoded_ptr ) {
			free(decoded_ptr);
			decoded_ptr = 0;
			decoded_size = 0;
		}

		jmls = jmlist_insert(new_req->data.bin.nvl,nvp);
		if( jmls != JMLIST_ERROR_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"(req=%p) jmlist failed to insert new nvpair data strucutre (jmls=%d)",req,jmls);
			goto return_fail;
		}

		if( value_start ) {
			aux = value_start + value_size;
			if( V_QUOTECHAR(*aux) )
				aux++;
		} else
			 aux = name_start + name_size;

		if( V_REQENDCHAR(*aux) )
			/* end of request was reached */
			break;

		if( *aux != ' ' ) {
			dbgprint(MOD_MODMGR,__func__,"(req=%p) [nv_idx=%u] invalid/unexpected character after nvpair",req,nv_idx);
			goto return_fail;
		}

		aux++;
	}

skip_nvpair_parsing:

	*req_bin = new_req;
	dbgprint(MOD_MODMGR,__func__,"(req=%p) updated req_bin to %p",req,*req_bin);

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:

	if( new_req )
   	{
		if( new_req->data.bin.nvl ) {
			jmlist_parse(new_req->data.bin.nvl,_req_nvl_jml_free,0);
			jmlist_free(new_req->data.bin.nvl);
		}

		free(new_req);
	}

	if( decoded_ptr )
		free(decoded_ptr);

	DBGRET_FAILURE(MOD_MODMGR);
}

wstatus
_req_to_bin(request_t req,request_t *req_bin)
{
	wstatus ws;
	dbgprint(MOD_MODMGR,__func__,"called with req=%p, req_bin=%p",req,req_bin);

	switch(req->stype)
	{
		case REQUEST_STYPE_BIN:
			dbgprint(MOD_MODMGR,__func__,"(req=%p) request is already in bin data structure");
			goto return_fail;
		case REQUEST_STYPE_TEXT:
			dbgprint(MOD_MODMGR,__func__,"(req=%p) calling helper function _req_from_text_to_bin",req);
			ws = _req_from_text_to_bin(req,req_bin);
			dbgprint(MOD_MODMGR,__func__,"(req=%p) helper function _req_from_text_to_bin returned ws=%d",req,ws);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_MODMGR,__func__,"(req=%p) helper function failed, aborting",req);
				goto return_fail;
			}
			break;
		case REQUEST_STYPE_PIPE:
			dbgprint(MOD_MODMGR,__func__,"(req=%p) calling helper function _req_from_pipe_to_bin",req);
			ws = _req_from_pipe_to_bin(req,req_bin);
			dbgprint(MOD_MODMGR,__func__,"(req=%p) helper function _req_from_pipe_to_bin returned ws=%d",req,ws);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_MODMGR,__func__,"(req=%p) helper function failed, aborting",req);
				goto return_fail;
			}
			break;
		default:
			dbgprint(MOD_MODMGR,__func__,"(req=%p) request has an invalid or unsupported stype (check req pointer...)",req);
			goto return_fail;
	}

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:
	DBGRET_FAILURE(MOD_MODMGR);
}

void _req_dump_header(uint16_t id,request_type_list type,char *mod_src,char *mod_dst,char *req_code)
{
	printf(	"    %s %u  %s -> %s (%s)\n",
			(type == REQUEST_TYPE_REQUEST ? "REQ" : "RLY"),id,
			mod_src,mod_dst,req_code);
}

/*
   _nvp_print_value

   Helper function to print the value of a nvpair. Values can contain special chars or even unprintable
   characters. This function supports different printing formats.
*/
void _nvp_print_value(const char *value_ptr, unsigned int value_size)
{
	unsigned int i;

	for( i = 0 ; i < value_size ; i++ )
	{
		if( isprint(value_ptr[i]) )
			putchar(value_ptr[i]);
		else
			putchar('?');
	}

	return;
}

/*
   _req_dump_nvp

   Helper function to dump a single nvpair. Remember that name_ptr and value_ptr need not to be
   null terminated, thats why this functions prints char by char.
*/
void _req_dump_nvp(char *name_ptr,uint16_t name_size,char *value_ptr,uint16_t value_size)
{
	unsigned int i;

	printf("    ");

	for( i = 0 ; i < name_size ; i++ )
		putchar(name_ptr[i]);

	if( value_size ) {
		printf("=");
		_nvp_print_value(value_ptr,value_size);
	}

	printf("\n");
}

wstatus
_req_text_dump(request_t req)
{
	printf("    %s|\n",req->data.text.raw);
	DBGRET_SUCCESS(MOD_MODMGR);
}

void _req_bin_jml_dump(void *ptr,void *param)
{
	nvpair_t nvp = (nvpair_t)ptr;
	_req_dump_nvp(nvp->name_ptr,nvp->name_size,nvp->value_ptr,nvp->value_size);
}

wstatus
_req_bin_dump(request_t req)
{
	dbgprint(MOD_MODMGR,__func__,"called with req=%p",req);

	_req_dump_header(req->data.bin.id,req->data.bin.type,req->data.bin.src,req->data.bin.dst,req->data.bin.code);
	jmlist_parse(req->data.bin.nvl,_req_bin_jml_dump,0);

	DBGRET_SUCCESS(MOD_MODMGR);
}

wstatus
_req_pipe_dump(request_t req)
{
	DBGRET_FAILURE(MOD_MODMGR);
}

wstatus
_req_dump(request_t req)
{
	wstatus ws;

	dbgprint(MOD_MODMGR,__func__,"called with req=%p",req);

	switch(req->stype)
	{
		case REQUEST_STYPE_BIN:
			dbgprint(MOD_MODMGR,__func__,"(req=%p) calling helper function _req_bin_dump",req);
			ws = _req_bin_dump(req);
			dbgprint(MOD_MODMGR,__func__,"(req=%p) helper function _req_bin_dump returned ws=%d",req,ws);
			break;
		case REQUEST_STYPE_TEXT:
			dbgprint(MOD_MODMGR,__func__,"(req=%p) calling helper function _req_text_dump",req);
			ws = _req_text_dump(req);
			dbgprint(MOD_MODMGR,__func__,"(req=%p) helper function _req_text_dump returned ws=%d",req,ws);
			break;
		case REQUEST_STYPE_PIPE:
			dbgprint(MOD_MODMGR,__func__,"(req=%p) calling helper function _req_pipe_dump",req);
			ws = _req_pipe_dump(req);
			dbgprint(MOD_MODMGR,__func__,"(req=%p) helper function _req_pipe_dump returned ws=%d",req,ws);
			break;
		default:
			dbgprint(MOD_MODMGR,__func__,"(req=%p) request has an invalid or unsupported stype (check req pointer...)",req);
			goto return_fail;
	}

	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) helper function failed, aborting",req);
		goto return_fail;
	}

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:
	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _req_to_text

   Helper function to convert a request to a new request with text form.
   Client code is responsible to free the new allocated request if the function
   succeds.
*/
wstatus
_req_to_text(request_t req,request_t *req_text)
{
	wstatus ws;
	dbgprint(MOD_MODMGR,__func__,"called with req=%p, req_text=%p",req,req_text);

	switch(req->stype)
	{
		case REQUEST_STYPE_BIN:
			dbgprint(MOD_MODMGR,__func__,"(req=%p) calling helper function _req_from_bin_to_text",req);
			ws = _req_from_bin_to_text(req,req_text);
			dbgprint(MOD_MODMGR,__func__,"(req=%p) helper function _req_from_bin_to_text returned ws=%d",req,ws);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_MODMGR,__func__,"(req=%p) helper function failed, aborting",req);
				goto return_fail;
			}
			break;
		case REQUEST_STYPE_TEXT:
			dbgprint(MOD_MODMGR,__func__,"(req=%p) request is already in text data structure");
			goto return_fail;
		case REQUEST_STYPE_PIPE:
			dbgprint(MOD_MODMGR,__func__,"(req=%p) calling helper function _req_from_pipe_to_text",req);
			ws = _req_from_pipe_to_text(req,req_text);
			dbgprint(MOD_MODMGR,__func__,"(req=%p) helper function _req_from_pipe_to_text returned ws=%d",req,ws);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_MODMGR,__func__,"(req=%p) helper function failed, aborting",req);
				goto return_fail;
			}
			break;
		default:
			dbgprint(MOD_MODMGR,__func__,"(req=%p) request has an invalid or unsupported stype (check req pointer...)",req);
			goto return_fail;
	}

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:
	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _req_bin_nvl_sum_length_jlcb

   Helper function used by req_from_bin_to_text, is a jmlist parser callback that sums
   the length of each nvl entry. The param pointer points to an integer which will receive
   the lengths.
*/
void _req_bin_nvl_sum_length_jlcb(void *ptr,void *param)
{
	nvpair_t nvp = (nvpair_t)ptr;
	int *nvl_size = (int*)param;
	unsigned int value_size;
	value_format_list value_format;
	wstatus ws;

	/* check if this nvpair has a value associated first */
	if( !nvp->value_size ) {
		/* name_size + ' ' */
		*nvl_size += nvp->name_size + 1;
		return;
	}

	/* get nvpair appropriate value format */
	ws = _nvp_value_format(nvp->value_ptr,nvp->value_size,&value_format);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"unable to get value format (nvp=%p)",nvp);
		return;
	}

	ws =_nvp_value_encoded_size(nvp->value_ptr,nvp->value_size,&value_size);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"unable to get encoded value size (nvp=%p)",nvp);
		return;
	}

	switch( value_format ) {
		case VALUE_FORMAT_UNQUOTED:
			/* name_size + '=' + value_size + ' ' */
			*nvl_size += nvp->name_size + 1 + nvp->value_size + 1;
			break;
		case VALUE_FORMAT_QUOTED:
			/* name_size + '=' + '"' + value_size + '"' + ' '   */
			*nvl_size += nvp->name_size + 1 + 1 + nvp->value_size + 1 + 1;
			break;
		case VALUE_FORMAT_ENCODED:
			/* name_size + '#' + value_size + ' ' */
			*nvl_size += nvp->name_size + 1 + value_size;
			break;
	}

	return;
}

/*
   _nvp_value_format

   Helper function that decides the format to use in the value of the name-value pair.
*/
wstatus
_nvp_value_format(const char *value_ptr, const unsigned int value_size,value_format_list *value_format)
{
	unsigned int i;
	value_format_list l_value_format = VALUE_FORMAT_UNQUOTED;
	value_format_list format_transform[][3] = {
		{ VALUE_FORMAT_UNQUOTED , VALUE_FORMAT_QUOTED , VALUE_FORMAT_ENCODED },
		{ VALUE_FORMAT_QUOTED , VALUE_FORMAT_QUOTED , VALUE_FORMAT_ENCODED },
		{ VALUE_FORMAT_ENCODED , VALUE_FORMAT_ENCODED , VALUE_FORMAT_ENCODED }
	}; /* format_transform[FORM][TO] */

	dbgprint(MOD_MODMGR,__func__,"called with value_ptr=%p, value_size=%u",value_ptr,value_size);

	if( !value_ptr ) {
		dbgprint(MOD_MODMGR,__func__,"invalid value_ptr argument (value_ptr=0)");
		goto return_fail;
	}

	if( !value_size ) {
		dbgprint(MOD_MODMGR,__func__,"value is empty (value_size=0), any format will do");
		goto return_fail;
	}

	if( !value_format ) {
		dbgprint(MOD_MODMGR,__func__,"invalid value_format argument (value_format=0)");
		goto return_fail;
	}

	for( i = 0 ; i < value_size ; i++ )
	{
		if( V_VALUECHAR(value_ptr[i]) ) {
			l_value_format = format_transform[l_value_format][VALUE_FORMAT_UNQUOTED];
			continue;
		}

		if( V_QVALUECHAR(value_ptr[i]) ) {
			l_value_format = format_transform[l_value_format][VALUE_FORMAT_QUOTED];
			continue;
		}

		l_value_format = VALUE_FORMAT_ENCODED;
		break;
	}

	dbgprint(MOD_MODMGR,__func__,"finished processing value bytes, result is format %d",l_value_format);

	*value_format = l_value_format;
	
	dbgprint(MOD_MODMGR,__func__,"updated value_format to %d",*value_format);

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:
	DBGRET_FAILURE(MOD_MODMGR);
}

/*
	_nvp_value_encoded_size

	Helper function to obtain the length in characters of the encoded value.
	This length includes the null char and is 1 based (starts at one).
*/
wstatus
_nvp_value_encoded_size(const char *value_ptr,const unsigned int value_size,unsigned int *encoded_size)
{
	dbgprint(MOD_MODMGR,__func__,"called with value_ptr=%p, value_size=%u, encoded_size=%p",
			value_ptr,value_size,encoded_size);

	if( !value_ptr ) {
		dbgprint(MOD_MODMGR,__func__,"invalid value_ptr argument (value_ptr=0)");
		goto return_fail;
	}

	if( !value_size ) {
		dbgprint(MOD_MODMGR,__func__,"this value is empty (value_size=0)");
		goto return_fail;
	}

	if( !encoded_size ) {
		dbgprint(MOD_MODMGR,__func__,"invalid encoded_size argument (encoded_size=0)");
		goto return_fail;
	}

	*encoded_size = (value_size * 2) + 1;
	dbgprint(MOD_MODMGR,__func__,"updated encoded_size value to %u",*encoded_size);

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:
	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _nvp_value_encode

   Helper function to encode the value data into a text string, here we've various options,
   one could use base64, uenc, etc. I'm going to use HEX encoding for now its the easiest to
   implement and to determine the final size, basically is the double.
*/
wstatus
_nvp_value_encode(const char *value_ptr,const unsigned int value_size,char **value_encoded)
{
	char *aux_ptr;
	unsigned int i;
	char c;
	char conv_table[] = {"0123456789ABCDEF"};

	dbgprint(MOD_MODMGR,__func__,"called with value_ptr=%p, value_size=%u, value_encoded=%p",
			value_ptr,value_size,value_encoded);

	aux_ptr = (char*)malloc(sizeof(char)*value_size*2 + 2*sizeof(char));
	if( !aux_ptr ) {
		dbgprint(MOD_MODMGR,__func__,"malloc failed (size=%d)",sizeof(char)*value_size*2 + 2*sizeof(char));
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"allocated buffer successfully (ptr=%p)",aux_ptr);

	*value_encoded = aux_ptr;

	dbgprint(MOD_MODMGR,__func__,"inserting encoded prefix");

	*aux_ptr = NVP_ENCODED_PREFIX;
	aux_ptr++;

	dbgprint(MOD_MODMGR,__func__,"starting the conversion of %d bytes",value_size);
	for( i = 0 ; i < value_size ; i++ )
	{
		c = value_ptr[i];

		*aux_ptr = conv_table[ (c & 0xF0) >> 4 ];
		aux_ptr++;
		*aux_ptr = conv_table[ (c & 0x0F) ];
		aux_ptr++;
	}
	dbgprint(MOD_MODMGR,__func__,"finished conversion of %d bytes, aux_ptr has offset +%d bytes",
			value_size,aux_ptr - *value_encoded);

	/* cat the null char */
	*aux_ptr = '\0';

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:
	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _req_bin_nvl_insert_jlcb

   Helper function used by req_from_bin_to_text, is a jmlist parser callback that inserts
   name-value pairs in text form into the request. The param is a pointer to the raw data
   of the request (text form).
*/
void _req_bin_nvl_insert_jlcb(void *ptr,void *param)
{
	char *req_text = (char*)param;
	nvpair_t nvp = (nvpair_t)ptr;
	value_format_list value_format;
	char *value_encoded;
	wstatus ws;

	req_text = req_text + strlen(req_text); /* points now to null char */

	dbgprint(MOD_MODMGR,__func__,"writing name into req_text (%p), length is %u bytes long",
			req_text,nvp->name_size);

	memcpy(req_text,nvp->name_ptr,nvp->name_size);
	req_text[nvp->name_size] = '\0';

	dbgprint(MOD_MODMGR,__func__,"copied name into req_text successfully");

	if( !nvp->value_size ) {
		req_text[nvp->name_size] = ' ';
		req_text[nvp->name_size + 1] = '\0';
		return;
	}

	ws = _nvp_value_format(nvp->value_ptr,nvp->value_size,&value_format);

	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"nvp_value_format function failed");
		return;
	}
	dbgprint(MOD_MODMGR,__func__,"value format appropriate for this value is %d",value_format);

	/* convert the value to the correct format if it requires any */

	req_text += nvp->name_size;
	*req_text = '=';
	req_text++;

	switch( value_format ) 
	{
		case VALUE_FORMAT_UNQUOTED:
			dbgprint(MOD_MODMGR,__func__,"writing value into req_text (%p), length is %u bytes long",
					req_text,nvp->value_size);
			memcpy(req_text,nvp->value_ptr,nvp->value_size);
			req_text[nvp->value_size] = ' ';
			req_text[nvp->value_size + 1] = '\0';
			return;
		case VALUE_FORMAT_QUOTED:
			*req_text = '"';
			dbgprint(MOD_MODMGR,__func__,"writing value into req_text (%p), length is %u bytes long",
					req_text,nvp->value_size);
			memcpy(req_text+1,nvp->value_ptr,nvp->value_size);
			req_text += nvp->value_size + 1;
			*req_text = '"';
			req_text++;
			*req_text = ' ';
			req_text++;
			*req_text = '\0';
			return;
		case VALUE_FORMAT_ENCODED:
			/* convert the value to a good format */
			ws = _nvp_value_encode(nvp->value_ptr,nvp->value_size,&value_encoded);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_MODMGR,__func__,"unable to convert value to encoded format");
				return;
			}
			req_text = stpcpy(req_text,value_encoded);
			*req_text = ' ';
			*(req_text+1) = '\0';
			free(value_encoded);
			return;
		default:
			dbgprint(MOD_MODMGR,__func__,"invalid or unsupported value format (%d)",value_format);
			return;
	}
}


/*
   _req_from_bin_to_text

   Helper function to convert a request in binary form to text form. This function will be useful
   when modmgr is forwarding a request from a DCR module to a module that is remote.

   Actually its much easier to convert a request from binary to text form.
*/
wstatus
_req_from_bin_to_text(request_t req,request_t *req_text)
{
	request_t req_ptr;
	char req_id[REQIDSIZE+1];
	char req_type[REQTYPESIZE+1];
	char req_src[REQMODSIZE+1];
	char req_dst[REQMODSIZE+1];
	char req_code[REQCODESIZE+1];
	int req_size = 0;
	int nvl_size = 0;
	unsigned int nv_count = 0;
	jmlist_status jmls;
	char *aux;

	dbgprint(MOD_MODMGR,__func__,"called with req=%p, req_text=%p",req,req_text);

	/* validate arguments */

	if( !req ) {
		dbgprint(MOD_MODMGR,__func__,"invalid req argument (req=0)");
		goto return_fail;
	}

	if( !req_text ) {
		dbgprint(MOD_MODMGR,__func__,"invalid req_text argument (req_text=0)");
		goto return_fail;
	}

	if( req->stype != REQUEST_STYPE_BIN ) {
		dbgprint(MOD_MODMGR,__func__,"invalid request to be used with this function");
		goto return_fail;
	}

	/* we need to calculate the request size, because the data structure for it in text form
	   should contain all the request in text form. */

	if( req->data.bin.id > MAXREQID ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) request id exceeds the limit (%d)",req,MAXREQID);
		goto return_fail;
	}

	if( !snprintf(req_id,sizeof(req_id),"%u",req->data.bin.id) ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to convert request id to string",req);
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"(req=%p) id length in chars is %d",req,strlen(req_id));
	req_size += strlen(req_id);

	if( req->data.bin.type == REQUEST_TYPE_REQUEST ) {
		/* request type is request, dont use any char in TYPE token */
		req_type[0] = '\0';
	} else if ( req->data.bin.type == REQUEST_TYPE_REPLY ) {
		/* request type is reply */
		req_type[0] = 'R';
		req_type[1] = '\0';
	} else {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) invalid or unexpected request type",req);
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"(req=%p) type length in chars is %d",req,strlen(req_type));
	req_size += strlen(req_type);

	/* request source, destination and code arrays in binary request don't count with the null
	   char in the end if their lenghts are at maximum, take care of that by copying to a larger
	   array and cat'ing a null char in the end. */
	memcpy(req_src,req->data.bin.src,sizeof(req_src)-1);
	memcpy(req_dst,req->data.bin.dst,sizeof(req_dst)-1);
	memcpy(req_code,req->data.bin.code,sizeof(req_code)-1);
	req_src[REQMODSIZE] = '\0';
	req_dst[REQMODSIZE] = '\0';
	req_code[REQCODESIZE] = '\0';

	dbgprint(MOD_MODMGR,__func__,"(req=%p) source, destination and code length in chars is %d",
			req, strlen(req_src) + strlen(req_dst) + strlen(req_code));

	req_size += strlen(req_src) + strlen(req_dst) + strlen(req_code);
	
	/* take care of nv-pairs */

	jmls = jmlist_entry_count(req->data.bin.nvl,&nv_count);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) jmlist_entry_count failed with jmls=%d",req,jmls);
		goto return_fail;
	}

	if( !nv_count )
		goto skip_nvl_size;

	dbgprint(MOD_MODMGR,__func__,"(req=%p) this request has %d nv-pair(s)",req,nv_count);

	/* count the SEP between CODE and NVL */
	req_size += 1;

	jmls = jmlist_parse(req->data.bin.nvl,_req_bin_nvl_sum_length_jlcb,&nvl_size);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) failed to parse nvpair list (length)",req);
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"(req=%p) nvlist calculated size is %d bytes long",req,nvl_size);
	req_size += nvl_size;

	/* decrement one space, the last one */
	req_size -= 1;

skip_nvl_size:

	/* ok, now, for the header, we need to include the separator chars (spaces)
	   in the request size. ID.T SRC DST CODE[ NVL..] */

	req_size += 3;

	/* plus the space and null char */
	req_size += 1 + 1;

	req_ptr = (request_t)malloc(sizeof(struct _request_t) + req_size);
	memset(req_ptr,0,sizeof(struct _request_t) + req_size);
	dbgprint(MOD_MODMGR,__func__,"(req=%p) allocated a new request ptr=%p",req,req_ptr);

	req_ptr->stype = REQUEST_STYPE_TEXT;

	if( !snprintf(req_ptr->data.text.raw,req_size,"%s%s %s %s %s",req_id,req_type,req_src,req_dst,req_code) )
	{
		dbgprint(MOD_MODMGR,__func__,"(req=%p) failed to fill new request text data",req);
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"(req=%p) request head was built \"%s\"",req,req_ptr->data.text.raw);

	if( !nv_count )
		goto skip_nvl_insert;

	/* NVL INSERT HERE */

	strcat(req_ptr->data.text.raw," ");

	jmls = jmlist_parse(req->data.bin.nvl,_req_bin_nvl_insert_jlcb,
			req_ptr->data.text.raw + strlen(req_ptr->data.text.raw) );
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		/* this would be odd since we've already used the parser before in this function */
		dbgprint(MOD_MODMGR,__func__,"(req=%p) failed to parse nvpair list (insert)",req);
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"(req=%p) finished insertion of %u nv-pairs",req,nv_count);

	/* finish insertion of nv-pairs cut the last space */
	dbgprint(MOD_MODMGR,__func__,"(req=%p) removing trail space character",req);
	if( (aux = strrchr(req_ptr->data.text.raw,' ')) )
		*aux = '\0';

skip_nvl_insert:

	*req_text = req_ptr;
	dbgprint(MOD_MODMGR,__func__,"(req=%p) updated req_text to %p",req,*req_text);

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:

	if( req_ptr ) {
		free(req_ptr);
	}

	DBGRET_FAILURE(MOD_MODMGR);
}

wstatus
_req_from_pipe_to_text(request_t req,request_t *req_text)
{
	return WSTATUS_UNIMPLEMENTED;
}

/*
   _req_diff

   Helper function to dump request differences, usefull for request conversion
   functions testing. Requests must be of the same type.
*/
wstatus
_req_diff(request_t req1_ptr,char *req1_label,request_t req2_ptr,char *req2_label)
{
	char aux_buf1[20] = {'\0'};
	char aux_buf2[20] = {'\0'};
	char reqmod1[REQMODSIZE+1] = {'\0'};
	char reqmod2[REQMODSIZE+1] = {'\0'};
	char reqcode1[REQCODESIZE+1] = {'\0'};
	char reqcode2[REQCODESIZE+1] = {'\0'};
	nvpair_t nvp1,nvp2;
	unsigned int nvcount1,nvcount2;
	jmlist_index nv_idx;

	assert(req1_ptr);
	assert(req1_label);
	assert(req2_ptr);
	assert(req2_label);
	assert(req1_ptr->stype == req2_ptr->stype);

	printf("                | %20s | %20s\n",req1_label,req2_label);
	switch(req1_ptr->stype)
	{
		case REQUEST_STYPE_TEXT:
			strncpy(aux_buf1,req1_ptr->data.text.raw,sizeof(aux_buf1)-1);
			strncpy(aux_buf2,req2_ptr->data.text.raw,sizeof(aux_buf2)-1);
	printf("       raw text | %20s | %20s\n",aux_buf1,aux_buf2); 
			break;
		case REQUEST_STYPE_BIN:
			
			if( req1_ptr->data.bin.type != req2_ptr->data.bin.type ) {
	printf("           type | %20s | %20s\n",
			rtype_str(req1_ptr->data.bin.type),rtype_str(req2_ptr->data.bin.type) );
			} else
	printf("           type | EQUAL\n");

			if( req1_ptr->data.bin.id != req2_ptr->data.bin.id ) {
	printf("             id | %20d | %20d\n",req1_ptr->data.bin.id,req1_ptr->data.bin.id);
			} else
	printf("             id | EQUAL\n");

		memcpy(reqmod1,req1_ptr->data.bin.src,REQMODSIZE);
		memcpy(reqmod2,req2_ptr->data.bin.src,REQMODSIZE);
			if( strcmp(reqmod1,reqmod2) ) {
	printf("         source | %20s | %20s\n",reqmod1,reqmod2);
			} else
	printf("         source | EQUAL\n");

		memcpy(reqmod1,req1_ptr->data.bin.dst,REQMODSIZE);
		memcpy(reqmod2,req2_ptr->data.bin.dst,REQMODSIZE);

			if( strcmp(reqmod1,reqmod2) ) {
	printf("        destiny | %20s | %20s\n",reqmod1,reqmod2);
			} else
	printf("        destiny | EQUAL\n");

		memcpy(reqcode1,req1_ptr->data.bin.code,REQCODESIZE);
		memcpy(reqcode2,req2_ptr->data.bin.code,REQCODESIZE);

			if( strcmp(reqcode1,reqcode2) ) {
	printf("           code | %20s | %20s\n",reqcode1,reqcode2);
			} else
	printf("           code | EQUAL\n");

			/* process nvpairs */

			jmlist_entry_count(req1_ptr->data.bin.nvl,&nvcount1);
			jmlist_entry_count(req2_ptr->data.bin.nvl,&nvcount2);

			if( nvcount1 != nvcount2 ) {
	printf("        nvcount | %20u | %20u\n",nvcount1,nvcount2);
			} else
	printf("        nvcount | EQUAL (%u)\n",nvcount1);

			for( nv_idx = 0 ; nv_idx < MAX(nvcount1,nvcount2) ; nv_idx++ )
			{
	printf("        name %02u | ",nv_idx);
				if( nv_idx < nvcount1 ) {
					jmlist_get_by_index(req1_ptr->data.bin.nvl,nv_idx,(void*)&nvp1);
					memset(aux_buf1,0,sizeof(aux_buf1));
					memcpy(aux_buf1,nvp1->name_ptr,MIN(nvp1->name_size,sizeof(aux_buf1)-1));
					printf("%20s ",aux_buf1);
				} else printf("%20s "," ");

				printf("| ");

				if( nv_idx < nvcount2 ) {
					jmlist_get_by_index(req1_ptr->data.bin.nvl,nv_idx,(void*)&nvp2);
					memset(aux_buf1,0,sizeof(aux_buf1));
					memcpy(aux_buf1,nvp2->name_ptr,MIN(nvp2->name_size,sizeof(aux_buf1)-1));
					printf("%20s ",aux_buf1);
				} else printf("%20s "," ");

				printf("\n");

	printf("           size | ");
				if( nv_idx < nvcount1 )
					printf("%20u ",nvp1->name_size);
				else
					printf("%20s "," ");

				printf("| ");

				if( nv_idx < nvcount2 )
					printf("%20u ",nvp2->name_size);
				else
					printf("%20s "," ");

				printf("\n");

	printf("       value %02u | ",nv_idx);
				if( (nv_idx < nvcount1) && nvp1->value_size ) {
					memset(aux_buf1,0,sizeof(aux_buf1));
					memcpy(aux_buf1,nvp2->value_ptr,MIN(nvp2->value_size,sizeof(aux_buf1)-1));
					printf("%20s ",aux_buf1);
				} else printf("%20s "," ");

				printf("| ");

				if( (nv_idx < nvcount2) && nvp2->value_size ) {
					strncpy(aux_buf1,nvp1->value_ptr,sizeof(aux_buf1)-1);
					printf("%20s ",aux_buf1);
				} else printf("%20s "," ");

				printf("\n");

	printf("           size | ");
				if( nv_idx < nvcount1 )
					printf("%20u ",nvp1->value_size);
				else
					printf("%20s "," ");

				printf("| ");

				if( nv_idx < nvcount2 )
					printf("%20u ",nvp2->value_size);
				else
					printf("%20s "," ");

				printf("\n");

			}
		break;
		case REQUEST_STYPE_PIPE:
			break;
		default:
			break;
	}
	return WSTATUS_SUCCESS;
}

/*
   _req_from_string

   Helper function that builds a TEXT type request from a string.
   The string should contain all the request, this string pointer
   wont be referenced in the request so the client code might free it.
*/
wstatus
_req_from_string(const char *raw_text,request_t *req_text)
{
	request_t req = 0;
	size_t req_size;

	dbgprint(MOD_MODMGR,__func__,"called with raw_text=%p, req_text=%p",raw_text,req_text);

	if( !raw_text ) {
		dbgprint(MOD_MODMGR,__func__,"invalid raw_text argument (raw_text=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( !req_text ) {
		dbgprint(MOD_MODMGR,__func__,"invalid req_text argument (req_text=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	req_size = strlen(raw_text) + sizeof(struct _request_t) + 1;
	req = (request_t)malloc(req_size);
	if( !req ) {
		dbgprint(MOD_MODMGR,__func__,"malloc failed for size %d",req_size);
		DBGRET_FAILURE(MOD_MODMGR);
	}

	dbgprint(MOD_MODMGR,__func__,"allocated request memory successfully (ptr=%p)",req);
	memset(req,0,req_size);
	req->stype = REQUEST_STYPE_TEXT;
	strcpy(req->data.text.raw,raw_text);

	*req_text = req;
	dbgprint(MOD_MODMGR,__func__,"updated req_text value to %p",*req_text);

	DBGRET_SUCCESS(MOD_MODMGR);

/*
return_fail:
	if( req )
		free(req);

	DBGRET_FAILURE(MOD_MODMGR); */
}

/*
   _request_build_error_reply

   This helper function will allocate a new request data structure that will be used
   as a reply to a request. Its important to notice that not all the information is filled
   in the request data structure. The request created has binary stype.

   The client is responsible for filling the folowing informations:
    - request ID
	- destination module
	- source module
*/
wstatus
_request_build_error_reply(const char *error_code,const char *error_description,request_t *req)
{
	request_t aux_req;
	wstatus ws;

	dbgprint(MOD_MODMGR,__func__,"called with error_code=\"%s\", error_description=\"%s\", req=%p",
			z_ptr(error_code),z_ptr(error_description),req);

	if( !error_code || !strlen(error_code) ) {
		dbgprint(MOD_MODMGR,__func__,"invalid error_code argument (error_code=0 or strlen(error_code)=0)");
		goto return_fail;
	}

	/* allocate new request data structure */

	aux_req = (request_t)malloc(sizeof(struct _request_t));
	aux_req->stype = REQUEST_STYPE_BIN;
	aux_req->data.bin.type = REQUEST_TYPE_REPLY;
	aux_req->data.bin.id = 0;
	memset(aux_req->data.bin.src,'\0',sizeof(aux_req->data.bin.src));
	memset(aux_req->data.bin.dst,'\0',sizeof(aux_req->data.bin.dst));
	if( strlen(error_code) > sizeof(aux_req->data.bin.code) ) {
		dbgprint(MOD_MODMGR,__func__,"error_code length is overlimit (max is %d, required are %d)",
				sizeof(aux_req->data.bin.code),strlen(error_code));
		goto return_fail;
	}
	memcpy(aux_req->data.bin.code,error_code,strlen(error_code));
	aux_req->data.bin.nvl = 0;

	/* error description is not mandatory when the error code is self-describing...
	   tho I'd always send some error description... */

	if( error_description && strlen(error_description) )
	{
		ws = _req_add_nvp_z(REQERROR_DESCNAME,error_description,aux_req);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"failed to add nvpair to request (req=%p)",aux_req);
			goto return_fail;
		}
		dbgprint(MOD_MODMGR,__func__,"added error description successfully to the request");
	} else
		dbgprint(MOD_MODMGR,__func__,"no error description detected, not adding it to the request");

	/* request is ready to be returned */

	*req = aux_req;
	dbgprint(MOD_MODMGR,__func__,"updated req value to %p",*req);

	DBGRET_SUCCESS(MOD_MODMGR);
return_fail:
	if( aux_req )
		_req_free(aux_req);

	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _request_process

   When a request is received by the modmgr module, it should:
   1) lookup destination module in loaded module list
   2.1) if module is not found send error reply to the source of the request.
   2.2) if module is found, forward request to it.

   When the module is registered it also indicates how the modmgr should
   communicate with it: by a callback or using a wchannel.
*/
wstatus
_request_process(request_t req)
{
	const struct _modreg_t *mod_src = 0;
	const struct _modreg_t *mod_dst = 0;
	request_t reply = 0;
	wstatus ws;

	dbgprint(MOD_MODMGR,__func__,"called with req=%p",req);

	ws = modmgr_lookup(array2z(req->data.bin.src,sizeof(req->data.bin.src)),&mod_src);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"failed to lookup module (ws=%s)",wstatus_str(ws));
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"found source module in registered modules list");

	ws = modmgr_lookup(req->data.bin.dst,&mod_dst);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"failed to lookup module (ws=%s)",wstatus_str(ws));
		goto dest_not_found;
	}
	dbgprint(MOD_MODMGR,__func__,"found destination module in registered modules list");

dest_not_found:
	
	/* reply with error message */
	ws = _request_build_error_reply(REQERROR_DESCNAME,REQERROR_MODUNFOUND,&reply);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"failed to create error reply (ws=%s)",wstatus_str(ws));
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"created error reply successfully");

	ws = _request_send(reply,mod_dst);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"failed to send reply (ws=%s)",wstatus_str(ws));
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"sent error reply successfully");

	/* free reply */
	_req_free(reply);

	if( mod_src )
		_modreg_free(mod_src);

	if( mod_dst )
		_modreg_free(mod_dst);

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:
	if( reply )
		_req_free(reply);

	if( mod_src )
		_modreg_free(mod_src);

	if( mod_dst )
		_modreg_free(mod_dst);

	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _request_processor_thread

   Thread callback which will receive both SSR and DCR and process them.
   This thread has three parts, initialization, processing and cleanup.
   
   During initialization the function allocates required data structure
   for futher processing. This includes the request buffer used for reading
   requests that come from SSR and DCR. This function receives a init
   data structure which contains some variables that enable the interface
   between this thread function and the rest of the modmgr module.
   There must be a way of stoping the thread, to do this a variable is
   passed inside this init data structure (unloading bool) which should
   be tested whenever possible in the processing loop.
   
   The processing part includes a loop which reads from the fast wchannel
   (for instance, a PIPE) the requests are expected to be in binary form.
   To read from this wchannel it uses the reqbuf_t created during the
   initialization. The processing loop breaks when unloading flag is set.

   The function reaches the cleanup part when the processing loop breaks,
   this part is responsible for freeing the allocated data structures
   used by the function, this includes the reqbuf_t.

   Since the wchannel object doesn't support timeouts whenever the client
   code wants to unload the modmgr module it should: i) set unloading flag,
   ii) send a dummy request code. Whenever a request arrives this function
   first checks if the module is unloading before processing the request.
*/
void _request_processor_thread(void *param)
{
	reqbuf_t rb = 0;
	request_t req;
	wstatus ws;
	request_proc_data_t *proc_data = (request_proc_data_t*)param;

	dbgprint(MOD_MODMGR,__func__,"called with param=%p");

	if( !param ) {
		dbgprint(MOD_MODMGR,__func__,"invalid param argument (param=0)");
		goto return_fail;
	}

	/* create request buffer */

	ws = reqbuf_create(reqbuf_wchannel_read_cb,proc_data->recv_wch,REQBUF_TYPE_BINARY,&rb);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"failed to create new reqbuf (ws=%s)",wstatus_str(ws));
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"created request buffer successfully (rb=%p)",rb);

	/* initialization part is finished, toggle flag */
	proc_data->initialized_flag = true;

	/* start processing part */
	for(;;)
	{
		ws = reqbuf_read(rb,&req);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"failed to read from request buffer (rb=%p, ws=%s)",
					rb,wstatus_str(ws));
			goto return_fail;
		}
		dbgprint(MOD_MODMGR,__func__,"received request id %u",req->data.bin.id);

		if( proc_data->unload_flag ) {
			/* dont process request if we're unloading... */
			dbgprint(MOD_MODMGR,__func__,"unload flag is set, finishing thread");
			goto finish_thread;
		}

		/* call helper function to process request */
		dbgprint(MOD_MODMGR,__func__,"calling helper function _request_process");
		ws = _request_process(req);
		dbgprint(MOD_MODMGR,__func__,"helper function _request_process returned ws=%s",wstatus_str(ws));
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"unable to process request (ws=%s)",wstatus_str(ws));
			goto return_fail;
		}
		dbgprint(MOD_MODMGR,__func__,"request processed successfully");
	}

return_success:

	dbgprint(MOD_MODMGR,__func__,"returning with success.");
	proc_data->ret_status = WSTATUS_SUCCESS;
	proc_data->finished_flag = true;
	return;

finish_thread:
	/* destroy request buffer */
	ws = reqbuf_destroy(rb);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"failed to destroy request buffer (reqbuf=%p, ws=%s)",
				rb,wstatus_str(ws));
		goto return_fail;
	}

	/* done cleanup, leave thread. */
	goto return_success;

return_fail:
	dbgprint(MOD_MODMGR,__func__,"returning with failure.");
	proc_data->ret_status = WSTATUS_FAILURE;
	proc_data->finished_flag = true;
	return;
}

/*
   _req_add_nvp_z

   Helper function of the req family for adding a nv-pair to the request.
   The name isn't checked for existence in the nv-pair list. When there is
   no value associated, use value_ptr=0. Both name_ptr and value_ptr are
   expected to be null terminated strings. The null characters are not included
   in the request nvpair NAME and VALUE.
*/
wstatus
_req_add_nvp_z(const char *name_ptr,const char *value_ptr,request_t req)
{
	unsigned int value_size;
	nvpair_t nvp = 0;
	jmlist jml = 0;
	jmlist_status jmls;
	wstatus ws;
	struct _jmlist_params params = {.flags = JMLIST_LINKED};
	unsigned int entry_count;

	dbgprint(MOD_MODMGR,__func__,"called with name_ptr=%p, value_ptr=%p, req=%p",
			name_ptr,value_ptr,req);

	if( !name_ptr || !strlen(name_ptr) ) {
		dbgprint(MOD_MODMGR,__func__,"invalid name_ptr argument (name_ptr=0 or strlen(name_ptr)=0)");
		goto return_fail;
	}

	if( !req ) {
		dbgprint(MOD_MODMGR,__func__,"invalid req argument (req=0)");
		goto return_fail;
	}

	if( req->stype == REQUEST_STYPE_BIN )
	{
		value_size = value_ptr ? strlen(value_ptr) : 0;
		ws = _nvp_alloc(strlen(name_ptr),value_size,&nvp);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"failed to allocated new nvpair (ws=%s)",wstatus_str(ws));
			goto return_fail;
		}
		dbgprint(MOD_MODMGR,__func__,"allocated new nvpair (nvp=%p) successfully",nvp);

		ws = _nvp_fill(name_ptr,strlen(name_ptr),value_ptr,value_size,nvp);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"failed to fill the new nvpair (ws=%s)",wstatus_str(ws));
			goto return_fail;
		}
		dbgprint(MOD_MODMGR,__func__,"filled the new nvpair successfully");

		if( !req->data.bin.nvl )
	   	{
			/* allocate new jmlist for the nvpairs */
			jmls = jmlist_create(&jml,&params);
			if( jmls != JMLIST_ERROR_SUCCESS ) {
				dbgprint(MOD_MODMGR,__func__,"failed to create jmlist (jmls=%d)",jmls);
				jml = 0;
				goto return_fail;
			}
			dbgprint(MOD_MODMGR,__func__,"created jmlist for the nvl successfully (jml=%p)",jml);
			req->data.bin.nvl = jml;
			dbgprint(MOD_MODMGR,__func__,"updated request nvl to %p",req->data.bin.nvl);
		}

		jmls = jmlist_insert(jml,nvp);
		if( jmls != JMLIST_ERROR_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"failed to insert nvpair into nvl (jmls=%d)",jmls);
			goto return_fail;
		}

		entry_count = 0;
		jmls = jmlist_entry_count(jml,&entry_count);
		dbgprint(MOD_MODMGR,__func__,"inserted nvpair into nvl successfully (nvl has now %u entries)",entry_count);

	} else if( req->stype == REQUEST_STYPE_TEXT ) {
		/* TODO */
	} else {
		dbgprint(MOD_MODMGR,__func__,"invalid or unsupported request stype (%d)",req->stype);
		goto return_fail;
	}
	DBGRET_SUCCESS(MOD_MODMGR);
return_fail:
	if( jml )
		jmlist_free(jml);

	if( nvp )
		_nvp_free(nvp);

	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _req_free

   As the name says, this function frees any data structure associated to the request.
*/
wstatus
_req_free(request_t req)
{
	jmlist_status jmls;

	dbgprint(MOD_MODMGR,__func__,"called with req=%p");
	if( !req ) {
		dbgprint(MOD_MODMGR,__func__,"invalid req argument (req=0)");
		goto return_fail;
	}

	switch(req->stype)
	{
		case REQUEST_STYPE_BIN:
			/* binary requests have an aditional data structure allocated which is the
			   jmlist that contains the list of nv-pairs. This must be freed using the
			   own jmlist APIs. */
			if( req->data.bin.nvl ) {
				jmls = jmlist_free(req->data.bin.nvl);
				if( jmls != JMLIST_ERROR_SUCCESS ) {
					dbgprint(MOD_MODMGR,__func__,"failed to free request nv-pair list (jmls=%d",jmls);
					/* what should we do here? free the request anyway?! something was bad implemented.. */
					goto return_fail;
				}
			}
			break;
		case REQUEST_STYPE_TEXT:
			/* text requests have an array that contains the request in raw characters,
			   this array is part of request data structure and is not a pointer to an
			   external buffer, meaning there are no aditional buffers to free. */
			break;
		case REQUEST_STYPE_PIPE:
			/* unsupported for now */
		default:
			dbgprint(MOD_MODMGR,__func__,"invalid or unsupported request type (%d)",req->stype);
			goto return_fail;
	}

	/* free the request data structure */
	dbgprint(MOD_MODMGR,__func__,"freeing request data structure");
	free(req);

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:
	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   modreg_create_from_request

   Helper function that converts a request with destination 'modmgr' and
   code 'registryModule' into a module registration data structure.
*/
wstatus
modreg_create_from_request(request_t req,modreg_t *modreg)
{
	return WSTATUS_UNIMPLEMENTED;
}

/*
   _modreg_alloc

   Helper function to allocate a new module registry data structure.
*/
wstatus _modreg_alloc(modreg_t *mod)
{
	modreg_t new_mod;
	dbgprint(MOD_MODMGR,__func__,"called with mod=%p",mod);

	if(!mod) {
		dbgprint(MOD_MODMGR,__func__,"invalid mod argument (mod=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	new_mod = (modreg_t)malloc(sizeof(struct _modreg_t));
	if( !new_mod ) {
		dbgprint(MOD_MODMGR,__func__,"malloc failed");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	dbgprint(MOD_MODMGR,__func__,"allocated memory for modreg_t data structure successfully (ptr=%p)",new_mod);

	dbgprint(MOD_MODMGR,__func__,"filling modreg_t data structure");
	memset(new_mod->basic.name,'\0',sizeof(new_mod->basic.name));
	memset(new_mod->basic.description,'\0',sizeof(new_mod->basic.description));
	memset(new_mod->basic.version,'\0',sizeof(new_mod->basic.version));
	memset(new_mod->basic.author_name,'\0',sizeof(new_mod->basic.author_name));
	memset(new_mod->basic.author_email,'\0',sizeof(new_mod->basic.author_email));

	new_mod->communication.type = MODREG_COMM_UNDEF;
	new_mod->communication.data.dcr.reqproc_cb = 0;
	memset(new_mod->communication.data.ssr.host,'\0',sizeof(new_mod->communication.data.ssr.host));
	memset(new_mod->communication.data.ssr.port,'\0',sizeof(new_mod->communication.data.ssr.port));
	dbgprint(MOD_MODMGR,__func__,"finished filling of new modreg_t data structure");

	*mod = new_mod;
	dbgprint(MOD_MODMGR,__func__,"updated mod value to %p",*mod);

	DBGRET_SUCCESS(MOD_MODMGR);
}

/*
   _modreg_free

   Helper function to free a single module registry data structure from memory.
   The data structure must have been allocated using _modreg_alloc function.
*/
wstatus _modreg_free(const struct _modreg_t *mod)
{
	dbgprint(MOD_MODMGR,__func__,"called with mod=%p");

	if(!mod) {
		dbgprint(MOD_MODMGR,__func__,"invalid mod argument (mod=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	free((void*)mod);

	dbgprint(MOD_MODMGR,__func__,"freed module registry data structure successfully (ptr=%p)",mod);

	DBGRET_SUCCESS(MOD_MODMGR);
}

/*
   _modmgr_reqproc_cb

   This is the callback that receives the requests which have as destiny module the 'modmgr'.
   Its one of the rare functions where modmgr doesn't give a damn if goes wrong so it doesn't
   provide any means for passing an error status code.
   
   The module manager accepts several request codes:

   moduleRegister	registers a new module into modmgr, this will make it possible to receive
					and send requests to the new module. This should be the first message to be sent from
					any module that wants to be part of running wicom. There are some nvpairs required
					for this request code:
						name = module name
						description = module description
						version = module version
						authorName = author name
						authorEmail = author email
						dependencies = other module list ex: "cfgmgr,datastor"
   
   moduleUnregister unregisters the previously registered calling module (the module to unregister
					is identified by the source of the message.

   moduleLookup		Checks if a module exists in the list of registered modules. Reply is error
					or success depending on the lookup result.
*/
void _modmgr_reqproc_cb(const request_t req)
{
	/* for now it will just dump the request.. */
	_req_dump(req);
}

/*
   modmgr_load

   Initializes modmgr module. This function must be called before the client
   code can use the modmgr. It should do the following things:
   - create jmlist for registered modules
   - insert the 'modmgr' module into the registered modules
   - create the bind wchannel and the pipe wchannel used in modmgr threads.
   - create the required threads for modmgr, this includes _request_processor and
     _remote_request_receiver threads.
   - wait for threads initialization.

TODO: code related to _remote_request_receiver thread.
*/
wstatus
modmgr_load(modmgr_load_t load)
{
	struct _jmlist_params params = { .flags = JMLIST_LINKED };
	jmlist_status jmls;
	wstatus ws;
	wchannel_opt_t fast_wch_opt; 
	wchannel_t fast_wch = 0;
	modreg_t modmgr_reg = 0;
	
	dbgprint(MOD_MODMGR,__func__,"called with load.bind_hostname=\"%s\", load.bind_port=%s",
			z_ptr(load.bind_hostname),z_ptr(load.bind_port));

	/* create jmlist for registered modules */

	jmls = jmlist_create(&mod_list,&params);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"failed to create jmlist (jmls=%d)",jmls);
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"created new jmlist for registered modules successfully (jml=%p)",mod_list);

	/* allocate new modmgr_reg */

	ws = _modreg_alloc(&modmgr_reg);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"failed to allocate new module registry data structure (ws=%s)",wstatus_str(ws));
		modmgr_reg = 0; /* _modreg_alloc shouldn't have touched it, anyways.. */
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"allocated new module registry data structure successfully (ptr=%p)",modmgr_reg);

	/* fill the data structure information */
	
	strcpy(modmgr_reg->basic.name,"modmgr");
	strcpy(modmgr_reg->basic.description,"The module manager from wicom itself.");
	strcpy(modmgr_reg->basic.version,"0.1");
	strcpy(modmgr_reg->basic.author_name,"Jean-François Mousinho");
	strcpy(modmgr_reg->basic.author_email,"jean.mousinho@ist.utl.pt");
	
	modmgr_reg->communication.type = MODREG_COMM_DCR;
	modmgr_reg->communication.data.dcr.reqproc_cb = _modmgr_reqproc_cb;

	dbgprint(MOD_MODMGR,__func__,"all new module registry data structure was filled OK (ptr=%p)",modmgr_reg);

	/* insert this module data structure into the jmlist */
	
	jmls = jmlist_insert(mod_list,modmgr_reg);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"failed to insert new module into module registry list (jmls=%d)",jmls);
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"new module was inserted into module registry list successfully (ptr=%p)",modmgr_reg);

	/* create new wchannel for communication with request_processor thread */

	fast_wch_opt.type = WCHANNEL_TYPE_PIPE;
	fast_wch_opt.host_src = NULL;
	fast_wch_opt.port_src = NULL;
	fast_wch_opt.host_dst = NULL;
	fast_wch_opt.port_dst = NULL;
	fast_wch_opt.debug_opts = WCHANNEL_NO_DEBUG;
	fast_wch_opt.dump_cb = 0;
	fast_wch_opt.buffer_size = 0;
	dbgprint(MOD_MODMGR,__func__,"filled fast wchannel data structure, creating wchannel");

	ws = wchannel_create(&fast_wch_opt,&fast_wch);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"failed to create wchannel (ws=%s)",wstatus_str(ws));
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"created new wchannel successfully (wch=%p)",fast_wch);

	/* initialize thread_reqproc_data which is the _request_processor thread interface data */

	thread_reqproc_data.unload_flag = false;
	thread_reqproc_data.initialized_flag = false;
	thread_reqproc_data.finished_flag = false;
	thread_reqproc_data.recv_wch = fast_wch;
	dbgprint(MOD_MODMGR,__func__,"initialized thread_reqproc_data for _request_processor thread");

	/* create _request_processor therad */

	ws = wthread_create(_request_processor_thread,&thread_reqproc_data,&thread_reqproc_data.wthread);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"failed to create wthread (ws=%s)",wstatus_str(ws));
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"created wthread for request processor successfully (wth=%p)",
			thread_reqproc_data.wthread);

	dbgprint(MOD_MODMGR,__func__,"going to wait for request processor thread initialization");

	while(!thread_reqproc_data.initialized_flag);

	if( thread_reqproc_data.finished_flag ) {
		dbgprint(MOD_MODMGR,__func__,"request processor thread failed to initialize (ws=%s)",
				wstatus_str(thread_reqproc_data.ret_status));

		/* wait for thread to RETN */
		dbgprint(MOD_MODMGR,__func__,"waiting for request processor thread to finish");
		wthread_wait(thread_reqproc_data.wthread);
		dbgprint(MOD_MODMGR,__func__,"request processor thread has finished");

		/* clear wthread */
		thread_reqproc_data.wthread = 0;
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"request processor thread initialized successfully");

	dbgprint(MOD_MODMGR,__func__,"updating loaded flag to TRUE");
	loaded = true;

	DBGRET_SUCCESS(MOD_MODMGR);
return_fail:

	/* free registered modules jmlist object */
	if( mod_list )
   	{
		jmls = jmlist_free(mod_list);
		if( jmls != JMLIST_ERROR_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"failed to free mod_list jmlist (jmls=%d)",jmls);
		} else {
			dbgprint(MOD_MODMGR,__func__,"registered modules list was freed successfully (jml=%p)",mod_list);
			mod_list = 0;
		}
	}

	/* free the modmgr registry data structure */
	if( modmgr_reg ) {
		free(modmgr_reg);
		modmgr_reg = 0;
	}

	/* free the fast wchannel */
	if( fast_wch ) 
	{
		ws = wchannel_destroy(fast_wch);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"failed to free fast wchannel (ws=%s)",wstatus_str(ws));
		} else {
			fast_wch = 0;
		}
	}

	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   modmgr_unload

   Uninitializes the modmgr module, this includes:
    - signal all theads to finish, using the unload_flag.
	- wait for all threads to finish using wthread_wait
	- free any data structure used by modmgr and clear the pointers to
	  these data structures (mod_list).
*/
wstatus
modmgr_unload(void)
{
	char *game_over_z = "game over code";
	wstatus ws;
	unsigned int used,mod_count;
	jmlist_status jmls;
	modreg_t mod_ptr;

	dbgprint(MOD_MODMGR,__func__,"called");

	if( !loaded ) {
		dbgprint(MOD_MODMGR,__func__,"module was not loaded yet");
		goto return_fail;
	}

	unloading = true;

	/* flag the thread to unload */

	thread_reqproc_data.unload_flag = true;

	/* send the dummy data to the thread */

	ws = wchannel_send(thread_reqproc_data.recv_wch,NULL,game_over_z,sizeof(game_over_z),&used);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"failed to send game over message to request processor thread (ws=%s)",
				wstatus_str(ws));
		/* this is dirty nasty fail... */
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"sent game over message to request processor thread successfully");

	/* wait for the thread to receive the message and finish */

	dbgprint(MOD_MODMGR,__func__,"waiting on request processor thread to finish");
	wthread_wait(thread_reqproc_data.wthread);
	dbgprint(MOD_MODMGR,__func__,"request processor thread finished (ws=%s)",wstatus_str(thread_reqproc_data.ret_status));
	thread_reqproc_data.wthread = 0;

	/* destroy the communications channel to this thread */
	ws = wchannel_destroy(thread_reqproc_data.recv_wch);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"failed to destroy reception wchannel of request processor thread");
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"request processor wchannel destroyed successfully");
	thread_reqproc_data.recv_wch = 0;

	/* free all modules inside the registered modules list */
	
	jmls = jmlist_entry_count(mod_list,&mod_count);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"failed to get number of registered modules (jmls=%d)",jmls);
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"freeing %u modules from the registered modules list",mod_count);

	while(mod_count--)
	{
		jmls = jmlist_pop(mod_list,(void*)&mod_ptr);
		if( jmls != JMLIST_ERROR_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"failed to pop an entry from registered modules list (jmls=%d)",jmls);
			goto return_fail;
		}

		dbgprint(MOD_MODMGR,__func__,"freeing module (%s) from the registered modules list",
				array2z(mod_ptr->basic.name,sizeof(mod_ptr->basic.name)));

		free(mod_ptr);
	}
	dbgprint(MOD_MODMGR,__func__,"freeing registered modules list object");
	jmls = jmlist_free(mod_list);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"failed to free jmlist (jmls=%d)",jmls);
		goto return_fail;
	}
	/* clear pointer */
	mod_list = 0;

	/* everything went OK .. */

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:
	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _req_lookup_nv

   Helper function that searches a name-value pair in a request, it uses the name as the needle.
   The name-value pair is returned in a nvpair_t structure, the client passes the address of
   a variable that receives a nvpair_t pointer. The client is responsible for freeing the nvp
   data structure returned using _nvp_free.
*/
wstatus
_req_lookup_nv(const request_t req,const char *name_ptr,unsigned int name_size,nvpair_t *nvpp)
{
	wstatus ws;

	dbgprint(MOD_MODMGR,__func__,"called with req=%p, name_ptr=%p (%s), name_size=%u, nvpp=%p",
			req,name_ptr,array2z(name_ptr,name_size),name_size,nvpp);

	/* start by validating the arguments */

	if( !req ) {
		dbgprint(MOD_MODMGR,__func__,"invalid req argument (req=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( !name_ptr ) {
		dbgprint(MOD_MODMGR,__func__,"invalid name_ptr argument (name_ptr=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( !name_size ) {
		dbgprint(MOD_MODMGR,__func__,"invalid name_size (name_size=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	switch(req->stype)
	{
		case REQUEST_STYPE_TEXT:
			dbgprint(MOD_MODMGR,__func__,"request in req=%p is of TEXT stype, calling helper function _req_text_get_nv");
			ws = _req_text_get_nv(req,name_ptr,name_size,nvpp);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_MODMGR,__func__,"helper function _req_text_get_nv failed (ws=%d)",ws);
				goto return_fail;
			}
			dbgprint(MOD_MODMGR,__func__,"helper function _req_text_get_nv was successful");
			break;
		case REQUEST_STYPE_BIN:
			dbgprint(MOD_MODMGR,__func__,"request in req=%p is of BIN stype, calling helper function _req_bin_get_nv");
			ws = _req_bin_get_nv(req,name_ptr,name_size,nvpp);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_MODMGR,__func__,"helper function _req_bin_get_nv failed (ws=%d)");
				goto return_fail;
			}
			dbgprint(MOD_MODMGR,__func__,"helper function _req_bin_get_nv was successful");
			break;
		case REQUEST_STYPE_PIPE:
		default:
			dbgprint(MOD_MODMGR,__func__,"invalid or unsupported request stype (%d)",req->stype);
			DBGRET_FAILURE(MOD_MODMGR);
	}

	DBGRET_SUCCESS(MOD_MODMGR);
return_fail:
	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _req_text_get_nv

   Helper function to get a specific name-value pair value data. This function will return the original
   decoded data. If the name-value pair isn't found or the request doesn't have any name-value pair
   this function returns error. Since this function is processing a request in text form, it starts
   by calling the text token seeker function to seek to the name-value list. After that it parses each
   name-value pair using _req_text_nv_parse which returns name/value pointers, using that information
   it compares the name with look_name, if its equal the name-value pair was found and the value
   is processed.
*/
wstatus
_req_text_get_nv(const request_t req,const char *look_name_ptr,unsigned int look_name_size,nvpair_t *nvpp)
{
	unsigned int nv_idx;
	char *aux;
	char *value_start,*decoded_ptr,*name_start;
	unsigned int name_size, value_size, decoded_size;
	wstatus ws;
	value_format_list value_format;
	nvpair_t aux_nvp = 0;

	dbgprint(MOD_MODMGR,__func__,"called with req=%p, name_ptr=%p (%s), name_size=%u, nvpp=%p",
			req,look_name_ptr,array2z(look_name_ptr,look_name_size),look_name_size,nvpp);

	/* start by validating the arguments */

	if( !req ) {
		dbgprint(MOD_MODMGR,__func__,"invalid req argument (req=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( !look_name_ptr ) {
		dbgprint(MOD_MODMGR,__func__,"invalid look_name_ptr argument (look_name_ptr=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( !look_name_size ) {
		dbgprint(MOD_MODMGR,__func__,"invalid look_name_size (look_name_size=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( req->stype != REQUEST_STYPE_TEXT ) {
		dbgprint(MOD_MODMGR,__func__,"invalid request to be used with this function");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	/* seek to name-value pair list in the request */

	ws = _req_text_token_seek(req->data.text.raw,TEXT_TOKEN_NVL,&aux);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to find nvl begin",req);
		goto return_fail;
	}

	for( nv_idx = 1 ; ; nv_idx++ )
   	{
		/* try to parse nvpair text tokens */
		ws = _req_text_nv_parse(aux,&name_start,&name_size,&value_start,&value_size,&value_format);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"(req=%p) failed to process text request",req);
			goto return_fail;
		}


		if( name_size != look_name_size ) {
			dbgprint(MOD_MODMGR,__func__,"(req=%p) this nvpair has different name size, seeking",req);
			goto seek_nvp;
		}

		/* compare name with the lookup name */

		if( memcmp(name_start,look_name_ptr,name_size) ) {
			dbgprint(MOD_MODMGR,__func__,"(req=%p) this nvpair has different name, seeking",req);
			goto seek_nvp;
		}
		dbgprint(MOD_MODMGR,__func__,"(req=%p) nvpair was found successfully",req);

		if( value_format == VALUE_FORMAT_ENCODED )
	   	{
			ws = _nvp_value_decoded_size(value_start,value_size,&decoded_size);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_MODMGR,__func__,"(req=%p) failed to get decoded size for nv_idx=%u",req,nv_idx);
				goto return_fail;
			}

			/* allocate buffer that will store the decoded value */

			decoded_ptr = (char*)malloc(decoded_size);
			if( !decoded_ptr ) {
				dbgprint(MOD_MODMGR,__func__,"(req=%p) malloc failed on decoded_size=%u",req,decoded_size);
				goto return_fail;
			}
			dbgprint(MOD_MODMGR,__func__,"(req=%p) allocated buffer for decoded value successful (ptr=%p)",
					req,decoded_ptr);

			ws = _nvp_value_decode(value_start,value_size,decoded_ptr,decoded_size);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to decode value (ws=%s)",req,wstatus_str(ws));
				goto return_fail;
			}
			dbgprint(MOD_MODMGR,__func__,"(req=%p) decoded successfully the value",req);
		}

		/* allocate new nvpair data structure for this nvpair */

		if( value_format == VALUE_FORMAT_ENCODED )
			ws = _nvp_alloc(name_size,decoded_size,&aux_nvp);
		else
			ws = _nvp_alloc(name_size,value_size,&aux_nvp);

		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"(req=%p) failed to allocate a new nvpair (name_size=%u, value_size=%u)",
					req,name_size,value_size);
			goto return_fail;
		}
		dbgprint(MOD_MODMGR,__func__,"(req=%p) allocated new nvpair data structure successfully (p=%p)",req,aux_nvp);

		/* fill the new data structure with the tokens */
		
		if( value_format == VALUE_FORMAT_ENCODED )
			ws = _nvp_fill(name_start,name_size,decoded_ptr,decoded_size,aux_nvp);
		else
			ws = _nvp_fill(name_start,name_size,value_start,value_size,aux_nvp);

		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"(req=%p) failed to fill the new nvpair (p=%p)",req,aux_nvp);
			goto return_fail;
		}
		dbgprint(MOD_MODMGR,__func__,"(req=%p) new nvpair was filled successfully",req);

		/* we can now free the decoded buffer if it was used */
		if( decoded_ptr ) {
			free(decoded_ptr);
			decoded_ptr = 0;
			decoded_size = 0;
		}

		/* return this nvpair to the calling function */

		*nvpp = aux_nvp;
		dbgprint(MOD_MODMGR,__func__,"(req=%p) updated nvpp value to %p",req,*nvpp);
		DBGRET_SUCCESS(MOD_MODMGR);

seek_nvp:
		if( value_start ) {
			aux = value_start + value_size;
			if( V_QUOTECHAR(*aux) )
				aux++;
		} else
			 aux = name_start + name_size;

		if( V_REQENDCHAR(*aux) ) {
			/* end of request was reached */
			dbgprint(MOD_MODMGR,__func__,"(req=%p) end of nvpair list");
			break;
		}

		if( *aux != ' ' ) {
			dbgprint(MOD_MODMGR,__func__,"(req=%p) [nv_idx=%u] invalid/unexpected character after nvpair",req,nv_idx);
			goto return_fail;
		}

		aux++;
	}

	/* parsed all the nvpairs but didn't found the wanted nvp */

	dbgprint(MOD_MODMGR,__func__,"(req=%p) the nvpair with name \"%s\" was not found in this request",
			req,array2z(look_name_ptr,look_name_size));
	goto return_fail;

return_fail:
	if( decoded_ptr )
		free(decoded_ptr);

	if( aux_nvp ) {
		_nvp_free(aux_nvp);
	}

	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _req_get_nv_count

   Helper function to get number of name-value pairs that a request has. The
   number of name-value pairs returned is one-based. If the request doesn't
   have any name-value pair, nv_count is set to 0 (zero).
*/
wstatus
_req_get_nv_count(const request_t req,unsigned int *nv_count)
{
	wstatus ws;
	jmlist_status jmls;
	unsigned int entry_count;

	dbgprint(MOD_MODMGR,__func__,"called with req=%p, nv_count=%p",req,nv_count);

	/* validate arguments */

	if( !req ) {
		dbgprint(MOD_MODMGR,__func__,"invalid req argument (req=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( !nv_count ) {
		dbgprint(MOD_MODMGR,__func__,"invalid nv_count argument (nv_count=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	switch(req->stype)
	{
		case REQUEST_STYPE_TEXT:
			
			dbgprint(MOD_MODMGR,__func__,"(req=%p) request type is text",req);

			dbgprint(MOD_MODMGR,__func__,"(req=%p) calling helper function",req);
			ws = _req_text_nv_count(req,&entry_count);
			dbgprint(MOD_MODMGR,__func__,"(req=%p) helper function returned (ws=%d)",req,ws);

			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_MODMGR,__func__,"(req=%p) failed to get request nv-count (helper function failed with ws=%s)",
						req,wstatus_str(ws));
				DBGRET_FAILURE(MOD_MODMGR);
			}
			dbgprint(MOD_MODMGR,__func__,"(req=%p) helper function was successful (%u)",req,entry_count);

			*nv_count = entry_count;
			dbgprint(MOD_MODMGR,__func__,"(req=%p) nv_count value updated to %u",*nv_count);
			DBGRET_SUCCESS(MOD_MODMGR);
			
			break;	
		case REQUEST_STYPE_BIN:

			dbgprint(MOD_MODMGR,__func__,"(req=%p) request stype is binary",req);

			dbgprint(MOD_MODMGR,__func__,"(req=%p) calling jmlist_entry_count with jml=%p",
					req,req->data.bin.nvl);

			jmls = jmlist_entry_count(req->data.bin.nvl,&entry_count);
			if( jmls != JMLIST_ERROR_SUCCESS ) {
				dbgprint(MOD_MODMGR,__func__,"(req=%p) jmlist_entry_count failed with jmls=%d",req,jmls);
				DBGRET_FAILURE(MOD_MODMGR);
			}
			dbgprint(MOD_MODMGR,__func__,"(req=%p) jmlist_entry_count was successful (%u)",req,entry_count);

			*nv_count = entry_count;
			dbgprint(MOD_MODMGR,__func__,"(req=%p) nv_count value updated to %u",*nv_count);
			DBGRET_SUCCESS(MOD_MODMGR);

			break;
		case REQUEST_STYPE_PIPE:
		default:
			dbgprint(MOD_MODMGR,__func__,"(req=%p) request has an invalid or unsupported stype (check req pointer...)",req);
	}

	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _req_bin_get_nv

   Helper function that gets the name-value pair in a binary stype request.
   If the request doesn't have any name-value pair or if the name-value isn't found
   this function returns failure. If the client code wants to simply check for the
   existence of the name-value pair, _req_get_nv_info function should be used instead.
*/
wstatus
_req_bin_get_nv(const request_t req,const char *look_name_ptr,unsigned int look_name_size,nvpair_t *nvpp)
{
	unsigned int nv_count;
	wstatus ws;
	unsigned int nv_idx;
	nvpair_t aux_nvp = 0;
	nvpair_t nvp_seek;
	jmlist_seek_handle shandle;
	jmlist_status jmls;
	void *aux_ptr;

	dbgprint(MOD_MODMGR,__func__,"called with req=%p, name_ptr=%p (%s), name_size=%u, nvpp=%p",
			req,look_name_ptr,array2z(look_name_ptr,look_name_size),look_name_size,nvpp);

	/* start by validating the arguments */

	if( !req ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) invalid req argument (req=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( !look_name_ptr ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) invalid look_name_ptr argument (look_name_ptr=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( !look_name_size ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) invalid look_name_size (look_name_size=0)",req);
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( req->stype != REQUEST_STYPE_TEXT ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) invalid request to be used with this function",req);
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( !nvpp ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) invalid nvpp argument (nvpp=0)",req);
		DBGRET_FAILURE(MOD_MODMGR);
	}

	dbgprint(MOD_MODMGR,__func__,"(req=%p) calling helper function to get name-value count",req);
	ws = _req_get_nv_count(req,&nv_count);
	dbgprint(MOD_MODMGR,__func__,"(req=%p) helper function returned (ws=%s)",wstatus_str(ws));
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) failed to get name-value count (helper function failed with ws=%s)",
				req,wstatus_str(ws));
		DBGRET_FAILURE(MOD_MODMGR);
	}
	dbgprint(MOD_MODMGR,__func__,"(req=%p) helper function was successful (nv_count=%u)",req,nv_count);

	if( nv_count == 0 ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) this request doesn't have any name-value pair",req);
		goto return_fail;
	}

	/* parse all nvpair from the name-value pair list of the binary stype request */

	jmls = jmlist_seek_start(req->data.bin.nvl,&shandle);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) failed to start seek throughout the name-value list "
				"(jmlist function failed with jmls=%d)",req,jmls);
		goto return_fail;
	}

	nv_idx = 0;
	*nvpp = 0;
	while( nv_count-- )
	{
		jmls = jmlist_seek_next(req->data.bin.nvl,&shandle,&aux_ptr);
		if( jmls != JMLIST_ERROR_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"(req=%p) failed to seek throughout the name-value list "
					"(jmlist function failed with jmls=%d)",req,jmls);
			goto return_fail;
		}

		nvp_seek = (nvpair_t)aux_ptr;

		dbgprint(MOD_MODMGR,__func__,"(req=%p) testing nvpair (idx=%u) name lengths",req,nv_idx);
		if( nvp_seek->name_size != look_name_size ) {
			dbgprint(MOD_MODMGR,__func__,"(req=%p) this nvpair (idx=%u) has different name size, seeking",
					req,nv_idx);
			goto seek_nvp;
		}
		dbgprint(MOD_MODMGR,__func__,"(req=%p) nvpair (idx=%u) has same length has the looking nvpair",req,nv_idx);

		/* compare name with the lookup name */

		dbgprint(MOD_MODMGR,__func__,"(req=%p) comparing nvpair (idx=%u) name characters",req,nv_idx);
		if( memcmp(nvp_seek->name_ptr,look_name_ptr,look_name_size) ) {
			dbgprint(MOD_MODMGR,__func__,"(req=%p) this nvpair (idx=%u) has different name, seeking",
					req,nv_idx);
			goto seek_nvp;
		}
		dbgprint(MOD_MODMGR,__func__,"(req=%p) nvpair was found successful in the list (idx=%u)",req,nv_idx);

		/* duplicate nvpair memory data structure to pass to the caller */

		ws = _nvp_dup(nvp_seek,&aux_nvp);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to duplicate request nvpair (idx=%u)",req,nv_idx);
			*nvpp = 0;
			goto return_fail;
		}
		dbgprint(MOD_MODMGR,__func__,"(req=%p) nvpair duplicated successfully (nvp=%p)",req,aux_nvp);


		/* do the cleanup */

		jmls = jmlist_seek_end(req->data.bin.nvl,&shandle);
		if( jmls != JMLIST_ERROR_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"(req=%p) failed to end jmlist seeking (jmls=%d)",req,jmls);
			goto return_fail;
		}

		*nvpp = aux_nvp;
		dbgprint(MOD_MODMGR,__func__,"(req=%p) nvpp value updated to %p",req,*nvpp);

		DBGRET_SUCCESS(MOD_MODMGR);

seek_nvp:
		nv_idx++;
	}

	DBGRET_FAILURE(MOD_MODMGR);

return_fail:
	if( aux_nvp ) {
		_nvp_free(aux_nvp);
		aux_nvp = 0;
	}

	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _nvp_dup

   Helper function to duplicate a nvpair data structure. To free the allocated
   nvpair data structure use the _nvp_free helper function.
*/

wstatus _nvp_dup(const nvpair_t nvp,nvpair_t *new_nvp)
{
	wstatus ws;
	nvpair_t aux_nvp = 0;

	dbgprint(MOD_MODMGR,__func__,"called with nvp=%p, new_nvp=%p",nvp,new_nvp);

	/* validate arguments */

	if( !nvp ) {
		dbgprint(MOD_MODMGR,__func__,"invalid nvp argument (nvp=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( !new_nvp ) {
		dbgprint(MOD_MODMGR,__func__,"invalid new_nvp argument (new_nvp=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	ws = _nvp_alloc(nvp->name_size,nvp->value_size,&aux_nvp);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"failed to allocate new nvpair data structure "
				"(helper function failed with ws=%s)",wstatus_str(ws));
		DBGRET_FAILURE(MOD_MODMGR);
	}
	dbgprint(MOD_MODMGR,__func__,"allocated new nvpair data structure (ptr=%p)",aux_nvp);

	ws = _nvp_fill(nvp->name_ptr,nvp->name_size,nvp->value_ptr,nvp->value_size,aux_nvp);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"failed to fill the new nvpair data structure "
				"(helper function failed with ws=%s)",wstatus_str(ws));

		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"new nvpair data structure was filled successfully");

	*new_nvp = aux_nvp;
	dbgprint(MOD_MODMGR,__func__,"updated new_nvp value to %p",*new_nvp);

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:

	if( aux_nvp )
   	{
		_nvp_free(aux_nvp);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"failed to free the allocated nvpair data structure (ptr=%p)",aux_nvp);
		}
		aux_nvp = 0;
	}

	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _req_get_nv_info

   Helper function that obtains a specific nvpair informations. These informations are saved
   in a data structure that is passed by the caller, nvpair_info_t see the header file for
   more information about the information that is possible to obtain for each nvpair.
*/
wstatus _req_get_nv_info(const request_t req,nvpair_info_t nvpi)
{
	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   modmgr_lookup

   This functions lookups for a specific module that was registered before in modmgr. The
   module search is done using the module name (meaning that there shouldn't exist two
   loaded modules with the same name in wicom) and a modreg_t data structure pointer is returned.
*/
wstatus modmgr_lookup(const char *mod_name,const struct _modreg_t **modp)
{
	jmlist_status jmls;
	unsigned int mod_idx;
	void *aux_ptr;
	const struct _modreg_t *aux_mod;
	jmlist_seek_handle shandle;
	jmlist_index entry_count;

	dbgprint(MOD_MODMGR,__func__,"called with mod_name=%s, modp=%p",z_ptr(mod_name),modp);

	if( !mod_name || !strlen(mod_name) ) {
		dbgprint(MOD_MODMGR,__func__,"invalid mod_name argument (mod_name=0 or strlen(mod_name)=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( !modp ) {
		dbgprint(MOD_MODMGR,__func__,"invalid modp argument (modp=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( !mod_list ) {
		/* TODO: use better way of testing for jmlist initialization...
		   this is code related to jmlist actually.. */
		dbgprint(MOD_MODMGR,__func__,"registered module list was not initialized yet");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	jmls = jmlist_entry_count(mod_list,&entry_count);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"failed to get registered modules list count "
				"(jmlist function failed with jmls=%d)",jmls);
		DBGRET_FAILURE(MOD_MODMGR);
	}

	jmls = jmlist_seek_start(mod_list,&shandle);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"failed to start seeking in registered modules list "
				"(jmlist function failed with jmls=%d)",jmls);
		DBGRET_FAILURE(MOD_MODMGR);
	}

	mod_idx = 0;
	while( entry_count-- )
	{
		jmls = jmlist_seek_next(mod_list,&shandle,&aux_ptr);
		if( jmls != JMLIST_ERROR_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"failed to seek in registered modules list "
					"(jmlist function failed with jmls=%d)",jmls);
			goto return_fail_end_seek;
		}

		aux_mod = (const modreg_t)aux_ptr;
		
		/* compare module's names */

		if( strcmp(aux_mod->basic.name,mod_name) ) {
			dbgprint(MOD_MODMGR,__func__,"module (idx=%u) name is different, "
					"seeking for next module in list",mod_idx);
			goto seek_mod;
		}

		dbgprint(MOD_MODMGR,__func__,"found requested module in index %u",mod_idx);

		/* finish seeking */

		jmls = jmlist_seek_end(mod_list,&shandle);
		if( jmls != JMLIST_ERROR_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"failed to finish seeking in registered modules list "
					"(jmlist function failed with jmls=%d)",jmls);
			DBGRET_FAILURE(MOD_MODMGR);
		}

		*modp = aux_mod;
		dbgprint(MOD_MODMGR,__func__,"updated modp value to %p",*modp);

		DBGRET_SUCCESS(MOD_MODMGR);

seek_mod:
		mod_idx++;
	}

return_fail_end_seek:
	jmls = jmlist_seek_end(mod_list,&shandle);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"failed to finish seeking in registered modules list "
				"(jmlist function failed with jmls=%d)",jmls);
	}
	DBGRET_FAILURE(MOD_MODMGR);
}

wstatus _request_send(const request_t req,const struct _modreg_t *mod)
{
	DBGRET_FAILURE(MOD_MODMGR);
}

