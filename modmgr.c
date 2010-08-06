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
#include <ctype.h>


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
_nvp_fill(nvpair_t nvp,char *name_ptr,uint16_t name_size,void *value_ptr,uint16_t value_size)
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
			continue;

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

	for( int i = 0 ; i < REQIDSIZE ; i++ ) {
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
	char end_char;
	int state;
	uint16_t reqnvcount;
	int space_count;

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
	space_count = 0;
	while( *aux ) {
		if( (*aux == ' ') && (space_count++ == 3) )
			goto at_nvpair;
		aux++;
	}

	dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to process request data",req);
	goto return_fail;

at_nvpair:
	aux++;

	dbgprint(MOD_MODMGR,__func__,"(req=%p) found nvpair start at +%d",req,(int)(aux - (char*)&req->data.text.raw));

	reqnvcount = 0;
	state = 0;

	while( *aux )
	{
		switch(state)
		{
			case 0:
				if( !isalnum(*aux) )
				{
					dbgprint(MOD_MODMGR,__func__,"(req=%p) detected non-alnum char at +%d",req,(int)(aux - (char*)&req->data.text.raw));

					if( *aux == '=' ) {
						dbgprint(MOD_MODMGR,__func__,"(req=%p) nvp[%u] has a value associated",req,reqnvcount+1);
						aux++;
						if( *aux == '"' ) {
							dbgprint(MOD_MODMGR,__func__,"(req=%p) nvp[%u] has end char defined by 0x22",req,reqnvcount+1);
							end_char = '"';
						} else
						{
							dbgprint(MOD_MODMGR,__func__,"(req=%p) nvp[%u] has end char defined by 0x20",req,reqnvcount+1);
							end_char = ' ';
						}
						state = 1;
						break;
					} else if( *aux == ' ' ) {
						dbgprint(MOD_MODMGR,__func__,"(req=%p) nvp[%u] has no value associated",req,reqnvcount+1);
						reqnvcount++;
						dbgprint(MOD_MODMGR,__func__,"(req=%p) incremented nvcount to %u",reqnvcount);
					} else
					{
						dbgprint(MOD_MODMGR,__func__,"(req=%p) invalid or unexpected char detected at +%d",req,(int)(aux - (char*)&req->data.text.raw));
						goto return_fail;
					}
				}
				break;
			case 1: /* parse value until end */
				if( *aux == end_char )
				{
					dbgprint(MOD_MODMGR,__func__,"(req=%p) nvp[%u] found end char, parsing of value terminated",req,reqnvcount+1);
					
					reqnvcount++;
					dbgprint(MOD_MODMGR,__func__,"(req=%p) incremented nvcount to %u",reqnvcount);
					
					/* skip space */
					aux++;
					if( *aux == ' ' ) {
						state = 0;
						break;
					} else
					{
						dbgprint(MOD_MODMGR,__func__,"(req=%p) nvp[%u] its always expected a 0x20 after the nvp",req,reqnvcount);
						goto return_fail;
					}
				}
				break;
		}
		aux++;			
	}

	dbgprint(MOD_MODMGR,__func__,"(req=%p) parsing is finished, detected %u nvpairs",req,reqnvcount);
	*nvcount = reqnvcount;
	dbgprint(MOD_MODMGR,__func__,"(req=%p) updated nvcount argument to new value %u",*nvcount);

	DBGRET_SUCCESS(MOD_MODMGR);
return_fail:
	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _req_text_nv_index

   Helper function to get request name-value with specific index.
   The index must be a number from 0 to req_text_nv_count - 1.
   Client code is responsible from freeing nvp data structure.
*/
wstatus
_req_text_nv_index(request_t req,int nv_index,nvpair_t *nvp)
{
	char *aux;
	char end_char;
	int state;
	uint16_t reqnv_idx;
	int space_count;
	char *name_ptr = 0,*name_buf = 0;
	char *value_ptr = 0,*value_buf = 0;
	uint16_t name_size = 0, value_size = 0;
	char *name_end = 0;
	nvpair_t new_nvpair = 0;
	wstatus ws;

	dbgprint(MOD_MODMGR,__func__,"called with req=%p, nvp=%p",req,nvp);

	if( !req ) {
		dbgprint(MOD_MODMGR,__func__,"invalid req argument (req=0)");
		goto return_fail;
	}

	if( !nvp ) {
		dbgprint(MOD_MODMGR,__func__,"invalid nvp argument (nvp=0)");
		goto return_fail;
	}

	if( req->stype != REQUEST_STYPE_TEXT ) {
		dbgprint(MOD_MODMGR,__func__,"invalid request to be used with this function");
		goto return_fail;
	}

	dbgprint(MOD_MODMGR,__func__,"(req=%p) looking up for nvpair start",req);

	aux = (char*)&req->data.text.raw;
	space_count = 0;
	while( *aux ) {
		if( (*aux == ' ') && (space_count++ == 3) )
			goto at_nvpair;
		aux++;
	}

	dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to process request data",req);
	goto return_fail;

at_nvpair:
	aux++;

	/* aux should now point to the first nvpair name string */
	dbgprint(MOD_MODMGR,__func__,"(req=%p) found nvpair start at +%d",req,(int)(aux - (char*)&req->data.text.raw));

	/* initialize some local variables */
	reqnv_idx = 1;
	state = 0;

	/* start loop that will parse all nvpairs until it reaches the end of the request
	 defined by a null char. */
	while( *aux )
	{
		switch(state)
		{
			case 0:

				name_ptr = aux;

				/* find the size of the name string, it ends at '=' or ' ' char. */
				name_end = strstr(name_ptr,"= ");
				if( !name_end ) {
					dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to find end of name for nvpair(%u)",req,reqnv_idx);
					goto return_fail;
				}

				/* calculate name size based on pointers to start and end */
				name_size = (unsigned int) (name_end - name_ptr);
				dbgprint(MOD_MODMGR,__func__,"(req=%p) found end of name for nvpair(%u), length=%u",req,reqnv_idx,name_size);

				if( reqnv_idx == nv_index )
			   	{
					dbgprint(MOD_MODMGR,__func__,"(req=%p) nvp[%u] is the wanted nvpair, keeping name information",req,reqnv_idx);

					/* now that we've the name length we can allocate a char buffer to
					   old the name from the nvpair */
					name_buf = (char*)malloc(name_size*sizeof(char) + 1);
					if( !name_buf ) {
						dbgprint(MOD_MODMGR,__func__,"(req=%p) malloc failed",req);
						goto return_fail;
					}
					dbgprint(MOD_MODMGR,__func__,"(req=%p) nvp[%u] allocated %u bytes for name",name_size*sizeof(char) + 1);

					/* copy name to name_buf, allocated memory */
					dbgprint(MOD_MODMGR,__func__,"(req=%p) copying name to name_buf (length=%u)",req,name_size);
					memcpy(name_buf,name_ptr,name_size);

					/* insert the null char also */
					dbgprint(MOD_MODMGR,__func__,"(req=%p) inserting null char to name_buf (at %p)",req,name_buf+name_size);
					name_buf[name_size] = '\0';
				}

				/* skip the name string chars and continue processing */
				aux += name_size;

				if( *aux == ' ' ) {
					/* means this nvpair doesn't have any value associated */
					goto nvpair_ok;
				}

				if( *aux != '=' ) {
					dbgprint(MOD_MODMGR,__func__,"(req=%p) invalid or unexpected char detected at +%d",req,(int)(aux - (char*)&req->data.text.raw));
					goto return_fail;
				}
				
				/* process nvpair value */
				aux++;

				if( *aux == '"' ) {
					dbgprint(MOD_MODMGR,__func__,"(req=%p) nvp[%u] has end char defined by 0x22",req,reqnv_idx);
					end_char = '"';
					aux++;
				} else
				{
					dbgprint(MOD_MODMGR,__func__,"(req=%p) nvp[%u] has end char defined by 0x20",req,reqnv_idx);
					end_char = ' ';
				}

				state = 1;
				value_ptr = aux;
				break;
			case 1: /* parse value until end */
				if( *aux == end_char )
				{
					dbgprint(MOD_MODMGR,__func__,"(req=%p) nvp[%u] found end char, parsing of value terminated",req,reqnv_idx);
					
					/* if this is the wanted nvpair, allocate buffer for the value */
					if( reqnv_idx == nv_index )
					{
						dbgprint(MOD_MODMGR,__func__,"(req=%p) nvp[%u] is the wanted nvpair, keeping name information",req,reqnv_idx);

						if( (unsigned int)(aux - value_ptr) > 65535 ) {
							dbgprint(MOD_MODMGR,__func__,"(req=%p) nvp[%u] value string is bigger than value_size field (65535)",req,reqnv_idx);
							goto return_fail;
						}

						value_size = (uint16_t)(aux - value_ptr);
						if( !value_size ) {
							dbgprint(MOD_MODMGR,__func__,"(req=%p) nvp[%u] value is empty string, resuming",req,reqnv_idx);
							goto nvpair_ok;
						}

						/* now that we've the name length we can allocate a char buffer to
						   old the value from the nvpair */
						value_buf = (char*)malloc(value_size*sizeof(char) + 1);
						if( !value_buf ) {
							dbgprint(MOD_MODMGR,__func__,"(req=%p) malloc failed",req);
							goto return_fail;
						}
						dbgprint(MOD_MODMGR,__func__,"(req=%p) nvp[%u] allocated %u bytes for value",value_size*sizeof(char) + 1);

						/* copy value to value_buf, allocated memory */
						dbgprint(MOD_MODMGR,__func__,"(req=%p) copying value to value_buf (length=%u)",req,value_size);
						memcpy(value_buf,value_ptr,value_size);

						/* insert the null char also */
						dbgprint(MOD_MODMGR,__func__,"(req=%p) inserting null char to value_buf (at %p)",req,value_buf+value_size);
						value_buf[value_size] = '\0';

						goto nvpair_ok;
					}

					reqnv_idx++;

					dbgprint(MOD_MODMGR,__func__,"(req=%p) incremented nvidx to %u",reqnv_idx);
					
					/* skip space */
					aux++;
					if( *aux == ' ' ) {
						state = 0;
						break;
					} else
					{
						dbgprint(MOD_MODMGR,__func__,"(req=%p) nvp[%u] its always expected a 0x20 after the nvp",req,reqnv_idx);
						goto return_fail;
					}
				}
				break;
		}
		aux++;			
	}

	dbgprint(MOD_MODMGR,__func__,"(req=%p) parsing is finished, detected %u nvpairs",req,reqnv_idx);
	dbgprint(MOD_MODMGR,__func__,"(req=%p) parsing is finished but the wanted nvpair was not found",req);
	goto return_fail;

nvpair_ok:

	dbgprint(MOD_MODMGR,__func__,"(req=%p) finished parsing wanted nvpair, building nvpair data structure");

	/* allocate new nvpair data structure */
	ws = _nvp_alloc(name_size,value_size,&new_nvpair);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to allocate new nvpair data structure",req);
		goto return_fail;
	}

	dbgprint(MOD_MODMGR,__func__,"(req=%p) new nvpair data structure allocated (new_nvpair=%p), filling with name and value",req,new_nvpair);
	ws = _nvp_fill(new_nvpair,name_ptr,name_size,value_ptr,value_size);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) unable to fill new nvpair data structure",req);
		goto return_fail;
	}

	*nvp = new_nvpair;
	dbgprint(MOD_MODMGR,__func__,"(req=%p) updated nvp argument to new value %p",*nvp);

	/* free used buffers, not needed anymore */
	dbgprint(MOD_MODMGR,__func__,"(req=%p) freeing name buffer (%p)",req,name_buf);
	free(name_buf);

	/* value buffer is optional */
	if( value_buf ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) freeing value buffer (%p)",req,value_buf);
		free(value_buf);
	}

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:

	if( name_buf ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) freeing name buffer (%p)",req,name_buf);
		free(name_buf);
	}

	if( value_buf ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) freeing value buffer (%p)",req,value_buf);
		free(value_buf);
	}

	if( new_nvpair ) {
		dbgprint(MOD_MODMGR,__func__,"(req=%p) destroying new_nvpair data structure (%p)",req,new_nvpair);
		_nvp_free(new_nvpair);
	}

	DBGRET_FAILURE(MOD_MODMGR);
}

