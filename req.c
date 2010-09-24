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

#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "wstatus.h"
#include "debug.h"
#include "nvpair.h"
#include "req.h"

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

	dbgprint(MOD_REQ,__func__,"called with req=%p, src=%p",req,src);

	if( !req ) {
		dbgprint(MOD_REQ,__func__,"invalid req argument (req=0)");
		goto return_fail;
	}

	if( !src ) {
		dbgprint(MOD_REQ,__func__,"invalid src argument (src=0)");
		goto return_fail;
	}

	if( req->stype != REQUEST_STYPE_TEXT ) {
		dbgprint(MOD_REQ,__func__,"invalid request to be used with this function");
		goto return_fail;
	}

	ws = _req_text_token_seek(req->data.text.raw,TEXT_TOKEN_MODSRC,&src_ptr);
	if( ws != WSTATUS_SUCCESS ) {
		DBGRET_FAILURE(MOD_REQ);
	}

	aux_ptr = src_ptr;
	for( i = 0, j = 0 ; !V_REQENDCHAR(aux_ptr[i]) ; i++ )
	{
		if( V_TOKSEPCHAR(aux_ptr[i]) )
			goto finished_token;

		if( V_MODCHAR(aux_ptr[i]) )
	   	{
			if( j >= REQMODSIZE ) {
				dbgprint(MOD_REQ,__func__,"(req=%p) source module name length is overlimit (limit is %d)",req,REQMODSIZE);
				goto return_fail;
			}
			reqsrc[j++] = aux_ptr[i];
			continue;
		}

		/* invalid character detected */
		dbgprint(MOD_REQ,__func__,"(req=%p) invalid/unexpected character in MODSRC token (offset %d)",req,i);
		goto return_fail;
	}

	dbgprint(MOD_REQ,__func__,"(req=%p) unable to reach MODSRC token end",req);
	goto return_fail;

finished_token:

	reqsrc[j] = '\0';

	dbgprint(MOD_REQ,__func__,"(req=%p) got source module name \"%s\"",req,reqsrc);
	strcpy(src,reqsrc);
	dbgprint(MOD_REQ,__func__,"(req=%p) wrote source module name into src=%p (%d bytes long)",
			req,src,strlen(reqsrc));

	DBGRET_SUCCESS(MOD_REQ);

return_fail:
	DBGRET_FAILURE(MOD_REQ);
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

	dbgprint(MOD_REQ,__func__,"called with req=%p, code=%p",req,code);

	if( !req ) {
		dbgprint(MOD_REQ,__func__,"invalid req argument (req=0)");
		goto return_fail;
	}

	if( !code ) {
		dbgprint(MOD_REQ,__func__,"invalid code argument (code=0)");
		goto return_fail;
	}

	if( req->stype != REQUEST_STYPE_TEXT ) {
		dbgprint(MOD_REQ,__func__,"invalid request to be used with this function");
		goto return_fail;
	}

	ws = _req_text_token_seek(req->data.text.raw,TEXT_TOKEN_CODE,&code_ptr);
	if( ws != WSTATUS_SUCCESS ) {
		DBGRET_FAILURE(MOD_REQ);
	}

	aux_ptr = code_ptr;
	for( i = 0, j = 0 ; !V_REQENDCHAR(aux_ptr[i]) ; i++ )
	{
		if( V_TOKSEPCHAR(aux_ptr[i]) )
			goto finished_token;

		if( V_CODECHAR(aux_ptr[i]) )
	   	{
			if( j >= REQCODESIZE ) {
				dbgprint(MOD_REQ,__func__,"(req=%p) request code has too many characters (limit is %d)",req,REQCODESIZE);
				goto return_fail;
			}
			reqcode[j++] = aux_ptr[i];
			continue;
		}

		/* invalid character detected */
		dbgprint(MOD_REQ,__func__,"(req=%p) invalid/unexpected character in CODE token (offset %d)",req,i);
		goto return_fail;
	}

	/* code is a "special case" token because its the last token of the header of the request,
	   name-value pairs are not mandatory so after the code token the request might end. */

//	dbgprint(MOD_REQ,__func__,"(req=%p) unable to reach CODE token end",req);
//	goto return_fail;

finished_token:

	reqcode[j] = '\0';

	dbgprint(MOD_REQ,__func__,"(req=%p) got request code \"%s\"",req,reqcode);
	strcpy(code,reqcode);
	dbgprint(MOD_REQ,__func__,"(req=%p) wrote request code into code=%p (%d bytes long)",
			req,code,strlen(reqcode));

	DBGRET_SUCCESS(MOD_REQ);

return_fail:
	DBGRET_FAILURE(MOD_REQ);
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

	dbgprint(MOD_REQ,__func__,"called with req=%p, dst=%p",req,dst);

	if( !req ) {
		dbgprint(MOD_REQ,__func__,"invalid req argument (req=0)");
		goto return_fail;
	}

	if( !dst ) {
		dbgprint(MOD_REQ,__func__,"invalid dst argument (dst=0)");
		goto return_fail;
	}

	if( req->stype != REQUEST_STYPE_TEXT ) {
		dbgprint(MOD_REQ,__func__,"invalid request to be used with this function");
		goto return_fail;
	}

	ws = _req_text_token_seek(req->data.text.raw,TEXT_TOKEN_MODDST,&dst_ptr);
	if( ws != WSTATUS_SUCCESS ) {
		DBGRET_FAILURE(MOD_REQ);
	}

	aux_ptr = dst_ptr;
	for( i = 0, j = 0 ; !V_REQENDCHAR(aux_ptr[i]) ; i++ )
	{
		if( V_TOKSEPCHAR(aux_ptr[i]) )
			goto finished_token;

		if( V_MODCHAR(aux_ptr[i]) )
	   	{
			if( j >= REQMODSIZE ) {
				dbgprint(MOD_REQ,__func__,"(req=%p) destination module name is overlimit (limit is %d)",req,REQMODSIZE);
				goto return_fail;
			}
			reqdst[j++] = aux_ptr[i];
			continue;
		}

		/* invalid character detected */
		dbgprint(MOD_REQ,__func__,"(req=%p) invalid/unexpected character in MODDST token",req);
		goto return_fail;
	}

	dbgprint(MOD_REQ,__func__,"(req=%p) unable to reach MODDST token end",req);
	goto return_fail;

finished_token:

	reqdst[j] = '\0';

	dbgprint(MOD_REQ,__func__,"(req=%p) got destiny module name \"%s\"",req,reqdst);
	strcpy(dst,reqdst);
	dbgprint(MOD_REQ,__func__,"(req=%p) wrote destiny module name into dst=%p (%d bytes long)",
			req,dst,strlen(reqdst));

	DBGRET_SUCCESS(MOD_REQ);

return_fail:
	DBGRET_FAILURE(MOD_REQ);
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

	dbgprint(MOD_REQ,__func__,"called with req=%p, rid=%p",req,rid);

	if( !req ) {
		dbgprint(MOD_REQ,__func__,"invalid req argument (req=0)");
		goto return_fail;
	}

	if( !rid ) {
		dbgprint(MOD_REQ,__func__,"invalid rid argument (rid=0)");
		goto return_fail;
	}

	if( req->stype != REQUEST_STYPE_TEXT ) {
		dbgprint(MOD_REQ,__func__,"invalid request to be used with this function");
		goto return_fail;
	}

	memset(reqid_char,0,sizeof(reqid_char));
	dbgprint(MOD_REQ,__func__,"(req=%p) local reqid cleared, starting the parsing",req);

	for( i = 0 ; i < REQIDSIZE ; i++ )
   	{
		if( V_REPLYCHAR(req->data.text.raw[i]) || V_TOKSEPCHAR(req->data.text.raw[i]) )
			break;

		if( !V_RIDCHAR(req->data.text.raw[i]) )
			goto invalid_char;

		reqid_char[i] = req->data.text.raw[i];
	}

	if( reqid_char[0] == '\0' ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) unable to parse request data",req);
		goto return_fail;
	}

	dbgprint(MOD_REQ,__func__,"(req=%p) converting reqid from char to int");
	if( !sscanf(reqid_char,"%hu",&reqid_int) ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) failed to convert reqid from char (%s) to int",req,reqid_char);
		goto return_fail;
	}
	dbgprint(MOD_REQ,__func__,"(req=%p) converted reqid from char (%s) to int successfully (%u)",req,reqid_char,reqid_int);
	*rid = reqid_int;
	dbgprint(MOD_REQ,__func__,"(req=%p) updated argument rid to new value %u",req,*rid);

	
	DBGRET_SUCCESS(MOD_REQ);

invalid_char:
	dbgprint(MOD_REQ,__func__,"(req=%p) invalid/unexpected character in ID token (c=%02X)",req,req->data.text.raw[i]);
return_fail:
	DBGRET_FAILURE(MOD_REQ);
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

	dbgprint(MOD_REQ,__func__,"called with req=%p, type=%p",req,type);

	if( !req ) {
		dbgprint(MOD_REQ,__func__,"invalid req argument (req=0)");
		goto return_fail;
	}

	if( !type ) {
		dbgprint(MOD_REQ,__func__,"invalid type argument (type=0)");
		goto return_fail;
	}

	if( req->stype != REQUEST_STYPE_TEXT ) {
		dbgprint(MOD_REQ,__func__,"invalid request to be used with this function");
		goto return_fail;
	}

	ws = _req_text_token_seek(req->data.text.raw,TEXT_TOKEN_TYPE,&type_ptr);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) unable to reach TYPE token in request",req);
		goto return_fail;
	}

	if( V_REPLYCHAR(*type_ptr) ) {
		reqtype_num = REQUEST_TYPE_REPLY;
	} else if( V_TOKSEPCHAR(*type_ptr) ) {
		reqtype_num = REQUEST_TYPE_REQUEST;
	} else {
		dbgprint(MOD_REQ,__func__,"(req=%p) invalid request type char found after the id",req);
		goto return_fail;
	}

	dbgprint(MOD_REQ,__func__,"(req=%p) updating type argument to value %d",req,reqtype_num);
	*type = reqtype_num;
	dbgprint(MOD_REQ,__func__,"(req=%p) updated type argument to value %d",req,*type);

	DBGRET_SUCCESS(MOD_REQ);

return_fail:
	DBGRET_FAILURE(MOD_REQ);
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
_req_text_nv_count(const struct _request_t *req,unsigned int *nvcount)
{
	char *aux;
	int state;
	unsigned int reqnvcount;
	char *name_start,*value_start;
	unsigned int name_size,value_size;
	wstatus ws;
	nvpair_fflag_list fflags;

	dbgprint(MOD_REQ,__func__,"called with req=%p, nvcount=%p",req,nvcount);

	if( !req ) {
		dbgprint(MOD_REQ,__func__,"invalid req argument (req=0)");
		goto return_fail;
	}

	if( !nvcount ) {
		dbgprint(MOD_REQ,__func__,"invalid nvcount argument (nvcount=0)");
		goto return_fail;
	}

	if( req->stype != REQUEST_STYPE_TEXT ) {
		dbgprint(MOD_REQ,__func__,"invalid request to be used with this function");
		goto return_fail;
	}

	dbgprint(MOD_REQ,__func__,"(req=%p) looking up for nvpair start",req);

	aux = (char*)&req->data.text.raw;
	ws = _req_text_token_seek(aux,TEXT_TOKEN_NVL,&aux);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) unable to find nvl begin in request",req);
		goto return_fail;
	}

	dbgprint(MOD_REQ,__func__,"(req=%p) found nvpair start at +%d",req,(int)(aux - (char*)&req->data.text.raw));

	if( V_REQENDCHAR(*aux) ) {
		/* this request doesn't have any nvpair */
		reqnvcount = 0;
		goto skip_nvpair_parsing;
	}

	reqnvcount = 0;
	state = 0;

	while(1)
	{
		ws = _req_text_nv_parse(aux,&name_start,&name_size,&value_start,&value_size,&fflags);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) failed to process text request");
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
			dbgprint(MOD_REQ,__func__,"(req=%p) [nv_idx=%u] invalid/unexpected character after nvpair",req,reqnvcount);
			goto return_fail;
		}

		aux++;
	}

