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

#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "wstatus.h"
#include "debug.h"
#include "modmgr.h"
#include "jmlist.h"
#include "wchannel.h"
#include "wthread.h"
#include "nvpair.h"
#include "req.h"
#include "reqbuf.h"

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

void _modmgr_reqproc_cb(const request_t req);
wstatus _modreg_alloc(modreg_t *new_mod);
wstatus _modreg_free(const struct _modreg_t *mod);

/* this module variables */
static jmlist mod_list = 0; /* modreg_t */
static request_proc_data_t thread_reqproc_data;
static bool unloading = false;
static bool loaded = false;

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
		ws = req_add_nvp_z(REQERROR_DESCNAME,error_description,aux_req);
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
		req_free(aux_req);

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
	req_free(reply);

	if( mod_src )
		_modreg_free(mod_src);

	if( mod_dst )
		_modreg_free(mod_dst);

	DBGRET_SUCCESS(MOD_MODMGR);

return_fail:
	if( reply )
		req_free(reply);

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
   modreg_create_from_request

   Helper function that converts a request with destination 'modmgr' and
   code 'registryModule' into a module registration data structure.
*/
wstatus
modreg_create_from_request(request_t req,modreg_t *modreg)
{
	return WSTATUS_UNIMPLEMENTED;
}