/*
   _req_from_text_to_pipe

   Helper function to convert a request data structure from text type to pipe type.
*/
wstatus
_req_from_text_to_pipe(request_t req,request_t *req_pipe)
{
	dbgprint(MOD_MODMGR,__func__,"called with req=%p, req_pipe=%p",
			req,req_pipe);

	DBGRET_SUCCESS(MOD_MODMGR);
}

wstatus
_req_to_pipe(request_t req,request_t *req_pipe)
{
	wstatus ws;
	dbgprint(MOD_MODMGR,__func__,"called with req=%p, req_pipe=%p",req,req_pipe);

	switch(req->stype)
	{
		case REQUEST_STYPE_BIN:
			dbgprint(MOD_MODMGR,__func__,"(req=%p) calling helper function _req_from_bin_to_pipe",req);
			ws = _req_from_bin_to_pipe(req,req_pipe);
			dbgprint(MOD_MODMGR,__func__,"(req=%p) helper function _req_from_bin_to_pipe returned ws=%d",req,ws);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_MODMGR,__func__,"(req=%p) helper function failed, aborting",req);
				goto return_fail;
			}
			break;
		case REQUEST_STYPE_TEXT:
			dbgprint(MOD_MODMGR,__func__,"(req=%p) calling helper function _req_from_text_to_pipe",req);
			ws = _req_from_text_to_pipe(req,req_pipe);
			dbgprint(MOD_MODMGR,__func__,"(req=%p) helper function _req_from_text_to_pipe returned ws=%d",req,ws);
			if( ws != WSTATUS_SUCCESS ) {
				dbgprint(MOD_MODMGR,__func__,"(req=%p) helper function failed, aborting",req);
				goto return_fail;
			}
			break;
		case REQUEST_STYPE_PIPE:
			dbgprint(MOD_MODMGR,__func__,"(req=%p) request is already in pipe data structure");
			goto return_fail;
		default:
			dbgprint(MOD_MODMGR,__func__,"(req=%p) request has an invalid or unsupported stype (check req pointer...)",req);
			goto return_fail;
	}

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:
	DBGRET_FAILURE(MOD_MODMGR);
}