skip_nvpair_parsing:
	dbgprint(MOD_REQ,__func__,"(req=%p) parsing is finished, detected %u nvpairs",req,reqnvcount);
	*nvcount = reqnvcount;
	dbgprint(MOD_REQ,__func__,"(req=%p) updated nvcount argument to new value %u",req,*nvcount);

	DBGRET_SUCCESS(MOD_REQ);
return_fail:
	DBGRET_FAILURE(MOD_REQ);
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
	dbgprint(MOD_REQ,__func__,"found invalid/unexpected character in name parsing (c=%02X), name_ptr=%p",name_ptr[i],name_ptr);
	goto return_fail;

end_of_name:
	if( !i ) {
		dbgprint(MOD_REQ,__func__,"name is empty (no name character found)");
		*name_size = 0;
	}
	*name_end = name_ptr + i - 1;
	*name_size = i;
	DBGRET_SUCCESS(MOD_REQ);

return_fail:
	DBGRET_FAILURE(MOD_REQ);
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
	dbgprint(MOD_REQ,__func__,"ups... fix _req_nv_value_info code");
	DBGRET_FAILURE(MOD_REQ);

end_of_req:
	if( quoted ) {
		dbgprint(MOD_REQ,__func__,"value didn't finished with quote char (%c)",end_char);
		DBGRET_FAILURE(MOD_REQ);
	}

	/* un-quoted, this was the last nvpair of the request */

	if( i == 0 ) {
		/* string is like "... name1=" , ugly way of finishing... */
		dbgprint(MOD_REQ,__func__,"if there's no value don't use NVPAIR separator char either");
		DBGRET_FAILURE(MOD_REQ);
	}

	*value_size = i;
	*value_start = value_ptr;
	*value_end = value_ptr + *value_size - 1;

	dbgprint(MOD_REQ,__func__,"finishing with end_of_req, writing value_start=%p, value_end=%p, value_size=%u",
			*value_start,*value_end,*value_size);

	DBGRET_SUCCESS(MOD_REQ);

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

	dbgprint(MOD_REQ,__func__,"updated value_start=%p, value_end=%p, value_size=%u",
			*value_start,*value_end,*value_size);

	DBGRET_SUCCESS(MOD_REQ);

invalid_char:
	dbgprint(MOD_REQ,__func__,"invalid or unexpected character found in VALUE field (c=%02X)",c);
	DBGRET_FAILURE(MOD_REQ);
}

/*
   _req_nv_value_format

   Helper function to get value format.
*/
wstatus
_req_nv_value_format(const char *value_ptr,nvpair_fflag_list *fflags)
{
	dbgprint(MOD_REQ,__func__,"called with value_ptr=%p, fflags=%p",value_ptr,fflags);

	if( !value_ptr ) {
		dbgprint(MOD_REQ,__func__,"invalid value_ptr argument (value_ptr=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	if( !fflags ) {
		dbgprint(MOD_REQ,__func__,"invalid fflags argument (fflags=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	if( V_ENCPREFIX(*value_ptr) ) {
		*fflags = NVPAIR_FFLAG_ENCODED;
	} else if ( V_QUOTECHAR(*value_ptr) ) {
		*fflags = NVPAIR_FFLAG_QUOTED;
	} else if ( V_VALUECHAR(*value_ptr) ) {
		*fflags = NVPAIR_FFLAG_UNQUOTED;
	} else {
		dbgprint(MOD_REQ,__func__,"unable to determine value format (0x%02X '%c')",
				*value_ptr,b2c(*value_ptr));
		DBGRET_FAILURE(MOD_REQ);
	}

	dbgprint(MOD_REQ,__func__,"updated fflags value to %d (%s)",*fflags,nvp_fflags_str(*fflags));

	DBGRET_SUCCESS(MOD_REQ);
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
		char **value_start,unsigned int *value_size,nvpair_fflag_list *fflags)
{
	wstatus ws;
	char *l_name_end;
	uint16_t l_name_size;
	char *l_value_start;
	char *l_value_end;
	uint16_t l_value_size;
	nvpair_fflag_list l_fflags;

	dbgprint(MOD_REQ,__func__,"called with nv_ptr=%p, name_start=%p, "
			"name_size=%p, value_start=%p, value_size=%p, fflags=%p",
			nv_ptr,name_start,name_size,value_start,value_size,fflags);

	assert(nv_ptr != 0);
	assert(name_start != 0);
	assert(name_size != 0);
	assert(value_start != 0);
	assert(value_size != 0);
	assert(fflags != 0);

	/* TODO: first validate NAME using nvp_name_validate() */

	/* start by parsing name token */
	ws = _req_nv_name_info(nv_ptr,&l_name_end,&l_name_size);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"failed to parse NAME token");
		goto return_fail;
	}
	dbgprint(MOD_REQ,__func__,"valid name found ptr=%p, size=%u",nv_ptr,l_name_size);

	*name_start = nv_ptr;
	*name_size = l_name_size;
	dbgprint(MOD_REQ,__func__,"updated name_start=%p, name_size=%u",*name_start,*name_size);

	/* verify if is there any value associated to this nvpair */

	if( V_REQENDCHAR(nv_ptr[l_name_size]) || V_TOKSEPCHAR(nv_ptr[l_name_size]) ) {
		l_value_start = 0;
		l_value_size = 0;
		goto no_value;
	}

	if( !V_NVSEPCHAR(nv_ptr[l_name_size]) ) {
		dbgprint(MOD_REQ,__func__,"invalid/unexpected char after NAME token (c=%02X)",nv_ptr[l_name_size]);
		goto return_fail;
	}

	/* get value format using specific function */

	ws = _req_nv_value_format(nv_ptr+l_name_size+1,&l_fflags);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"failed to get VALUE token format of this nvpair");
		goto return_fail;
	}
	dbgprint(MOD_REQ,__func__,"got value format flags (%d) successfully",l_fflags);

	/* get value using helper function, this will get the full token in raw form,
	   so even if its encoded, it will return the encoded value. */
	ws = _req_nv_value_info(nv_ptr+l_name_size+1,&l_value_start,&l_value_end,&l_value_size);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"failed to parse VALUE token of this nvpair");
		goto return_fail;
	}
	dbgprint(MOD_REQ,__func__,"valid value found ptr=%p, size=%u",l_value_start,l_value_size);

no_value:
	*value_start = l_value_start;
	*value_size = l_value_size;
	*fflags = l_fflags;
	dbgprint(MOD_REQ,__func__,"updated value_start=%p, value_size=%u, fflags=%d",
			*value_start,*value_size,*fflags);

	DBGRET_SUCCESS(MOD_REQ);

return_fail:
	DBGRET_FAILURE(MOD_REQ);
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

	dbgprint(MOD_REQ,__func__,"called with req_text=%p, token=%d, token_ptr=%p",req_text,token,token_ptr);

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
				dbgprint(MOD_REQ,__func__,"invalid/unsupported token selected");
				goto return_fail;
		}
	}

	if( cur_token == TEXT_TOKEN_CODE ) {
		/* this request doesn't have any name-value pair */
		cur_token = TEXT_TOKEN_NVL;
		/* token_ptr will point to the null char, take care with that Mr.Client code. */
		goto reached_token;
	}

	dbgprint(MOD_REQ,__func__,"unable to reach to the requested token");
	DBGRET_FAILURE(MOD_REQ);

reached_token:
	dbgprint(MOD_REQ,__func__,"found token at offset +%d",i);
	*token_ptr = (char*)(req_text + i);
	dbgprint(MOD_REQ,__func__,"updated token_ptr to %p",*token_ptr);
	DBGRET_SUCCESS(MOD_REQ);