wstatus
_modreg_validate_name(const char *name_ptr,unsigned int name_size,modreg_validation_result *result)
{
	unsigned int i;

	dbgprint(MOD_MODMGR,__func__,"called with name_ptr=%p, name_size=%u, result=%p",
			name_ptr,name_size,result);

	if( !name_ptr ) {
		dbgprint(MOD_MODMGR,__func__,"invalid name_ptr argument (name_ptr=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( !name_size ) {
		dbgprint(MOD_MODMGR,__func__,"invalid name_size argument (name_size=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( !result ) {
		dbgprint(MOD_MODMGR,__func__,"invalid result argument (result=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	for( i = 0 ; i < name_size ; i++ )
   	{
		if( MRV_NAMECHAR(name_ptr[i]) )
			continue;

		dbgprint(MOD_MODMGR,__func__,"invalid/unsupported character found in module name (name[%u]=%02X)",i,name_ptr[i]);
		*result = MODREG_PARAM_INVALID_CHARACTER_FOUND;
		dbgprint(MOD_MODMGR,__func__,"updated result value to %s",modreg_val_str(*result));
		DBGRET_SUCCESS(MOD_MODMGR);
	}

	dbgprint(MOD_MODMGR,__func__,"finished parsing all name characters");

	/* all characters are valid */
	
	*result = MODREG_PARAM_CHARACTERS_ARE_VALID;
	dbgprint(MOD_MODMGR,__func__,"updated result value to %s",modreg_val_str(*result));
	DBGRET_SUCCESS(MOD_MODMGR);
}

wstatus
_modreg_validate_description(const char *description_ptr,unsigned int description_size,modreg_validation_result *result)
{
	unsigned int i;

	dbgprint(MOD_MODMGR,__func__,"called with description_ptr=%p, description_size=%u, result=%p",
			description_ptr,description_size,result);

	if( !description_ptr ) {
		dbgprint(MOD_MODMGR,__func__,"invalid description_ptr argument (description_ptr=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( !description_size ) {
		dbgprint(MOD_MODMGR,__func__,"invalid description_size argument (description_size=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( !result ) {
		dbgprint(MOD_MODMGR,__func__,"invalid result argument (result=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	for( i = 0 ; i < description_size ; i++ )
   	{
		if( MRV_DESCCHAR(description_ptr[i]) )
			continue;

		dbgprint(MOD_MODMGR,__func__,"invalid/unsupported character found in module description (name[%u]=%02X)",i,description_ptr[i]);
		*result = MODREG_PARAM_INVALID_CHARACTER_FOUND;
		dbgprint(MOD_MODMGR,__func__,"updated result value to %s",modreg_val_str(*result));
		DBGRET_SUCCESS(MOD_MODMGR);
	}

	dbgprint(MOD_MODMGR,__func__,"finished parsing all description characters");

	/* all characters are valid */
	
	*result = MODREG_PARAM_CHARACTERS_ARE_VALID;
	dbgprint(MOD_MODMGR,__func__,"updated result value to %s",modreg_val_str(*result));
	DBGRET_SUCCESS(MOD_MODMGR);
}

wstatus
_modreg_validate_author_name(const char *author_name_ptr,unsigned int author_name_size,modreg_validation_result *result)
{
	unsigned int i;

	dbgprint(MOD_MODMGR,__func__,"called with author_name_ptr=%p, author_name_size=%u, result=%p",
			author_name_ptr,author_name_size,result);

	if( !author_name_ptr ) {
		dbgprint(MOD_MODMGR,__func__,"invalid author_name_ptr argument (author_name_ptr=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( !author_name_size ) {
		dbgprint(MOD_MODMGR,__func__,"invalid author_name_size argument (author_name_size=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( !result ) {
		dbgprint(MOD_MODMGR,__func__,"invalid result argument (result=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	for( i = 0 ; i < author_name_size ; i++ )
   	{
		if( MRV_AUTHORNCHAR(author_name_ptr[i]) )
			continue;

		dbgprint(MOD_MODMGR,__func__,"invalid/unsupported character found in module author name (author_name[%u]=%02X)",i,author_name_ptr[i]);
		*result = MODREG_PARAM_INVALID_CHARACTER_FOUND;
		dbgprint(MOD_MODMGR,__func__,"updated result value to %s",modreg_val_str(*result));
		DBGRET_SUCCESS(MOD_MODMGR);
	}

	dbgprint(MOD_MODMGR,__func__,"finished parsing all author name characters");

	/* all characters are valid */
	
	*result = MODREG_PARAM_CHARACTERS_ARE_VALID;
	dbgprint(MOD_MODMGR,__func__,"updated result value to %s",modreg_val_str(*result));
	DBGRET_SUCCESS(MOD_MODMGR);
}

wstatus
_modreg_validate_author_email(const char *author_email_ptr,unsigned int author_email_size,modreg_validation_result *result)
{
	unsigned int i;

	dbgprint(MOD_MODMGR,__func__,"called with author_email_ptr=%p, author_email_size=%u, result=%p",
			author_email_ptr,author_email_size,result);

	if( !author_email_ptr ) {
		dbgprint(MOD_MODMGR,__func__,"invalid author_email_ptr argument (author_email_ptr=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( !author_email_size ) {
		dbgprint(MOD_MODMGR,__func__,"invalid author_email_size argument (author_email_size=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	if( !result ) {
		dbgprint(MOD_MODMGR,__func__,"invalid result argument (result=0)");
		DBGRET_FAILURE(MOD_MODMGR);
	}

	for( i = 0 ; i < author_email_size ; i++ )
   	{
		if( MRV_AUTHORECHAR(author_email_ptr[i]) )
			continue;

		dbgprint(MOD_MODMGR,__func__,"invalid/unsupported character found in module author email (author_email[%u]=%02X)",i,author_email_ptr[i]);
		*result = MODREG_PARAM_INVALID_CHARACTER_FOUND;
		dbgprint(MOD_MODMGR,__func__,"updated result value to %s",modreg_val_str(*result));
		DBGRET_SUCCESS(MOD_MODMGR);
	}

	dbgprint(MOD_MODMGR,__func__,"finished parsing all author email characters");

	/* all characters are valid */
	
	*result = MODREG_PARAM_CHARACTERS_ARE_VALID;
	dbgprint(MOD_MODMGR,__func__,"updated result value to %s",modreg_val_str(*result));
	DBGRET_SUCCESS(MOD_MODMGR);
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
	req_dump(req);
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

