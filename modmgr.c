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

#include "wstatus.h"
#include "debug.h"
#include "modmgr.h"
#include "string.h"
#include "stdlib.h"
#include "jmlist.h"
#include <ctype.h>
#include <assert.h>


wstatus _req_from_text_to_bin(request_t req,request_t *req_bin);
wstatus _req_from_pipe_to_bin(request_t req,request_t *req_bin);
wstatus _req_nv_value_info(char *value_ptr,char **value_start,char **value_end,uint16_t *value_size);
wstatus _req_nv_name_info(char *name_ptr,char **name_end,uint16_t *name_size);
wstatus _req_text_nv_parse(char *nv_ptr,char **name_start,uint16_t *name_size,char **value_start,uint16_t *value_size);
wstatus _req_text_nvl_begin(char *req_text,char **nvl_begin);

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
_nvp_fill(char *name_ptr,uint16_t name_size,void *value_ptr,uint16_t value_size,nvpair_t nvp)
{
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

	if( nvp->value_size )
	{
		dbgprint(MOD_MODMGR,__func__,"copying %u bytes of the value to nvp structure (%p)",value_size,nvp);
		memcpy(nvp->value_ptr,value_ptr,value_size);
		dbgprint(MOD_MODMGR,__func__,"copied %d bytes successfully of the value to the nvp structure (%p)",value_size,nvp);
	}

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:
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
   It is assumed that the request format was validated before calling this function.
*/
wstatus
_req_text_src(request_t req,char *src)
{
	char reqidt[REQIDSIZE + REQTYPESIZE + 1];
	char reqsrc[REQMODSIZE+1];

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

	if( sscanf(req->data.text.raw,"%s %s ",reqidt,reqsrc) != 2 ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to parse request data!",req);
		goto return_fail;
	}

	dbgprint(MOD_MODMGR,__func__,"(req=%p) got source module name \"%s\"",req,reqsrc);
	strcpy(src,reqsrc);
	dbgprint(MOD_MODMGR,__func__,"(req=%p) wrote source module name into src=%p (%d bytes long)",
			req,src,strlen(reqsrc));

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
	char reqidt[REQIDSIZE + REQTYPESIZE + 1];
	char reqsrc[REQMODSIZE+1];
	char reqdst[REQMODSIZE+1];

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

	if( sscanf(req->data.text.raw,"%s %s %s",reqidt,reqsrc,reqdst) != 3 ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to parse request data!",req);
		goto return_fail;
	}

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
		if( !isdigit(req->data.text.raw[i]) )
			break;

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
	int i;
	request_type_list reqtype_num;

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

	for( i = 0 ; i < REQIDSIZE ; i++ ) {
		if( !isdigit(req->data.text.raw[i]) )
			break;
	}

	if( i == 0 ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) invalid request format, reqid was not detected",req);
		goto return_fail;
	}

	if( (req->data.text.raw[i] == 'R') || (req->data.text.raw[i] == 'r') ) {
		reqtype_num = REQUEST_TYPE_REPLY;
	} else if( req->data.text.raw[i] == ' ' ) {
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
_req_text_nv_count(request_t req,uint16_t *nvcount)
{
	char *aux;
	int state;
	uint16_t reqnvcount;
	char *name_start,*value_start;
	uint16_t name_size,value_size;
	wstatus ws;

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
	ws = _req_text_nvl_begin(aux,&aux);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to find nvl begin in request",req);
		goto return_fail;
	}

	dbgprint(MOD_MODMGR,__func__,"(req=%p) found nvpair start at +%d",req,(int)(aux - (char*)&req->data.text.raw));

	reqnvcount = 0;
	state = 0;

	while(1)
	{
		ws = _req_text_nv_parse(aux,&name_start,&name_size,&value_start,&value_size);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"(req=%p) failed to process text request");
			goto return_fail;
		}
		reqnvcount++;

		if( value_start ) {
			aux = value_start + value_size;
			if( V_QUOTECHAR(*aux) )
				aux++;
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
		if( V_REQENDCHAR(name_ptr[i]) || V_NVSEPCHAR(name_ptr[i]) )
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
	uint16_t i = 0;
	char end_char = ' ',c;
	bool quoted = false;

	assert( value_ptr != 0 );
	assert( value_end != 0 );
	assert( value_size != 0 );

	if( V_QUOTECHAR(value_ptr[0]) ) {
		quoted = true;
		end_char = value_ptr[0];
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

		if( (quoted && !V_QVALUECHAR(c)) || !V_VALUECHAR(c) ) {
			/* found an invalid character in VALUE field */
			goto invalid_char;
		}
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
		*value_start = value_ptr;
		*value_end = value_ptr + i - 1;
		*value_size = i;
	}

	dbgprint(MOD_MODMGR,__func__,"updated value_start=%p, value_end=%p, value_size=%u",
			*value_start,*value_end,*value_size);

	DBGRET_SUCCESS(MOD_MODMGR);

invalid_char:
	dbgprint(MOD_MODMGR,__func__,"invalid or unexpected character found in VALUE field (c=%02X)",c);
	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _req_text_nv_parse

   Helper function that will parse a name-value pair in text form and return
   some useful information that can be used to convert the name-value pair
   into other form (fifo, binary). This function can also be used for request
   validation and others.

   It is possible that the specific nvpair doesn't have a value associated,
   in that case, value_size is set to 0 (zero). Name size cannot in any case
   be zero, if it happens, this function returns failure.
*/
wstatus
_req_text_nv_parse(char *nv_ptr,char **name_start,uint16_t *name_size,char **value_start,uint16_t *value_size)
{
	wstatus ws;
	char *l_name_end;
	uint16_t l_name_size;
	char *l_value_start;
	char *l_value_end;
	uint16_t l_value_size;

	dbgprint(MOD_MODMGR,__func__,"called with nv_ptr=%p, name_start=%p, name_size=%p, value_start=%p, value_size=%p",
			nv_ptr,name_start,name_size,value_start,value_size);

	assert(nv_ptr != 0);
	assert(name_start != 0);
	assert(name_size != 0);
	assert(value_start != 0);
	assert(value_size != 0);

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

	if( V_REQENDCHAR(nv_ptr[l_name_size]) ) {
		l_value_start = 0;
		l_value_size = 0;
		goto no_value;
	}

	if( !V_NVSEPCHAR(nv_ptr[l_name_size]) ) {
		dbgprint(MOD_MODMGR,__func__,"invalid/unexpected char after NAME token (c=%02X)",nv_ptr[l_name_size]);
		goto return_fail;
	}

	/* we've a value associated, try parse it */
	ws = _req_nv_value_info(nv_ptr+l_name_size+1,&l_value_start,&l_value_end,&l_value_size);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"failed to parse VALUE token of this nvpair");
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"valid value found ptr=%p, size=%u",l_value_start,l_value_size);

no_value:
	*value_start = l_value_start;
	*value_size = l_value_size;
	dbgprint(MOD_MODMGR,__func__,"updated value_start=%p, value_size=%u",*value_start,*value_size);

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:
	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _req_text_nvl_begin

   Helper function that returns a pointer to the beginning of the nvpair list
   from the request in text form.
*/
wstatus
_req_text_nvl_begin(char *req_text,char **nvl_begin)
{
	int space_count;

	dbgprint(MOD_MODMGR,__func__,"called with req_text=%p, nvl_begin=%p",req_text,nvl_begin);

	assert(req_text != 0);
	assert(nvl_begin != 0);

	space_count = 0;
	while( *req_text )
   	{		
		if( (*req_text == ' ') && (space_count++ == 3) )
			goto at_nvpair;

		req_text++;
	}

	dbgprint(MOD_MODMGR,__func__,"unable to find nvl begin");
	DBGRET_FAILURE(MOD_MODMGR);

at_nvpair:
	req_text++;
	*nvl_begin = req_text;
	DBGRET_SUCCESS(MOD_MODMGR);
}

wstatus
_req_from_pipe_to_bin(request_t req,request_t *req_bin)
{
	return WSTATUS_UNIMPLEMENTED;
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
	uint16_t nv_idx,nvcount;
	nvpair_t nvp;
	jmlist_status jmls;
	wstatus ws;
	char *aux;
	char *name_start,*value_start;
	uint16_t name_size,value_size;

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

	/* start processing the aditional nvpairs */

	ws = _req_text_nv_count(req,&nvcount);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to get nvcount from request",req);
		goto return_fail;
	}
	dbgprint(MOD_MODMGR,__func__,"(req=%p) this request has %u nvpairs",req,nvcount);
	
	if( nvcount == 0 )
		goto skip_nvpair_parsing;

	ws = _req_text_nvl_begin(req->data.text.raw,&aux);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to find nvl begin",req);
		goto return_fail;
	}

	for( nv_idx = 1 ; ; nv_idx++ )
   	{
		/* try to parse nvpair text tokens */
		ws = _req_text_nv_parse(aux,&name_start,&name_size,&value_start,&value_size);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"(req=%p) failed to process text request");
			goto return_fail;
		}

		/* allocate new nvpair data structure for this nvpair */
		ws = _nvp_alloc(name_size,value_size,&nvp);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"(req=%p) failed to allocate a new nvpair (name_size=%u, value_size=%u)",
					req,name_size,value_size);
			goto return_fail;
		}
		dbgprint(MOD_MODMGR,__func__,"(req=%p) allocated new nvpair data structure successfully (p=%p)",req,nvp);

		/* fill the new data structure with the tokens */
		ws = _nvp_fill(name_start,name_size,value_start,value_size,nvp);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"(req=%p) failed to fill the new nvpair (p=%p)",req,nvp);
			goto return_fail;
		}
		dbgprint(MOD_MODMGR,__func__,"(req=%p) new nvpair ready for insertion in nvpair list",req);

		jmls = jmlist_insert(new_req->data.bin.nvl,nvp);
		if( jmls != JMLIST_ERROR_SUCCESS ) {
			dbgprint(MOD_MODMGR,__func__,"(req=%p) jmlist failed to insert new nvpair data strucutre (jmls=%d)",req,jmls);
			goto return_fail;
		}

		if( value_start ) {
			aux = value_start + value_size;
			if( V_QUOTECHAR(*aux) )
				aux++;
		}

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
			/* TODO: this is not freeing each item!! */
			jmlist_free(new_req->data.bin.nvl);
		}

		free(new_req);
	}

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