return_fail:
	if( cur_token_str )
		dbgprint(MOD_REQ,__func__,"invalid/unexpected character (c=%02X) at %u in token %s",
				req_text[i], i, cur_token_str);

	DBGRET_FAILURE(MOD_REQ);
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
	nvpair_fflag_list fflags;
	char *decoded_ptr = 0;
	unsigned int decoded_size = 0;

	dbgprint(MOD_REQ,__func__,"called with req=%p, req_bin=%p",req,req_bin);

	/* validate arguments */

	if( !req ) {
		dbgprint(MOD_REQ,__func__,"invalid req argument (req=0)");
		goto return_fail;
	}

	if( !req_bin ) {
		dbgprint(MOD_REQ,__func__,"invalid req_bin argument (req_bin=0)");
		goto return_fail;
	}

	if( req->stype != REQUEST_STYPE_TEXT ) {
		dbgprint(MOD_REQ,__func__,"invalid request to be used with this function");
		goto return_fail;
	}

	/* process headers */

	new_req = (request_t)malloc(sizeof(struct _request_t));
	if( !new_req ) {
		dbgprint(MOD_REQ,__func__,"malloc failed");
		goto return_fail;
	}
	memset(new_req,0,sizeof(struct _request_t));
	dbgprint(MOD_REQ,__func__,"(req=%p) allocated new request data structure (%p)",req,new_req);

	/* create jmlist for the nvp */
	jmls = jmlist_create(&new_req->data.bin.nvl,&jmlp);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) failed to create jmlist (jmls=%d)",req,jmls);
		goto return_fail;
	}
	dbgprint(MOD_REQ,__func__,"(req=%p) created jmlist for nvl successfully (jml=%p)",req,new_req->data.bin.nvl);

	/* start processing text request */

	ws = _req_text_rid(req,&reqid);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) unable to get request id",req);
		goto return_fail;
	}
	dbgprint(MOD_REQ,__func__,"(req=%p) got request id %u",req,reqid);
	new_req->data.bin.id = reqid;

	ws = _req_text_type(req,&reqtype);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) unable to get request type",req);
		goto return_fail;
	}
	dbgprint(MOD_REQ,__func__,"(req=%p) got request type %s",req,
			(reqtype == REQUEST_TYPE_REQUEST ? "REQUEST" : "REPLY"));
	new_req->data.bin.type = reqtype;

	ws = _req_text_src(req,new_req->data.bin.src);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) unable to get request source module",req);
		goto return_fail;
	}
	dbgprint(MOD_REQ,__func__,"(req=%p) got request source module (%s)",req,new_req->data.bin.src);

	ws = _req_text_dst(req,new_req->data.bin.dst);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) unable to get request destiny module",req);
		goto return_fail;
	}
	dbgprint(MOD_REQ,__func__,"(req=%p) got request destination module (%s)",req,new_req->data.bin.dst);

	ws = _req_text_code(req,new_req->data.bin.code);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) unable to get request code",req);
		goto return_fail;
	}
	dbgprint(MOD_REQ,__func__,"(req=%p) got request code (%s)",req,new_req->data.bin.code);

	/* start processing the aditional nvpairs */

	ws = _req_text_nv_count(req,&nvcount);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) unable to get nvcount from request",req);
		goto return_fail;
	}
	dbgprint(MOD_REQ,__func__,"(req=%p) this request has %u nvpairs",req,nvcount);
	
	if( nvcount == 0 )
		goto skip_nvpair_parsing;

	ws = _req_text_token_seek(req->data.text.raw,TEXT_TOKEN_NVL,&aux);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) unable to find nvl begin",req);
		goto return_fail;
	}

	for( nv_idx = 1 ; ; nv_idx++ )
   	{
		/* TODO: validate first then parse! */

		/* try to parse nvpair text tokens */
		ws = _req_text_nv_parse(aux,&name_start,&name_size,&value_start,&value_size,&fflags);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) failed to process text request");
			goto return_fail;
		}

		if( nvp_flag_test(fflags,NVPAIR_FFLAG_ENCODED) )
	   	{
			ws = _nvp_value_decoded_size(value_start,value_size,&decoded_size);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_REQ,__func__,"(req=%p) failed to get decoded size for nv_idx=%u",req,nv_idx);
				goto return_fail;
			}

			/* allocate buffer that will store the decoded value */

			decoded_ptr = (char*)malloc(decoded_size);
			if( !decoded_ptr ) {
				dbgprint(MOD_REQ,__func__,"(req=%p) malloc failed on decoded_size=%u",req,decoded_size);
				goto return_fail;
			}
			dbgprint(MOD_REQ,__func__,"(req=%p) allocated buffer for decoded value successful (ptr=%p)",
					req,decoded_ptr);

			ws = _nvp_value_decode(value_start,value_size,decoded_ptr,decoded_size);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_REQ,__func__,"(req=%p) unable to decode value (ws=%s)",req,wstatus_str(ws));
				goto return_fail;
			}
			dbgprint(MOD_REQ,__func__,"(req=%p) decoded successfully the value",req);
		}

		/* allocate new nvpair data structure for this nvpair */

		if( nvp_flag_test(fflags,NVPAIR_FFLAG_ENCODED) )
			ws = _nvp_alloc(name_size,decoded_size,&nvp);
		else
			ws = _nvp_alloc(name_size,value_size,&nvp);

		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) failed to allocate a new nvpair (name_size=%u, value_size=%u)",
					req,name_size,value_size);
			goto return_fail;
		}
		dbgprint(MOD_REQ,__func__,"(req=%p) allocated new nvpair data structure successfully (p=%p)",req,nvp);

		/* fill the new data structure with the tokens */
		
		if( nvp_flag_test(fflags,NVPAIR_FFLAG_ENCODED) )
			ws = _nvp_fill(name_start,name_size,decoded_ptr,decoded_size,nvp);
		else
			ws = _nvp_fill(name_start,name_size,value_start,value_size,nvp);

		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) failed to fill the new nvpair (p=%p)",req,nvp);
			goto return_fail;
		}
		dbgprint(MOD_REQ,__func__,"(req=%p) new nvpair ready for insertion in nvpair list",req);

		/* we can now free the decoded buffer if it was used */
		
		if( decoded_ptr ) {
			free(decoded_ptr);
			decoded_ptr = 0;
			decoded_size = 0;
		}

		jmls = jmlist_insert(new_req->data.bin.nvl,nvp);
		if( jmls != JMLIST_ERROR_SUCCESS ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) jmlist failed to insert new nvpair data strucutre (jmls=%d)",req,jmls);
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
			dbgprint(MOD_REQ,__func__,"(req=%p) [nv_idx=%u] invalid/unexpected character after nvpair",req,nv_idx);
			goto return_fail;
		}

		aux++;
	}

skip_nvpair_parsing:

	*req_bin = new_req;
	dbgprint(MOD_REQ,__func__,"(req=%p) updated req_bin to %p",req,*req_bin);

	DBGRET_SUCCESS(MOD_REQ);

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

	DBGRET_FAILURE(MOD_REQ);
}

wstatus
req_to_bin(request_t req,request_t *req_bin)
{
	wstatus ws;
	dbgprint(MOD_REQ,__func__,"called with req=%p, req_bin=%p",req,req_bin);

	switch(req->stype)
	{
		case REQUEST_STYPE_BIN:
			dbgprint(MOD_REQ,__func__,"(req=%p) request is already in bin data structure");
			goto return_fail;
		case REQUEST_STYPE_TEXT:
			dbgprint(MOD_REQ,__func__,"(req=%p) calling helper function _req_from_text_to_bin",req);
			ws = _req_from_text_to_bin(req,req_bin);
			dbgprint(MOD_REQ,__func__,"(req=%p) helper function _req_from_text_to_bin returned ws=%d",req,ws);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_REQ,__func__,"(req=%p) helper function failed, aborting",req);
				goto return_fail;
			}
			break;
		case REQUEST_STYPE_PIPE:
			dbgprint(MOD_REQ,__func__,"(req=%p) calling helper function _req_from_pipe_to_bin",req);
			ws = _req_from_pipe_to_bin(req,req_bin);
			dbgprint(MOD_REQ,__func__,"(req=%p) helper function _req_from_pipe_to_bin returned ws=%d",req,ws);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_REQ,__func__,"(req=%p) helper function failed, aborting",req);
				goto return_fail;
			}
			break;
		default:
			dbgprint(MOD_REQ,__func__,"(req=%p) request has an invalid or unsupported stype (check req pointer...)",req);
			goto return_fail;
	}

	DBGRET_SUCCESS(MOD_REQ);

return_fail:
	DBGRET_FAILURE(MOD_REQ);
}

void req_dump_header(uint16_t id,request_type_list type,char *mod_src,char *mod_dst,char *req_code)
{
	printf(	"    %s %u  %s -> %s (%s)\n",
			(type == REQUEST_TYPE_REQUEST ? "REQ" : "RLY"),id,
			mod_src,mod_dst,req_code);
}

/*
   req_dump_nvp

   Helper function to dump a single nvpair. Remember that name_ptr and value_ptr need not to be
   null terminated, thats why this functions prints char by char.
*/
void req_dump_nvp(char *name_ptr,uint16_t name_size,char *value_ptr,uint16_t value_size)
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
	DBGRET_SUCCESS(MOD_REQ);
}

void _req_bin_jml_dump(void *ptr,void *param)
{
	nvpair_t nvp = (nvpair_t)ptr;
	req_dump_nvp(nvp->name_ptr,nvp->name_size,nvp->value_ptr,nvp->value_size);
}

wstatus
_req_bin_dump(request_t req)
{
	dbgprint(MOD_REQ,__func__,"called with req=%p",req);

	req_dump_header(req->data.bin.id,req->data.bin.type,req->data.bin.src,req->data.bin.dst,req->data.bin.code);
	jmlist_parse(req->data.bin.nvl,_req_bin_jml_dump,0);

	DBGRET_SUCCESS(MOD_REQ);
}

wstatus
_req_pipe_dump(request_t req)
{
	DBGRET_FAILURE(MOD_REQ);
}

wstatus
req_dump(request_t req)
{
	wstatus ws;

	dbgprint(MOD_REQ,__func__,"called with req=%p",req);

	switch(req->stype)
	{
		case REQUEST_STYPE_BIN:
			dbgprint(MOD_REQ,__func__,"(req=%p) calling helper function _req_bin_dump",req);
			ws = _req_bin_dump(req);
			dbgprint(MOD_REQ,__func__,"(req=%p) helper function _req_bin_dump returned ws=%d",req,ws);
			break;
		case REQUEST_STYPE_TEXT:
			dbgprint(MOD_REQ,__func__,"(req=%p) calling helper function _req_text_dump",req);
			ws = _req_text_dump(req);
			dbgprint(MOD_REQ,__func__,"(req=%p) helper function _req_text_dump returned ws=%d",req,ws);
			break;
		case REQUEST_STYPE_PIPE:
			dbgprint(MOD_REQ,__func__,"(req=%p) calling helper function _req_pipe_dump",req);
			ws = _req_pipe_dump(req);
			dbgprint(MOD_REQ,__func__,"(req=%p) helper function _req_pipe_dump returned ws=%d",req,ws);
			break;
		default:
			dbgprint(MOD_REQ,__func__,"(req=%p) request has an invalid or unsupported stype (check req pointer...)",req);
			goto return_fail;
	}

	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) helper function failed, aborting",req);
		goto return_fail;
	}

	DBGRET_SUCCESS(MOD_REQ);

return_fail:
	DBGRET_FAILURE(MOD_REQ);
}

/*
   req_to_text

   Helper function to convert a request to a new request with text form.
   Client code is responsible to free the new allocated request if the function
   succeds.
*/
wstatus
req_to_text(request_t req,request_t *req_text)
{
	wstatus ws;
	dbgprint(MOD_REQ,__func__,"called with req=%p, req_text=%p",req,req_text);

	switch(req->stype)
	{
		case REQUEST_STYPE_BIN:
			dbgprint(MOD_REQ,__func__,"(req=%p) calling helper function _req_from_bin_to_text",req);
			ws = _req_from_bin_to_text(req,req_text);
			dbgprint(MOD_REQ,__func__,"(req=%p) helper function _req_from_bin_to_text returned ws=%d",req,ws);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_REQ,__func__,"(req=%p) helper function failed, aborting",req);
				goto return_fail;
			}
			break;
		case REQUEST_STYPE_TEXT:
			dbgprint(MOD_REQ,__func__,"(req=%p) request is already in text data structure");
			goto return_fail;
		case REQUEST_STYPE_PIPE:
			dbgprint(MOD_REQ,__func__,"(req=%p) calling helper function _req_from_pipe_to_text",req);
			ws = _req_from_pipe_to_text(req,req_text);
			dbgprint(MOD_REQ,__func__,"(req=%p) helper function _req_from_pipe_to_text returned ws=%d",req,ws);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_REQ,__func__,"(req=%p) helper function failed, aborting",req);
				goto return_fail;
			}
			break;
		default:
			dbgprint(MOD_REQ,__func__,"(req=%p) request has an invalid or unsupported stype (check req pointer...)",req);
			goto return_fail;
	}

	DBGRET_SUCCESS(MOD_REQ);

return_fail:
	DBGRET_FAILURE(MOD_REQ);
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
	nvpair_fflag_list fflags;
	wstatus ws;

	/* check if this nvpair has a value associated first */
	if( !nvp->value_size ) {
		/* name_size + ' ' */
		*nvl_size += nvp->name_size + 1;
		return;
	}

	/* get nvpair appropriate value format */
	ws = _nvp_value_format(nvp->value_ptr,nvp->value_size,&fflags);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"unable to get value format (nvp=%p)",nvp);
		return;
	}

	ws =_nvp_value_encoded_size(nvp->value_ptr,nvp->value_size,&value_size);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"unable to get encoded value size (nvp=%p)",nvp);
		return;
	}

	if( nvp_flag_test(fflags,NVPAIR_FFLAG_UNQUOTED) )
		/* name_size + '=' + value_size + ' ' */
		*nvl_size += nvp->name_size + 1 + nvp->value_size + 1;
	else if( nvp_flag_test(fflags,NVPAIR_FFLAG_QUOTED) )
		/* name_size + '=' + '"' + value_size + '"' + ' '   */
		*nvl_size += nvp->name_size + 1 + 1 + nvp->value_size + 1 + 1;
	else if( nvp_flag_test(fflags,NVPAIR_FFLAG_ENCODED) )
		/* name_size + '#' + value_size + ' ' */
		*nvl_size += nvp->name_size + 1 + value_size;
	else
		dbgprint(MOD_REQ,__func__,"unexpected/invalid fflag value (%d, %s)",
				fflags,nvp_fflags_str(fflags));

	return;
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
	nvpair_fflag_list fflags;
	char *value_encoded;
	wstatus ws;

	req_text = req_text + strlen(req_text); /* points now to null char */

	dbgprint(MOD_REQ,__func__,"writing name into req_text (%p), length is %u bytes long",
			req_text,nvp->name_size);

	memcpy(req_text,nvp->name_ptr,nvp->name_size);
	req_text[nvp->name_size] = '\0';

	dbgprint(MOD_REQ,__func__,"copied name into req_text successfully");

	if( !nvp->value_size ) {
		req_text[nvp->name_size] = ' ';
		req_text[nvp->name_size + 1] = '\0';
		return;
	}

	ws = _nvp_value_format(nvp->value_ptr,nvp->value_size,&fflags);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"nvp_value_format function failed");
		return;
	}
	dbgprint(MOD_REQ,__func__,"value format appropriate for this value is %d (%s)",
			fflags,nvp_fflags_str(fflags));

	/* convert the value to the correct format if it requires any */
	req_text += nvp->name_size;
	*req_text = '=';
	req_text++;

	if( nvp_flag_test(fflags,NVPAIR_FFLAG_UNQUOTED) )
	{
			dbgprint(MOD_REQ,__func__,"writing value into req_text (%p), length is %u bytes long",
					req_text,nvp->value_size);
			memcpy(req_text,nvp->value_ptr,nvp->value_size);
			req_text[nvp->value_size] = ' ';
			req_text[nvp->value_size + 1] = '\0';
			return;
	} else if ( nvp_flag_test(fflags,NVPAIR_FFLAG_QUOTED) )
	{
			*req_text = '"';
			dbgprint(MOD_REQ,__func__,"writing value into req_text (%p), length is %u bytes long",
					req_text,nvp->value_size);
			memcpy(req_text+1,nvp->value_ptr,nvp->value_size);
			req_text += nvp->value_size + 1;
			*req_text = '"';
			req_text++;
			*req_text = ' ';
			req_text++;
			*req_text = '\0';
			return;
	} else if ( nvp_flag_test(fflags,NVPAIR_FFLAG_ENCODED) )
	{
			/* convert the value to a good format */
			ws = _nvp_value_encode(nvp->value_ptr,nvp->value_size,&value_encoded);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_REQ,__func__,"unable to convert value to encoded format");
				return;
			}
			req_text = stpcpy(req_text,value_encoded);
			*req_text = ' ';
			*(req_text+1) = '\0';
			free(value_encoded);
			return;
	} else
	{
			dbgprint(MOD_REQ,__func__,"invalid or unsupported value format %d (%s)",
					fflags,nvp_fflags_str(fflags));
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

	dbgprint(MOD_REQ,__func__,"called with req=%p, req_text=%p",req,req_text);

	/* validate arguments */

	if( !req ) {
		dbgprint(MOD_REQ,__func__,"invalid req argument (req=0)");
		goto return_fail;
	}

	if( !req_text ) {
		dbgprint(MOD_REQ,__func__,"invalid req_text argument (req_text=0)");
		goto return_fail;
	}

	if( req->stype != REQUEST_STYPE_BIN ) {
		dbgprint(MOD_REQ,__func__,"invalid request to be used with this function");
		goto return_fail;
	}

	/* we need to calculate the request size, because the data structure for it in text form
	   should contain all the request in text form. */

	if( req->data.bin.id > MAXREQID ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) request id exceeds the limit (%d)",req,MAXREQID);
		goto return_fail;
	}

	if( !snprintf(req_id,sizeof(req_id),"%u",req->data.bin.id) ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) unable to convert request id to string",req);
		goto return_fail;
	}
	dbgprint(MOD_REQ,__func__,"(req=%p) id length in chars is %d",req,strlen(req_id));
	req_size += strlen(req_id);

	if( req->data.bin.type == REQUEST_TYPE_REQUEST ) {
		/* request type is request, dont use any char in TYPE token */
		req_type[0] = '\0';
	} else if ( req->data.bin.type == REQUEST_TYPE_REPLY ) {
		/* request type is reply */
		req_type[0] = 'R';
		req_type[1] = '\0';
	} else {
		dbgprint(MOD_REQ,__func__,"(req=%p) invalid or unexpected request type",req);
		goto return_fail;
	}
	dbgprint(MOD_REQ,__func__,"(req=%p) type length in chars is %d",req,strlen(req_type));
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

	dbgprint(MOD_REQ,__func__,"(req=%p) source, destination and code length in chars is %d",
			req, strlen(req_src) + strlen(req_dst) + strlen(req_code));

	req_size += strlen(req_src) + strlen(req_dst) + strlen(req_code);
	
	/* take care of nv-pairs */

	jmls = jmlist_entry_count(req->data.bin.nvl,&nv_count);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) jmlist_entry_count failed with jmls=%d",req,jmls);
		goto return_fail;
	}

	if( !nv_count )
		goto skip_nvl_size;

	dbgprint(MOD_REQ,__func__,"(req=%p) this request has %d nv-pair(s)",req,nv_count);

	/* count the SEP between CODE and NVL */
	req_size += 1;

	jmls = jmlist_parse(req->data.bin.nvl,_req_bin_nvl_sum_length_jlcb,&nvl_size);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) failed to parse nvpair list (length)",req);
		goto return_fail;
	}
	dbgprint(MOD_REQ,__func__,"(req=%p) nvlist calculated size is %d bytes long",req,nvl_size);
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
	dbgprint(MOD_REQ,__func__,"(req=%p) allocated a new request ptr=%p",req,req_ptr);

	req_ptr->stype = REQUEST_STYPE_TEXT;

	if( !snprintf(req_ptr->data.text.raw,req_size,"%s%s %s %s %s",req_id,req_type,req_src,req_dst,req_code) )
	{
		dbgprint(MOD_REQ,__func__,"(req=%p) failed to fill new request text data",req);
		goto return_fail;
	}
	dbgprint(MOD_REQ,__func__,"(req=%p) request head was built \"%s\"",req,req_ptr->data.text.raw);

	if( !nv_count )
		goto skip_nvl_insert;

	/* NVL INSERT HERE */

	strcat(req_ptr->data.text.raw," ");

	jmls = jmlist_parse(req->data.bin.nvl,_req_bin_nvl_insert_jlcb,
			req_ptr->data.text.raw + strlen(req_ptr->data.text.raw) );
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		/* this would be odd since we've already used the parser before in this function */
		dbgprint(MOD_REQ,__func__,"(req=%p) failed to parse nvpair list (insert)",req);
		goto return_fail;
	}
	dbgprint(MOD_REQ,__func__,"(req=%p) finished insertion of %u nv-pairs",req,nv_count);

	/* finish insertion of nv-pairs cut the last space */
	dbgprint(MOD_REQ,__func__,"(req=%p) removing trail space character",req);
	if( (aux = strrchr(req_ptr->data.text.raw,' ')) )
		*aux = '\0';

skip_nvl_insert:

	*req_text = req_ptr;
	dbgprint(MOD_REQ,__func__,"(req=%p) updated req_text to %p",req,*req_text);

	DBGRET_SUCCESS(MOD_REQ);

return_fail:

	if( req_ptr ) {
		free(req_ptr);
	}

	DBGRET_FAILURE(MOD_REQ);
}

wstatus
_req_from_pipe_to_text(request_t req,request_t *req_text)
{
	return WSTATUS_UNIMPLEMENTED;
}

/*
   req_diff

   Helper function to dump request differences, usefull for request conversion
   functions testing. Requests must be of the same type.
*/
wstatus
req_diff(request_t req1_ptr,char *req1_label,request_t req2_ptr,char *req2_label)
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
   req_from_string

   Public function that builds a TEXT type request from a string.
   The string should contain all the request, this string pointer
   wont be referenced in the request so the client code might free it.

   This function allocates a new request data structure which is
   stored in req_text and the client code is responsible for freeing it.
*/
wstatus
req_from_string(const char *raw_text,request_t *req_text)
{
	request_t req = 0;
	size_t req_size;

	dbgprint(MOD_REQ,__func__,"called with raw_text=%p, req_text=%p",raw_text,req_text);

	if( !raw_text ) {
		dbgprint(MOD_REQ,__func__,"invalid raw_text argument (raw_text=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	if( !req_text ) {
		dbgprint(MOD_REQ,__func__,"invalid req_text argument (req_text=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	req_size = strlen(raw_text) + sizeof(struct _request_t) + 1;
	req = (request_t)malloc(req_size);
	if( !req ) {
		dbgprint(MOD_REQ,__func__,"malloc failed for size %d",req_size);
		DBGRET_FAILURE(MOD_REQ);
	}

	dbgprint(MOD_REQ,__func__,"allocated request memory successfully (ptr=%p)",req);
	memset(req,0,req_size);
	req->stype = REQUEST_STYPE_TEXT;
	strcpy(req->data.text.raw,raw_text);

	*req_text = req;
	dbgprint(MOD_REQ,__func__,"updated req_text value to %p",*req_text);

	DBGRET_SUCCESS(MOD_REQ);

/*
return_fail:
	if( req )
		free(req);

	DBGRET_FAILURE(MOD_REQ); */
}

/*
   req_add_nvp_z

   Helper function of the req family for adding a nv-pair to the request.
   The name isn't checked for existence in the nv-pair list. When there is
   no value associated, use value_ptr=0. Both name_ptr and value_ptr are
   expected to be null terminated strings. The null characters are not included
   in the request nvpair NAME and VALUE.
*/
wstatus
req_add_nvp_z(const char *name_ptr,const char *value_ptr,request_t req)
{
	unsigned int value_size;
	nvpair_t nvp = 0;
	jmlist jml = 0;
	jmlist_status jmls;
	wstatus ws;
	struct _jmlist_params params = {.flags = JMLIST_LINKED};
	unsigned int entry_count;

	dbgprint(MOD_REQ,__func__,"called with name_ptr=%p, value_ptr=%p, req=%p",
			name_ptr,value_ptr,req);

	if( !name_ptr || !strlen(name_ptr) ) {
		dbgprint(MOD_REQ,__func__,"invalid name_ptr argument (name_ptr=0 or strlen(name_ptr)=0)");
		goto return_fail;
	}

	if( !req ) {
		dbgprint(MOD_REQ,__func__,"invalid req argument (req=0)");
		goto return_fail;
	}

	if( req->stype == REQUEST_STYPE_BIN )
	{
		value_size = value_ptr ? strlen(value_ptr) : 0;
		ws = _nvp_alloc(strlen(name_ptr),value_size,&nvp);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_REQ,__func__,"failed to allocated new nvpair (ws=%s)",wstatus_str(ws));
			goto return_fail;
		}
		dbgprint(MOD_REQ,__func__,"allocated new nvpair (nvp=%p) successfully",nvp);

		ws = _nvp_fill(name_ptr,strlen(name_ptr),value_ptr,value_size,nvp);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_REQ,__func__,"failed to fill the new nvpair (ws=%s)",wstatus_str(ws));
			goto return_fail;
		}
		dbgprint(MOD_REQ,__func__,"filled the new nvpair successfully");

		if( !req->data.bin.nvl )
	   	{
			/* allocate new jmlist for the nvpairs */
			jmls = jmlist_create(&jml,&params);
			if( jmls != JMLIST_ERROR_SUCCESS ) {
				dbgprint(MOD_REQ,__func__,"failed to create jmlist (jmls=%d)",jmls);
				jml = 0;
				goto return_fail;
			}
			dbgprint(MOD_REQ,__func__,"created jmlist for the nvl successfully (jml=%p)",jml);
			req->data.bin.nvl = jml;
			dbgprint(MOD_REQ,__func__,"updated request nvl to %p",req->data.bin.nvl);
		}

		jmls = jmlist_insert(jml,nvp);
		if( jmls != JMLIST_ERROR_SUCCESS ) {
			dbgprint(MOD_REQ,__func__,"failed to insert nvpair into nvl (jmls=%d)",jmls);
			goto return_fail;
		}

		entry_count = 0;
		jmls = jmlist_entry_count(jml,&entry_count);
		dbgprint(MOD_REQ,__func__,"inserted nvpair into nvl successfully (nvl has now %u entries)",entry_count);

	} else if( req->stype == REQUEST_STYPE_TEXT ) {
		/* TODO */
	} else {
		dbgprint(MOD_REQ,__func__,"invalid or unsupported request stype (%d)",req->stype);
		goto return_fail;
	}
	DBGRET_SUCCESS(MOD_REQ);
return_fail:
	if( jml )
		jmlist_free(jml);

	if( nvp )
		_nvp_free(nvp);

	DBGRET_FAILURE(MOD_REQ);
}

/*
   req_free

   As the name says, this function frees any data structure associated to the request.
*/
wstatus
req_free(request_t req)
{
	jmlist_status jmls;

	dbgprint(MOD_REQ,__func__,"called with req=%p");
	if( !req ) {
		dbgprint(MOD_REQ,__func__,"invalid req argument (req=0)");
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
					dbgprint(MOD_REQ,__func__,"failed to free request nv-pair list (jmls=%d",jmls);
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
			dbgprint(MOD_REQ,__func__,"invalid or unsupported request type (%d)",req->stype);
			goto return_fail;
	}

	/* free the request data structure */
	dbgprint(MOD_REQ,__func__,"freeing request data structure");
	free(req);

	DBGRET_SUCCESS(MOD_REQ);

return_fail:
	DBGRET_FAILURE(MOD_REQ);
}

/*
   req_get_nv

   Helper function that searches a name-value pair in a request, it uses the name as the needle.
   The name-value pair is returned in a nvpair_t structure, the client passes the address of
   a variable that receives a nvpair_t pointer. The client is responsible for freeing the nvp
   data structure returned using _nvp_free.
*/
wstatus
req_get_nv(const request_t req,const char *name_ptr,unsigned int name_size,nvpair_t *nvpp)
{
	wstatus ws;

	dbgprint(MOD_REQ,__func__,"called with req=%p, name_ptr=%p (%s), name_size=%u, nvpp=%p",
			req,name_ptr,array2z(name_ptr,name_size),name_size,nvpp);

	/* start by validating the arguments */

	if( !req ) {
		dbgprint(MOD_REQ,__func__,"invalid req argument (req=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	if( !name_ptr ) {
		dbgprint(MOD_REQ,__func__,"invalid name_ptr argument (name_ptr=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	if( !name_size ) {
		dbgprint(MOD_REQ,__func__,"invalid name_size (name_size=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	switch(req->stype)
	{
		case REQUEST_STYPE_TEXT:
			dbgprint(MOD_REQ,__func__,"request in req=%p is of TEXT stype, calling helper function _req_text_get_nv");
			ws = _req_text_get_nv(req,name_ptr,name_size,nvpp);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_REQ,__func__,"helper function _req_text_get_nv failed (ws=%d)",ws);
				goto return_fail;
			}
			dbgprint(MOD_REQ,__func__,"helper function _req_text_get_nv was successful");
			break;
		case REQUEST_STYPE_BIN:
			dbgprint(MOD_REQ,__func__,"request in req=%p is of BIN stype, calling helper function _req_bin_get_nv");
			ws = _req_bin_get_nv(req,name_ptr,name_size,nvpp);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_REQ,__func__,"helper function _req_bin_get_nv failed (ws=%d)");
				goto return_fail;
			}
			dbgprint(MOD_REQ,__func__,"helper function _req_bin_get_nv was successful");
			break;
		case REQUEST_STYPE_PIPE:
		default:
			dbgprint(MOD_REQ,__func__,"invalid or unsupported request stype (%d)",req->stype);
			DBGRET_FAILURE(MOD_REQ);
	}

	DBGRET_SUCCESS(MOD_REQ);
return_fail:
	DBGRET_FAILURE(MOD_REQ);
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
	nvpair_fflag_list fflags;
	nvpair_t aux_nvp = 0;

	dbgprint(MOD_REQ,__func__,"called with req=%p, name_ptr=%p (%s), name_size=%u, nvpp=%p",
			req,look_name_ptr,array2z(look_name_ptr,look_name_size),look_name_size,nvpp);

	/* start by validating the arguments */

	if( !req ) {
		dbgprint(MOD_REQ,__func__,"invalid req argument (req=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	if( !look_name_ptr ) {
		dbgprint(MOD_REQ,__func__,"invalid look_name_ptr argument (look_name_ptr=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	if( !look_name_size ) {
		dbgprint(MOD_REQ,__func__,"invalid look_name_size (look_name_size=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	if( req->stype != REQUEST_STYPE_TEXT ) {
		dbgprint(MOD_REQ,__func__,"invalid request to be used with this function");
		DBGRET_FAILURE(MOD_REQ);
	}

	/* seek to name-value pair list in the request */

	ws = _req_text_token_seek(req->data.text.raw,TEXT_TOKEN_NVL,&aux);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) unable to find nvl begin",req);
		goto return_fail;
	}

	for( nv_idx = 1 ; ; nv_idx++ )
   	{
		/* try to parse nvpair text tokens */
		ws = _req_text_nv_parse(aux,&name_start,&name_size,&value_start,&value_size,&fflags);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) failed to process text request",req);
			goto return_fail;
		}


		if( name_size != look_name_size ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) this nvpair has different name size, seeking",req);
			goto seek_nvp;
		}

		/* compare name with the lookup name */

		if( memcmp(name_start,look_name_ptr,name_size) ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) this nvpair has different name, seeking",req);
			goto seek_nvp;
		}
		dbgprint(MOD_REQ,__func__,"(req=%p) nvpair was found successfully",req);

		if( nvp_flag_test(fflags,NVPAIR_FFLAG_ENCODED) )
	   	{
			ws = _nvp_value_decoded_size(value_start,value_size,&decoded_size);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_REQ,__func__,"(req=%p) failed to get decoded size for nv_idx=%u",req,nv_idx);
				goto return_fail;
			}

			/* allocate buffer that will store the decoded value */

			decoded_ptr = (char*)malloc(decoded_size);
			if( !decoded_ptr ) {
				dbgprint(MOD_REQ,__func__,"(req=%p) malloc failed on decoded_size=%u",req,decoded_size);
				goto return_fail;
			}
			dbgprint(MOD_REQ,__func__,"(req=%p) allocated buffer for decoded value successful (ptr=%p)",
					req,decoded_ptr);

			ws = _nvp_value_decode(value_start,value_size,decoded_ptr,decoded_size);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_REQ,__func__,"(req=%p) unable to decode value (ws=%s)",req,wstatus_str(ws));
				goto return_fail;
			}
			dbgprint(MOD_REQ,__func__,"(req=%p) decoded successfully the value",req);
		}

		/* allocate new nvpair data structure for this nvpair */

		if( nvp_flag_test(fflags,NVPAIR_FFLAG_ENCODED) )
			ws = _nvp_alloc(name_size,decoded_size,&aux_nvp);
		else
			ws = _nvp_alloc(name_size,value_size,&aux_nvp);

		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) failed to allocate a new nvpair (name_size=%u, value_size=%u)",
					req,name_size,value_size);
			goto return_fail;
		}
		dbgprint(MOD_REQ,__func__,"(req=%p) allocated new nvpair data structure successfully (p=%p)",req,aux_nvp);

		/* fill the new data structure with the tokens */
		
		if( nvp_flag_test(fflags,NVPAIR_FFLAG_ENCODED) )
			ws = _nvp_fill(name_start,name_size,decoded_ptr,decoded_size,aux_nvp);
		else
			ws = _nvp_fill(name_start,name_size,value_start,value_size,aux_nvp);

		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) failed to fill the new nvpair (p=%p)",req,aux_nvp);
			goto return_fail;
		}
		dbgprint(MOD_REQ,__func__,"(req=%p) new nvpair was filled successfully",req);

		/* we can now free the decoded buffer if it was used */
		if( decoded_ptr ) {
			free(decoded_ptr);
			decoded_ptr = 0;
			decoded_size = 0;
		}

		/* return this nvpair to the calling function */

		*nvpp = aux_nvp;
		dbgprint(MOD_REQ,__func__,"(req=%p) updated nvpp value to %p",req,*nvpp);
		DBGRET_SUCCESS(MOD_REQ);

seek_nvp:
		if( value_start ) {
			aux = value_start + value_size;
			if( V_QUOTECHAR(*aux) )
				aux++;
		} else
			 aux = name_start + name_size;

		if( V_REQENDCHAR(*aux) ) {
			/* end of request was reached */
			dbgprint(MOD_REQ,__func__,"(req=%p) end of nvpair list");
			break;
		}

		if( *aux != ' ' ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) [nv_idx=%u] invalid/unexpected character after nvpair",req,nv_idx);
			goto return_fail;
		}

		aux++;
	}

	/* parsed all the nvpairs but didn't found the wanted nvp */

	dbgprint(MOD_REQ,__func__,"(req=%p) the nvpair with name \"%s\" was not found in this request",
			req,array2z(look_name_ptr,look_name_size));
	goto return_fail;

return_fail:
	if( decoded_ptr )
		free(decoded_ptr);

	if( aux_nvp ) {
		_nvp_free(aux_nvp);
	}

	DBGRET_FAILURE(MOD_REQ);
}

/*
   req_get_nv_count

   Helper function to get number of name-value pairs that a request has. The
   number of name-value pairs returned is one-based. If the request doesn't
   have any name-value pair, nv_count is set to 0 (zero).
*/
wstatus
req_get_nv_count(const struct _request_t *req,unsigned int *nv_count)
{
	wstatus ws;
	jmlist_status jmls;
	unsigned int entry_count;

	dbgprint(MOD_REQ,__func__,"called with req=%p, nv_count=%p",req,nv_count);

	/* validate arguments */

	if( !req ) {
		dbgprint(MOD_REQ,__func__,"invalid req argument (req=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	if( !nv_count ) {
		dbgprint(MOD_REQ,__func__,"invalid nv_count argument (nv_count=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	switch(req->stype)
	{
		case REQUEST_STYPE_TEXT:
			
			dbgprint(MOD_REQ,__func__,"(req=%p) request type is text",req);

			dbgprint(MOD_REQ,__func__,"(req=%p) calling helper function",req);
			ws = _req_text_nv_count(req,&entry_count);
			dbgprint(MOD_REQ,__func__,"(req=%p) helper function returned (ws=%d)",req,ws);

			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_REQ,__func__,"(req=%p) failed to get request nv-count (helper function failed with ws=%s)",
						req,wstatus_str(ws));
				DBGRET_FAILURE(MOD_REQ);
			}
			dbgprint(MOD_REQ,__func__,"(req=%p) helper function was successful (%u)",req,entry_count);

			*nv_count = entry_count;
			dbgprint(MOD_REQ,__func__,"(req=%p) nv_count value updated to %u",req,*nv_count);
			DBGRET_SUCCESS(MOD_REQ);
			
			break;	
		case REQUEST_STYPE_BIN:

			dbgprint(MOD_REQ,__func__,"(req=%p) request stype is binary",req);

			dbgprint(MOD_REQ,__func__,"(req=%p) calling jmlist_entry_count with jml=%p",
					req,req->data.bin.nvl);

			jmls = jmlist_entry_count(req->data.bin.nvl,&entry_count);
			if( jmls != JMLIST_ERROR_SUCCESS ) {
				dbgprint(MOD_REQ,__func__,"(req=%p) jmlist_entry_count failed with jmls=%d",req,jmls);
				DBGRET_FAILURE(MOD_REQ);
			}
			dbgprint(MOD_REQ,__func__,"(req=%p) jmlist_entry_count was successful (%u)",req,entry_count);

			*nv_count = entry_count;
			dbgprint(MOD_REQ,__func__,"(req=%p) nv_count value updated to %u",req,*nv_count);
			DBGRET_SUCCESS(MOD_REQ);

			break;
		case REQUEST_STYPE_PIPE:
		default:
			dbgprint(MOD_REQ,__func__,"(req=%p) request has an invalid or unsupported stype (check req pointer...)",req);
	}

	DBGRET_FAILURE(MOD_REQ);
}

/*
   _req_bin_get_nv

   Helper function that gets the name-value pair in a binary stype request.
   If the request doesn't have any name-value pair or if the name-value isn't found
   this function returns failure. If the client code wants to simply check for the
   existence of the name-value pair, req_get_nv_info function should be used instead.
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

	dbgprint(MOD_REQ,__func__,"called with req=%p, name_ptr=%p (%s), name_size=%u, nvpp=%p",
			req,look_name_ptr,array2z(look_name_ptr,look_name_size),look_name_size,nvpp);

	/* start by validating the arguments */

	if( !req ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) invalid req argument (req=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	if( !look_name_ptr ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) invalid look_name_ptr argument (look_name_ptr=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	if( !look_name_size ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) invalid look_name_size (look_name_size=0)",req);
		DBGRET_FAILURE(MOD_REQ);
	}

	if( req->stype != REQUEST_STYPE_BIN ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) invalid request to be used with this function",req);
		DBGRET_FAILURE(MOD_REQ);
	}

	if( !nvpp ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) invalid nvpp argument (nvpp=0)",req);
		DBGRET_FAILURE(MOD_REQ);
	}

	dbgprint(MOD_REQ,__func__,"(req=%p) calling helper function to get name-value count",req);
	ws = req_get_nv_count(req,&nv_count);
	dbgprint(MOD_REQ,__func__,"(req=%p) helper function returned (ws=%s)",req,wstatus_str(ws));
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) failed to get name-value count (helper function failed with ws=%s)",
				req,wstatus_str(ws));
		DBGRET_FAILURE(MOD_REQ);
	}
	dbgprint(MOD_REQ,__func__,"(req=%p) helper function was successful (nv_count=%u)",req,nv_count);

	if( nv_count == 0 ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) this request doesn't have any name-value pair",req);
		goto return_fail;
	}

	/* parse all nvpair from the name-value pair list of the binary stype request */

	jmls = jmlist_seek_start(req->data.bin.nvl,&shandle);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) failed to start seek throughout the name-value list "
				"(jmlist function failed with jmls=%d)",req,jmls);
		goto return_fail;
	}

	nv_idx = 0;
	*nvpp = 0;
	while( nv_count-- )
	{
		jmls = jmlist_seek_next(req->data.bin.nvl,&shandle,&aux_ptr);
		if( jmls != JMLIST_ERROR_SUCCESS ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) failed to seek throughout the name-value list "
					"(jmlist function failed with jmls=%d)",req,jmls);

			/* need to end jmlist seeking before returning.. */
			jmls = jmlist_seek_end(req->data.bin.nvl,&shandle);
			if( jmls != JMLIST_ERROR_SUCCESS ) {
				dbgprint(MOD_REQ,__func__,"(req=%p) failed to end jmlist seeking (jmls=%d)",req,jmls);
			}

			goto return_fail;
		}

		nvp_seek = (nvpair_t)aux_ptr;

		dbgprint(MOD_REQ,__func__,"(req=%p) testing nvpair (idx=%u) name lengths",req,nv_idx);
		if( nvp_seek->name_size != look_name_size ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) this nvpair (idx=%u) has different name size, seeking",
					req,nv_idx);
			goto seek_nvp;
		}
		dbgprint(MOD_REQ,__func__,"(req=%p) nvpair (idx=%u) has same length as the looking nvpair",req,nv_idx);

		/* compare name with the lookup name */

		dbgprint(MOD_REQ,__func__,"(req=%p) comparing nvpair (idx=%u) name characters",req,nv_idx);
		if( memcmp(nvp_seek->name_ptr,look_name_ptr,look_name_size) ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) this nvpair (idx=%u) has different name, seeking",
					req,nv_idx);
			goto seek_nvp;
		}
		dbgprint(MOD_REQ,__func__,"(req=%p) nvpair was found successful in the list (idx=%u)",req,nv_idx);

		/* duplicate nvpair memory data structure to pass to the caller */

		ws = _nvp_dup(nvp_seek,&aux_nvp);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) unable to duplicate request nvpair (idx=%u)",req,nv_idx);
			*nvpp = 0;
			
			/* need to end jmlist seeking before returning.. */
			jmls = jmlist_seek_end(req->data.bin.nvl,&shandle);
			if( jmls != JMLIST_ERROR_SUCCESS ) {
				dbgprint(MOD_REQ,__func__,"(req=%p) failed to end jmlist seeking (jmls=%d)",req,jmls);
			}

			goto return_fail;
		}
		dbgprint(MOD_REQ,__func__,"(req=%p) nvpair duplicated successfully (nvp=%p)",req,aux_nvp);


		/* do the cleanup */

		jmls = jmlist_seek_end(req->data.bin.nvl,&shandle);
		if( jmls != JMLIST_ERROR_SUCCESS ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) failed to end jmlist seeking (jmls=%d)",req,jmls);
			goto return_fail;
		}

		*nvpp = aux_nvp;
		dbgprint(MOD_REQ,__func__,"(req=%p) nvpp value updated to %p",req,*nvpp);

		DBGRET_SUCCESS(MOD_REQ);

seek_nvp:
		nv_idx++;
	}

	dbgprint(MOD_REQ,__func__,"(req=%p) unable to find the wanted nvpair",req);

	/* need to end jmlist seeking before returning.. */
	jmls = jmlist_seek_end(req->data.bin.nvl,&shandle);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) failed to end jmlist seeking (jmls=%d)",req,jmls);
	}

	DBGRET_FAILURE(MOD_REQ);

return_fail:
	if( aux_nvp ) {
		_nvp_free(aux_nvp);
		aux_nvp = 0;
	}

	DBGRET_FAILURE(MOD_REQ);
}

/*
   req_get_nv_info

   Helper function that obtains a specific nvpair informations. These informations are saved
   in a data structure that is passed by the caller, nvpair_info_t see the header file for
   more information about the information that is possible to obtain for each nvpair.

   The implementation of this function is based on the code from req_get_nv except that
   when it finds the wanted name-value pair, it obtains the characteristics of the name-value
   pair and saves that information into nvpi structure.

   How the information is gathered? _nvp_text_... _req_nv...?
*/
wstatus req_get_nv_info(const struct _request_t *req,const char *look_name_ptr,unsigned int look_name_size,nvpair_info_t nvpi)
{
	wstatus ws;

	dbgprint(MOD_REQ,__func__,"called with req=%p, nvpi=%p",req,nvpi);

	/* validate arguments */

	if( !req ) {
		dbgprint(MOD_REQ,__func__,"invalid req argument (req=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	if( !nvpi ) {
		dbgprint(MOD_REQ,__func__,"invalid nvpi argument (nvpi=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	if( !look_name_ptr ) {
		dbgprint(MOD_REQ,__func__,"invalid look_name_ptr argument (name_ptr=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	if( !look_name_size ) {
		dbgprint(MOD_REQ,__func__,"invalid look_name_size argument (name_size=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	switch(req->stype)
	{
		case REQUEST_STYPE_TEXT:
			dbgprint(MOD_REQ,__func__,"(req=%p) request is in text format",req);
			dbgprint(MOD_REQ,__func__,"(req=%p) calling helper function",req);
			ws = _req_text_get_nv_info(req,look_name_ptr,look_name_size,nvpi);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_REQ,__func__,"(req=%p) failed to get name-value pair informations "
						"(helper function failed with ws=%s)",req,wstatus_str(ws));
				goto return_fail;
			}
			dbgprint(MOD_REQ,__func__,"(req=%p) helper function returned OK",req);
			break;
		case REQUEST_STYPE_BIN:
			dbgprint(MOD_REQ,__func__,"(req=%p) request is in binary format",req);
			dbgprint(MOD_REQ,__func__,"(req=%p) calling helper function",req);
			ws = _req_bin_get_nv_info(req,look_name_ptr,look_name_size,nvpi);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_REQ,__func__,"(req=%p) failed to get name-value pair informations "
						"(helper function failed with ws=%s)",req,wstatus_str(ws));
				goto return_fail;
			}
			dbgprint(MOD_REQ,__func__,"(req=%p) helper function returned OK",req);
			break;
		case REQUEST_STYPE_PIPE:
		default:
			dbgprint(MOD_REQ,__func__,"(req=%p) request has an invalid or unsupported stype (check req pointer...)",req);
			DBGRET_FAILURE(MOD_REQ);
	}

	DBGRET_SUCCESS(MOD_REQ);

return_fail:
	DBGRET_FAILURE(MOD_REQ);
}

/*
   _req_bin_get_nv_info

   Helper function that obtains a specific name-value pair informations from
   the request. This helper function will handle binary type requests specifically.
   The encoded size returned in nvpair_info data structure includes any special
   character used to encode the data.
*/
wstatus
_req_bin_get_nv_info(const struct _request_t *req,const char *look_name_ptr,
		unsigned int look_name_size,nvpair_info_t nvpi)
{
	nvpair_t nvp_seek;
	void *aux_ptr;
	jmlist_seek_handle shandle;
	jmlist_status jmls;
	wstatus ws;
	unsigned int nv_idx;
	unsigned int nv_count;
	unsigned int i;
	nvpair_fflag_list fflags;

	dbgprint(MOD_REQ,__func__,"called with req=%p, look_name_ptr=%p (%s), look_name_size=%u, nvpi=%p",
			req,look_name_ptr,z_ptr(look_name_ptr),look_name_size,nvpi);

	/* validate arguments */

	if( !req ) {
		dbgprint(MOD_REQ,__func__,"invalid req argument (req=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	if( !nvpi ) {
		dbgprint(MOD_REQ,__func__,"invalid nvpi argument (nvpi=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	if( !look_name_ptr ) {
		dbgprint(MOD_REQ,__func__,"invalid look_name_ptr argument (look_name_ptr=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	if( !look_name_size ) {
		dbgprint(MOD_REQ,__func__,"invalid look_name_size argument (look_name_size=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	if( req->stype != REQUEST_STYPE_BIN ) {
		dbgprint(MOD_REQ,__func__,"invalid request to be used with this function");
		DBGRET_FAILURE(MOD_REQ);
	}

	dbgprint(MOD_REQ,__func__,"(req=%p) clearing nvpi",req);
	nvpi->flags = NVPAIR_FLAG_UNINITIALIZED;

	dbgprint(MOD_REQ,__func__,"(req=%p) calling helper function to get name-value count",req);
	ws = req_get_nv_count(req,&nv_count);
	dbgprint(MOD_REQ,__func__,"(req=%p) helper function returned (ws=%s)",req,wstatus_str(ws));
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) failed to get name-value count (helper function failed with ws=%s)",
				req,wstatus_str(ws));
		DBGRET_FAILURE(MOD_REQ);
	}
	dbgprint(MOD_REQ,__func__,"(req=%p) helper function was successful (nv_count=%u)",req,nv_count);

	if( nv_count == 0 )
   	{
		/* update nvpi */
		nvpi->flags = NVPAIR_LFLAG_NOT_FOUND;
		dbgprint(MOD_REQ,__func__,"(req=%p) updated nvpi flags to %d (NVPAIR_LFLAG_NOT_FOUND)",req,nvpi->flags);
		DBGRET_SUCCESS(MOD_REQ);
	}

	/* parse all nvpair from the name-value pair list of the binary stype request */

	jmls = jmlist_seek_start(req->data.bin.nvl,&shandle);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) failed to start seek throughout the name-value list "
				"(jmlist function failed with jmls=%d)",req,jmls);
		DBGRET_FAILURE(MOD_REQ);
	}

	nv_idx = 0;
	while( nv_count-- )
	{
		jmls = jmlist_seek_next(req->data.bin.nvl,&shandle,&aux_ptr);
		if( jmls != JMLIST_ERROR_SUCCESS )
	   	{
			dbgprint(MOD_REQ,__func__,"(req=%p) failed to seek throughout the name-value list "
					"(jmlist function failed with jmls=%d)",req,jmls);

			jmls = jmlist_seek_end(req->data.bin.nvl,&shandle);
			if( jmls != JMLIST_ERROR_SUCCESS ) {
				dbgprint(MOD_REQ,__func__,"(req=%p) failed to end jmlist seeking (jmls=%d)",req,jmls);
			}
			DBGRET_FAILURE(MOD_REQ);
		}

		nvp_seek = (nvpair_t)aux_ptr;

		dbgprint(MOD_REQ,__func__,"(req=%p) testing nvpair (idx=%u) name lengths",req,nv_idx);
		if( nvp_seek->name_size != look_name_size ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) this nvpair (idx=%u) has different name size, seeking",
					req,nv_idx);
			goto seek_nvp;
		}
		dbgprint(MOD_REQ,__func__,"(req=%p) nvpair (idx=%u) has same length as the looking nvpair",req,nv_idx);

		/* compare name with the lookup name */

		dbgprint(MOD_REQ,__func__,"(req=%p) comparing nvpair (idx=%u) name characters",req,nv_idx);
		if( memcmp(nvp_seek->name_ptr,look_name_ptr,look_name_size) ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) this nvpair (idx=%u) has different name, seeking",
					req,nv_idx);
			goto seek_nvp;
		}
		dbgprint(MOD_REQ,__func__,"(req=%p) nvpair was found successful in the list (idx=%u)",req,nv_idx);

		/* do the cleanup */

		jmls = jmlist_seek_end(req->data.bin.nvl,&shandle);
		if( jmls != JMLIST_ERROR_SUCCESS ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) failed to end jmlist seeking (jmls=%d)",req,jmls);
			DBGRET_FAILURE(MOD_REQ);
		}

		nvpi->flags = NVPAIR_LFLAG_FOUND;
		dbgprint(MOD_REQ,__func__,"(req=%p) nvpi flags set to %d (NVPAIR_IFLAG_FOUND)",req,nvpi->flags);

		/* get the name-value pair informations */

		if( !nvp_seek->name_ptr || !nvp_seek->name_size ) {
			nvpi->flags |= NVPAIR_NFLAG_NOT_SET;
			dbgprint(MOD_REQ,__func__,"(req=%p) nvpi flags updated to %d (|NVPAIR_IFLAG_NAME_NOT_SET)",req,nvpi->flags);

			nvpi->name_ptr = 0;
			nvpi->name_size = 0;
			dbgprint(MOD_REQ,__func__,"(req=%p) nvpi name information updated to ptr=%p, size=%u",nvpi->name_ptr,nvpi->name_size);
		} else
		{
			nvpi->name_ptr = nvp_seek->name_ptr;
			nvpi->name_size = nvp_seek->name_size;
			dbgprint(MOD_REQ,__func__,"(req=%p) nvpi name information updated to ptr=%p, size=%u",nvpi->name_ptr,nvpi->name_size);

			/* validate name characters */
			dbgprint(MOD_REQ,__func__,"(req=%p) validating name-value pair name characters",req);
			for( i = 0 ; i < nvpi->name_size ; i++ )
		   	{
				if( V_NAMECHAR(nvpi->name_ptr[i]) )
					continue;

				dbgprint(MOD_REQ,__func__,"(req=%p) found invalid character (%02X) at index %u",req,nvpi->name_ptr[i],i);
				/*nvpi->flags |= NVPAIR_IFLAG_NAME_INVALID; */
				goto name_finished;
			}

			/* all name characters are OK */
			dbgprint(MOD_REQ,__func__,"(req=%p) all name-value pair name characters are acceptable",req);
			nvpi->flags |= NVPAIR_NFLAG_VALID;
			dbgprint(MOD_REQ,__func__,"(req=%p) updated nvpi flags to %d (|NVPAIR_IFLAG_NAME_VALID)",req,nvpi->flags);
		}

name_finished:

		/* now parse the value informations */

		if( !nvp_seek->value_ptr )
		{
			nvpi->flags |= NVPAIR_VFLAG_NOT_SET;
			dbgprint(MOD_REQ,__func__,"(req=%p) updated nvpi flags to %d (|NVPAIR_IFLAG_VALUE_NOT_SET)",req,nvpi->flags);

			nvpi->value_ptr = 0;
			nvpi->value_size = 0;
			nvpi->encoded_size = 0;
			nvpi->flags = NVPAIR_FLAG_UNINITIALIZED;
			dbgprint(MOD_REQ,__func__,"(req=%p) updated nvpi value informations to ptr=%p, size=%u, encoded_size=%u, format=%d",
					req,nvpi->value_ptr,nvpi->value_size,nvpi->encoded_size,nvpi->flags);
		} else if( nvp_seek->value_size )
		{
			/* got ptr and size */
			nvpi->flags |= NVPAIR_VFLAG_VALID;
			dbgprint(MOD_REQ,__func__,"(req=%p) updated nvpi flags to %d (|NVPAIR_VFLAG_VALID)",req,nvpi->flags);

			nvpi->value_ptr = nvp_seek->value_ptr;
			nvpi->value_size = nvp_seek->value_size;

			/* get encoded value informations (format and length in characters) */

			ws = _nvp_value_encoded_size(nvp_seek->value_ptr,nvp_seek->value_size,&nvpi->encoded_size);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_REQ,__func__,"(req=%p) failed to get encoded value size "
						"(helper function failed with ws=%s)",req,wstatus_str(ws));
				DBGRET_FAILURE(MOD_REQ);
			}
			dbgprint(MOD_REQ,__func__,"(req=%p) updated nvpi encoded value size to %u",req,nvpi->encoded_size);

			ws = _nvp_value_format(nvp_seek->value_ptr,nvp_seek->value_size,&fflags);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_REQ,__func__,"(req=%p) failed to get encoded value format "
						"(helper function failed with ws=%s)",req,wstatus_str(ws));
				DBGRET_FAILURE(MOD_REQ);
			}
			dbgprint(MOD_REQ,__func__,"(req=%p) got value format flags %d (%s)",fflags,nvp_iflags_str(fflags));

			nvpi->flags |= fflags;
			dbgprint(MOD_REQ,__func__,"(req=%p) updated nvpi flags to %d (%s)",req,nvpi->flags,nvp_iflags_str(fflags));

			/* finished processing value */
		} else
		{
			/* value_ptr != 0 but value_size = 0... this shouldn't happen.. */
			dbgprint(MOD_REQ,__func__,"(req=%p) invalid name-pair value informations found (ptr!=0,size=0)",req);

			nvpi->flags |= NVPAIR_VFLAG_NOT_SET;
			dbgprint(MOD_REQ,__func__,"(req=%p) updated nvpi flags to %d (|NVPAIR_IFLAG_VALUE_NOT_SET)",req,nvpi->flags);

			nvpi->value_ptr = 0;
			nvpi->value_size = 0;
			nvpi->encoded_size = 0;
			dbgprint(MOD_REQ,__func__,"(req=%p) updated nvpi value informations to ptr=%p, size=%u, encoded_size=%u",
					req,nvpi->value_ptr,nvpi->value_size,nvpi->encoded_size);
		}

		DBGRET_SUCCESS(MOD_REQ);

seek_nvp:
		nv_idx++;
	}

	dbgprint(MOD_REQ,__func__,"(req=%p) unable to find the wanted nvpair (name=%s)",req,array2z(look_name_ptr,look_name_size));

	jmls = jmlist_seek_end(req->data.bin.nvl,&shandle);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) failed to end jmlist seeking (jmls=%d)",req,jmls);
	}

	DBGRET_FAILURE(MOD_REQ);
}

/*
   _req_text_get_nv_info

   Helper function to obtain the name-value pair informations when the request
   is in text format.

   TODO: this function doesn't detect INVALID tokens...
   needs a _req_text_nv_validate instead of _req_text_nv_parse ..
*/
wstatus
_req_text_get_nv_info(const struct _request_t *req,const char *look_name_ptr,
		unsigned int look_name_size,nvpair_info_t nvpi)
{
	char *aux;
	wstatus ws;
	char *l_value_ptr, *l_name_ptr;
	unsigned int l_value_size, l_name_size;
	unsigned int nv_idx;
	nvpair_fflag_list l_fflags;
	nvpair_iflag_list nvp_flags;

	dbgprint(MOD_REQ,__func__,"called with req=%p, look_name_ptr=%p (%s), look_name_size=%u, nvpi=%p",
			req,look_name_ptr,array2z(look_name_ptr,look_name_size),nvpi);

	/* validate arguments */

	if( !req ) {
		dbgprint(MOD_REQ,__func__,"invalid req argument (req=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	if( !nvpi ) {
		dbgprint(MOD_REQ,__func__,"invalid nvpi argument (nvpi=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	if( !look_name_ptr ) {
		dbgprint(MOD_REQ,__func__,"invalid look_name_ptr argument (look_name_ptr=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	if( !look_name_size ) {
		dbgprint(MOD_REQ,__func__,"invalid look_name_size argument (look_name_size=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	if( req->stype != REQUEST_STYPE_TEXT ) {
		dbgprint(MOD_REQ,__func__,"invalid request to be used with this function (expecting REQUEST_STYPE_TEXT)");
		DBGRET_FAILURE(MOD_REQ);
	}

	dbgprint(MOD_REQ,__func__,"(req=%p) clearing nvpi",req);
	nvpi->flags = NVPAIR_FLAG_UNINITIALIZED;

	/* seek to name-value pair list in the request */

	ws = _req_text_token_seek(req->data.text.raw,TEXT_TOKEN_NVL,&aux);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) unable to find nvl begin",req);
		goto return_fail;
	}

	for( nv_idx = 1 ; ; nv_idx++ )
   	{
		/* validate nvpair using helper function */
		ws = _req_text_nv_validate(req,&nvp_flags);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) failed to validate text request nvpair (idx=%u)",req,nv_idx);
			goto return_fail;
		}

		if( !nvp_flag_test(nvp_flags,NVPAIR_NFLAG_VALID) || 
				!nvp_flag_test(nvp_flags,NVPAIR_VFLAG_VALID) )
	   	{
			dbgprint(MOD_REQ,__func__,"(req=%p) detected invalid nvpair (idx=%u) in nvpair list, aborting",req,nv_idx);
			goto return_fail;
		}

		/* try to parse nvpair text tokens */
		ws = _req_text_nv_parse(aux,&l_name_ptr,&l_name_size,&l_value_ptr,&l_value_size,&l_fflags);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) failed to process text request",req);
			goto return_fail;
		}

		if( l_name_size != look_name_size ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) this nvpair has different name size, seeking",req);
			goto seek_nvp;
		}

		/* compare name with the lookup name */

		if( memcmp(l_name_ptr,look_name_ptr,l_name_size) ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) this nvpair has different name, seeking",req);
			goto seek_nvp;
		}
		dbgprint(MOD_REQ,__func__,"(req=%p) nvpair was found successfully",req);
		nvpi->flags = NVPAIR_LFLAG_FOUND | l_fflags;
		dbgprint(MOD_REQ,__func__,"(req=%p) nvpi flags updated to %d (%s)",req,nvpi->flags,
				nvp_iflags_str(nvpi->flags));

		nvpi->name_ptr = l_name_ptr;
		nvpi->name_size = l_name_size;
		nvpi->value_ptr = l_value_ptr;
		nvpi->value_size = l_value_size;

		dbgprint(MOD_REQ,__func__,"(req=%p) nvpi updated name_ptr=%p, name_size=%u, value_ptr=%p, value_size=%u",
				req,nvpi->name_ptr,nvpi->name_size,nvpi->value_ptr,nvpi->value_size);

		nvpi->flags |= NVPAIR_NFLAG_VALID | NVPAIR_VFLAG_VALID;
		dbgprint(MOD_REQ,__func__,"(req=%p) nvpi flags updated to %d (%s)",req,nvpi->flags,nvp_iflags_str(nvpi->flags));

		if( nvp_flag_test(l_fflags,NVPAIR_FFLAG_ENCODED) )
	   	{
			ws = _nvp_value_decoded_size(l_value_ptr,l_value_size,&nvpi->decoded_size);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_REQ,__func__,"(req=%p) failed to get decoded size for nv_idx=%u",req,nv_idx);
				goto return_fail;
			}
			dbgprint(MOD_REQ,__func__,"(req=%p) nvpi updated decoded size value to %u",req,nvpi->decoded_size);
		}

		/* all nvpi data structure was filled */

		DBGRET_SUCCESS(MOD_REQ);

seek_nvp:
		if( l_value_ptr ) {
			aux = l_value_ptr + l_value_size;
			if( V_QUOTECHAR(*aux) )
				aux++;
		} else
			 aux = l_name_ptr + l_name_size;

		if( V_REQENDCHAR(*aux) ) {
			/* end of request was reached */
			dbgprint(MOD_REQ,__func__,"(req=%p) end of nvpair list");
			break;
		}

		if( *aux != ' ' ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) [nv_idx=%u] invalid/unexpected character after nvpair",req,nv_idx);
			goto return_fail;
		}

		aux++;
	}

	/* parsed all the nvpairs but didn't found the wanted nvp */

	dbgprint(MOD_REQ,__func__,"(req=%p) the nvpair with name \"%s\" was not found in this request",
			req,array2z(look_name_ptr,look_name_size));

	nvpi->flags = NVPAIR_LFLAG_NOT_FOUND;
	dbgprint(MOD_REQ,__func__,"(req=%p) nvpi updated flags to %d (NVPAIR_LFLAG_NOT_FOUND)",req,nvpi->flags);

	DBGRET_SUCCESS(MOD_REQ);
	
return_fail:
	DBGRET_FAILURE(MOD_REQ);
}

/*
   _req_text_nv_validate

   Helper function to validate a nvpair which is in text form, this function should
   be used whenever its needed to validate the characters and format of the nvpair
   in text form. First we validate the data only then we use it, if something messes
   up when we're using the data, we'll be sure its not due to the message formating
   and characters.
*/
wstatus
_req_text_nv_validate(const struct _request_t *req,nvpair_iflag_list *iflags)
{
	char *aux;
	char *aux_ref;
	wstatus ws;
	nvpair_iflag_list l_iflags;
	bool encoded = false;
	bool quoted = false;

	dbgprint(MOD_REQ,__func__,"called with req=%p, iflags=%p",req,iflags);

	/* validate arguments */

	if( !req ) {
		dbgprint(MOD_REQ,__func__,"invalid req argument (req=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	if( !iflags ) {
		dbgprint(MOD_REQ,__func__,"invalid iflags argument (iflags=0)");
		DBGRET_FAILURE(MOD_REQ);
	}

	if( req->stype != REQUEST_STYPE_TEXT ) {
		dbgprint(MOD_REQ,__func__,"invalid request to be used with this function (expecting REQUEST_STYPE_TEXT)");
		DBGRET_FAILURE(MOD_REQ);
	}

	dbgprint(MOD_REQ,__func__,"(req=%p) clearing iflags",req);
	l_iflags = NVPAIR_FLAG_UNINITIALIZED;

	dbgprint(MOD_REQ,__func__,"(req=%p) seeking to the name-value list",req);
	ws = _req_text_token_seek(req->data.text.raw,TEXT_TOKEN_NVL,&aux);
	if( ws != WSTATUS_SUCCESS ) {
		DBGRET_FAILURE(MOD_REQ);
	}
	dbgprint(MOD_REQ,__func__,"(req=%p) seeked to the name-value list successfully (ptr=%p)",req,aux);

	aux_ref = aux;

	while(1)
	{
		if( V_REQENDCHAR(*aux) )
			goto name_end_of_req;

		if( V_NVSEPCHAR(*aux) )
			goto end_of_name;

		if( V_TOKSEPCHAR(*aux) )
			goto end_of_name;

		if( !V_NAMECHAR(*aux) )
			goto invalid_name;

		aux++;
	}

	/* didn't reach to the end char.. this shouldn't happen! */

	dbgprint(MOD_REQ,__func__,"(req=%p) couldn't reach to the end of the name in this nvpair",req);
	DBGRET_FAILURE(MOD_REQ);

name_end_of_req:
	if( aux == aux_ref ) {
		/* no name was processed */
		dbgprint(MOD_REQ,__func__,"(req=%p) name is empty",req);
		DBGRET_FAILURE(MOD_REQ);
	}

	/* this name-value pair has only name */
	l_iflags = NVPAIR_NFLAG_VALID;
	goto return_success;

invalid_name:
	/* found invalid character in NAME field */
	dbgprint(MOD_REQ,__func__,"(req=%p) found invalid/unexpected character (0x%02X '%c') "
			"in name parsing at index %u",req,*aux,b2c(*aux),aux - aux_ref);
	goto return_success;

end_of_name:
	if( aux == aux_ref ) {
		/* this happens when the user passes name_ptr="=value" so it doesn't have
		   any name character and the value prefix starts right next. */
		dbgprint(MOD_REQ,__func__,"(req=%p) name is empty (no name character found)",req);
		l_iflags = NVPAIR_NFLAG_NOT_SET;
		goto return_success;
	}
	l_iflags = NVPAIR_NFLAG_VALID;
	
	/* start processing value */
	aux_ref = aux;

	dbgprint(MOD_REQ,__func__,"(req=%p) parsed name successfully, starting value parsing",req);

	if( V_TOKSEPCHAR(*aux) ) {
		l_iflags |= NVPAIR_VFLAG_VALID | NVPAIR_VFLAG_NOT_SET;
		goto return_success;
	}

	if( !V_NVSEPCHAR(*aux) ) {
		dbgprint(MOD_REQ,__func__,"(req=%p) unexpected character found in value prefix (0x%02X '%c') at index %u",
				req,*aux,b2c(*aux),aux - aux_ref);
	}
	aux++;

	if( V_ENCPREFIX(*aux) ) {
		/* value is encoded */
		encoded = true;
		aux++;
	} else if( V_QUOTECHAR(*aux) ) {
		quoted = true;
		aux++;
	}

	while(1)
	{
		if( V_REQENDCHAR(*aux) )
			goto value_end_of_req;

		if( V_TOKSEPCHAR(*aux) )
			goto value_end;

		if( quoted && V_QUOTECHAR(*aux) ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) reached quote character (this value has finished)",req);
			aux++;
			goto value_end;
		}

		if( encoded && !V_EVALUECHAR(*aux) ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) invalid character found in encoded value (0x%02X '%c' at index %u)",
					req,*aux,b2c(*aux),aux - aux_ref);
			goto return_success;
		}

		if( quoted && !V_QVALUECHAR(*aux) ) {
			dbgprint(MOD_REQ,__func__,"(req=%p) invalid character found in quoted value (0x%02X '%c' at index %u)",
					req,*aux,b2c(*aux),aux - aux_ref);
			goto return_success;
		}

		if( !quoted && !encoded && !V_VALUECHAR(*aux) )
		{
			dbgprint(MOD_REQ,__func__,"(req=%p) invalid character found in value (0x%02X '%c' at index %u)",
					req,*aux,b2c(*aux),aux - aux_ref);
			goto return_success;
		}
	}

value_end_of_req:
value_end:
	l_iflags |= NVPAIR_VFLAG_VALID;

return_success:

	*iflags = l_iflags;
	dbgprint(MOD_REQ,__func__,"updated iflags value to %d (%s)",*iflags,nvp_iflags_str(*iflags));

	DBGRET_SUCCESS(MOD_REQ);
}

