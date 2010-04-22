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

	Copyright (C) 2009 Jean Mousinho <jean.mousinho@ist.utl.pt>
	Centro de Informatica do IST - Universidade Tecnica de Lisboa 
*/

#include <stdbool.h>
#include <string.h>
#ifndef sleep
#include <unistd.h>
#endif
#include <stdlib.h>

#include "posh.h"
#include "wstatus.h"
#include "debug.h"
#include "wviewcb.h"
#include "wviewctl.h"
#include "wview.h"
#include "wthread.h"
#include "wlock.h"
#include "jmlist.h"

/* define the callback lists and their associated synchronization object */

static jmlist keyboard_cbl;
static wlock_t keyboard_cbl_lock;
static jmlist mouse_cbl;
static wlock_t mouse_cbl_lock;
static jmlist draw_cbl;
static wlock_t draw_cbl_lock;

static bool wvctl_loaded = false;
static bool wvctl_unloading = false;
static bool wvctl_closed = false;
static bool wvctl_redraw_flag = false;

static wvctl_exit_routine_cb exit_routine = 0;
static wthread_t worker_thread;

/* structure that is temporarly used to store thread initialization status */

typedef struct _thread_init_status {
	bool init_finished_flag;
	wstatus init_finished_status;
} thread_init_status;

/* some internal functions used by this module */

static wstatus wvctl_free_keyboard_cbl(void);
static wstatus wvctl_free_keyboard_cbl_lock(void);
static wstatus wvctl_free_mouse_cbl(void);
static wstatus wvctl_free_mouse_cbl_lock(void);
static wstatus wvctl_free_draw_cbl(void);
static wstatus wvctl_free_draw_cbl_lock(void);
static void wvctl_worker_routine(void *param);
static wstatus wvctl_keyboard_routine(wvkey_t key,wvkey_mode_t key_mode,void *param);
static wstatus wvctl_mouse_routine(wvmouse_t mouse,void *param);
static wstatus wvctl_draw_routine(wvdraw_t draw,void *param);
static wstatus wvctl_closewnd_routine(bool *ignore);

static jmlist_status iwvctl_find_keycb(void *ptr,void *param,jmlist_lookup_result *result);
static jmlist_status iwvctl_find_mousecb(void *ptr,void *param,jmlist_lookup_result *result);
static jmlist_status iwvctl_find_drawcb(void *ptr,void *param,jmlist_lookup_result *result);

/*
   wvctl_free_keyboard_cb

   Internal helper function to free a single keyboard callback structure.
*/
wstatus
wvctl_free_keyboard_cb(wvkeyboardcb_t *keycb)
{
	dbgprint(MOD_WVIEWCTL,__func__,"called with keycb=%p",keycb);

	dbgprint(MOD_WVIEWCTL,__func__,"freeing keycb=%p\n",keycb);
	free(keycb);
	dbgprint(MOD_WVIEWCTL,__func__,"keyboard callback with keycb=%p was freed successfully",keycb);

	DBGRET_SUCCESS(MOD_WVIEWCTL);
}

/*
   wvctl_free_keyboard_cbl

   Internal helper function to free keyboard callback list and its lock. This function
   doesn't acquire the keyboard callback list lock, the caller is responsible for that.
*/
wstatus
wvctl_free_keyboard_cbl(void)
{
	jmlist_status jmls;
	wstatus ws;
	dbgprint(MOD_WVIEWCTL,__func__,"called");

	dbgprint(MOD_WVIEWCTL,__func__,"freeing keyboard callback list");
	
	jmlist_index cbcount;
	jmls = jmlist_entry_count(keyboard_cbl,&cbcount);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		/* something went wrong with getting entry count! */
		dbgprint(MOD_WVIEWCTL,__func__,"unable to get number of callback entries in keyboard callback list");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	if( cbcount == 0 ) {
//		dbgprint(MOD_WVIEWCTL,__func__,"cannot free keyboard callback list because its empty");
//		DBGRET_FAILURE(MOD_WVIEWCTL);
		dbgprint(MOD_WVIEWCTL,__func__,"list of callbacks is empty");
		DBGRET_SUCCESS(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"freeing %u callbacks from keyboard callback list",cbcount);

	wvkeyboardcb_t *keycbt_ptr;
	jmlist_index cbcount_org = cbcount-1;
	while( cbcount-- )
	{
		jmls = jmlist_pop(keyboard_cbl,(void*)&keycbt_ptr);
	
		if( jmls != JMLIST_ERROR_SUCCESS )
	   	{
			dbgprint(MOD_WVIEWCTL,__func__,"unable to pop keyboard list entry");
			if( cbcount_org != cbcount )
				dbgprint(MOD_WVIEWCTL,__func__,"warning!! keyboard callback list was partially freed");
			DBGRET_FAILURE(MOD_WVIEWCTL);
		}
	
		dbgprint(MOD_WVIEWCTL,__func__,"calling wvctl_free_keyboard_cb(keycbt=%p)",keycbt_ptr);
		ws = wvctl_free_keyboard_cb(keycbt_ptr);
		dbgprint(MOD_WVIEWCTL,__func__,"wvctl_free_keyboard_cb returned with ws=%u",ws);
	
		if( ws != WSTATUS_SUCCESS ) 
		{
			dbgprint(MOD_WVIEWCTL,__func__,"wvctl_free_keyboard_cb failed");
			if( cbcount_org != cbcount )
				dbgprint(MOD_WVIEWCTL,__func__,"warning!! keyboard callback list was partially freed");
			DBGRET_FAILURE(MOD_WVIEWCTL);
		}

		/* continue to the next callback entry.. */
	}

	/* check if the entry count is zero now. */
	jmls = jmlist_entry_count(keyboard_cbl,&cbcount);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		/* something went wrong with getting entry count! */
		dbgprint(MOD_WVIEWCTL,__func__,"unable to get number of callback entries in keyboard callback list (after free loop)");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	/* still have entries... means some other thread is adding entries to the list
	and the code is broken because its not verifying that we're unloading the module! */
	if( cbcount ) {
		dbgprint(MOD_WVIEWCTL,__func__,"after free loop there are still entries in the list, is *anyone* adding entries to the list?");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	/* all entries removed successfully. */
	dbgprint(MOD_WVIEWCTL,__func__,"all keyboard callback list entries were freed successfully");
	DBGRET_SUCCESS(MOD_WVIEWCTL);
}

/*
   wvctl_free_mouse_cbl

   Internal helper function to free mouse callback list.
*/
wstatus
wvctl_free_mouse_cbl(void)
{
	jmlist_status jmls;
	dbgprint(MOD_WVIEWCTL,__func__,"called");

	dbgprint(MOD_WVIEWCTL,__func__,"freeing mouse callback list");

	jmls = jmlist_free(mouse_cbl);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		/* something went wrong with the free, program should abort ASAP.. */
		dbgprint(MOD_WVIEWCTL,__func__,"unable to free mouse callback list");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	DBGRET_SUCCESS(MOD_WVIEWCTL);
}

/*
   wvctl_free_keyboard_cbl_lock

   Internal helper function to free keyboard callback list lock.
*/
wstatus
wvctl_free_keyboard_cbl_lock(void)
{
	dbgprint(MOD_WVIEWCTL,__func__,"called");

	dbgprint(MOD_WVIEWCTL,__func__,"freeing keyboard callback list lock");

	if( wlock_free(&keyboard_cbl_lock) != WSTATUS_SUCCESS ) {
		/* something went wrong with the free, program should abort ASAP.. */
		dbgprint(MOD_WVIEWCTL,__func__,"unable to free keyboard callback list lock");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"keyboard callback list lock was freed successfully");
	DBGRET_SUCCESS(MOD_WVIEWCTL);
}

/*
   wvctl_free_mouse_cbl_lock

   Internal helper function to free mouse callback list lock.
*/
wstatus
wvctl_free_mouse_cbl_lock(void)
{
	dbgprint(MOD_WVIEWCTL,__func__,"called");

	dbgprint(MOD_WVIEWCTL,__func__,"freeing mouse callback list lock");
	if( wlock_free(&mouse_cbl_lock) != WSTATUS_SUCCESS ) {
		/* something went wrong with the free, program should abort ASAP.. */
		dbgprint(MOD_WVIEWCTL,__func__,"unable to free mouse callback list lock");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	DBGRET_SUCCESS(MOD_WVIEWCTL);
}

/*
   wvctl_load

   This function loads the module, that will create the worker thread
   which will play with the wview interface. This module depends on
   configuration module, since it does have configuration parameters
   like the window size and position. Create/initialize the callback
   lists and their associated synchronization objects.
*/
wstatus
wvctl_load(wvctl_load_t load)
{
	struct _jmlist_params jmlp;
	jmlist_status jmls;
	wstatus ws;

	dbgprint(MOD_WVIEWCTL,__func__,"called with load.exit_routine = %p",load.exit_routine);

	if( wvctl_loaded ) {
		dbgprint(MOD_WVIEWCTL,__func__,"module was already loaded");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	jmlp.flags = JMLIST_LINKED;
	dbgprint(MOD_WVIEWCTL,__func__,"creating jmlist for keyboard callback list");
	jmls = jmlist_create(&keyboard_cbl,&jmlp);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		/* something went wrong with list creation */
		dbgprint(MOD_WVIEWCTL,__func__,"failed to create jmlist (status=%d)",jmls);
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"created jmlist for keyboard callback list successfully (jml=%p)",keyboard_cbl);
	dbgprint(MOD_WVIEWCTL,__func__,"creating wlock for keyboard callback list");

	/* create lock for the list */
	ws = wlock_create(&keyboard_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"wlock creation failed for keyboard callback list");

		/* if this has failed, we should free the rest of the objects that were created
		 since the module will have unloaded state. */
		wvctl_free_keyboard_cbl();
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"created lock for keyboard callback list successfully");

	jmlp.flags = JMLIST_LINKED;
	dbgprint(MOD_WVIEWCTL,__func__,"creating jmlist for mouse callback list");
	jmls = jmlist_create(&mouse_cbl,&jmlp);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"failed to create jmlist (status=%d)",jmls);
		
		/* if this has failed, we should free the rest of the objects that were created
		 since the module will have unloaded state. */
		wvctl_free_keyboard_cbl();
		wvctl_free_keyboard_cbl_lock();
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"created jmlist for mouse callback list successfully (jml=%p)",mouse_cbl);
	dbgprint(MOD_WVIEWCTL,__func__,"creating wlock for mouse callback list");

	ws = wlock_create(&mouse_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"wlock creation failed for mouse callback list");

		/* if this has failed, we should free the rest of the objects that were created
		 since the module will have unloaded state. */
		wvctl_free_keyboard_cbl();
		wvctl_free_keyboard_cbl_lock();
		wvctl_free_mouse_cbl();
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	jmlp.flags = JMLIST_LINKED;
	dbgprint(MOD_WVIEWCTL,__func__,"creating jmlist for draw callback list");
	jmls = jmlist_create(&draw_cbl,&jmlp);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"failed to create jmlist (status=%d)",jmls);
		
		/* if this has failed, we should free the rest of the objects that were created
		 since the module will have unloaded state. */
		wvctl_free_keyboard_cbl();
		wvctl_free_keyboard_cbl_lock();
		wvctl_free_mouse_cbl();
		wvctl_free_mouse_cbl_lock();
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"created jmlist for draw callback list successfully (jml=%p)",mouse_cbl);
	dbgprint(MOD_WVIEWCTL,__func__,"creating wlock for draw callback list");

	ws = wlock_create(&draw_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"wlock creation failed for draw callback list");

		/* if this has failed, we should free the rest of the objects that were created
		 since the module will have unloaded state. */
		wvctl_free_keyboard_cbl();
		wvctl_free_keyboard_cbl_lock();
		wvctl_free_mouse_cbl();
		wvctl_free_mouse_cbl_lock();
		wvctl_free_draw_cbl();
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	/* if the load structure is not initialized, it might cause a crash when unloading
	   this module... */
	dbgprint(MOD_WVIEWCTL,__func__,"setting up the exit routine to %p",load.exit_routine);
	exit_routine = load.exit_routine;

	/* Now that the objects are created, create the working thread, wait for it to initialize
	and then return success or failure depending on thread's initialization return status.
	A small hack is used to wait for thread initialization, we loop until a flag is set. */
	thread_init_status tis;
	tis.init_finished_flag = false;
	tis.init_finished_status = 0;
	ws = wthread_create(&wvctl_worker_routine,&tis,&worker_thread);

	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"failed to create working thread");
free_all:
		/* if this has failed, we should free the rest of the objects that were created
		 since the module will have unloaded state. */
		wvctl_free_keyboard_cbl();
		wvctl_free_keyboard_cbl_lock();
		wvctl_free_mouse_cbl();
		wvctl_free_mouse_cbl_lock();
		wvctl_free_draw_cbl();
		wvctl_free_draw_cbl_lock();
		exit_routine = 0; /* cleanup the exit routine */
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"working thread was created, waiting for its initialization");

	/* wait for the working thread to initialize */
	while( !tis.init_finished_flag ) sleep(0);

	if( tis.init_finished_status != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"working thread failed to initialize");
		goto free_all;
	}

	dbgprint(MOD_WVIEWCTL,__func__,"working thread initialized successfully");

	dbgprint(MOD_WVIEWCTL,__func__,"changing module state to 'loaded'");
	wvctl_loaded = true;

	dbgprint(MOD_WVIEWCTL,__func__,"reseting unloading flag to false");
	wvctl_unloading = false;

	/* all went OK */
	DBGRET_SUCCESS(MOD_WVIEWCTL);
}

/*
   wvctl_worker_routine

   This is the routine of the thread created in the wviewctl module initialization,
   it is the thread that will contact with the wview interface. This thread initializes
   wview interface module, registers the wviewctl callbacks (one for keyboard, another
   for mouse, etc.) and then finishes by calling the main loop function that will only
   return when the window of wview is closed or destroyed.
*/
void
wvctl_worker_routine(void *param)
{
	thread_init_status *tis;
	wstatus ws;

	dbgprint(MOD_WVIEWCTL,__func__,"called with param=%p",param);

	tis = (thread_init_status*)param;

	/* initialize wview */
	wview_load_t wvl;
	ws = wview_load(wvl);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"unable to load wview module");
		dbgprint(MOD_WVIEWCTL,__func__,"filling thread_init_status with failure code");
		tis->init_finished_status = WSTATUS_FAILURE;
		tis->init_finished_flag = true;
		dbgprint(MOD_WVIEWCTL,__func__,"Returning with no status.");
		return;
	}

	/* for now I'm going to use constants, when cfgmgr is done I'll use it
	   to get some configuration parameters for this module. */

	wview_window_t window;
	window.width = WINDOW_WIDTH;
	window.height = WINDOW_HEIGHT;
	window.keyboard_routine = wvctl_keyboard_routine;
	window.mouse_routine = wvctl_mouse_routine;
	window.draw_routine = wvctl_draw_routine;
	window.closewnd_routine = wvctl_closewnd_routine;
	ws = wview_create_window(window);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"unable to create window for wview");
		dbgprint(MOD_WVIEWCTL,__func__,"filling thread_init_status with failure code");
		tis->init_finished_status = WSTATUS_FAILURE;
		tis->init_finished_flag = true;
		dbgprint(MOD_WVIEWCTL,__func__,"Returning with no status.");
		return;
	}

	/* initialization went OK, pass that information.. */
	dbgprint(MOD_WVIEWCTL,__func__,"filling thread_init_status with success code");
	tis->init_finished_status = WSTATUS_SUCCESS;
	tis->init_finished_flag = true;

	/* main loop */
	while( !wvctl_closed )
	{
		/* window was created successfully, pass the CPU to the main loop */
		ws = wview_process(WVIEW_ASYNCHRONOUS);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_WVIEWCTL,__func__,"wview_process failed");
			dbgprint(MOD_WVIEWCTL,__func__,"Returning with no status.");
			return;
		}

		sleep(0.5);

		if( wvctl_redraw_flag ) {
			wview_redraw();
			wvctl_redraw_flag = false;
		}
	}

	/* call client exit routine if any was set */
	if( exit_routine ) {
		/* return status of exit routine is ignored by now. */
		dbgprint(MOD_WVIEWCTL,__func__,"passing CPU to the exit routine...");
		exit_routine(WVCTL_WINDOW_CLOSED);
		dbgprint(MOD_WVIEWCTL,__func__,"exit routine returned");
	} else
	{
		dbgprint(MOD_WVIEWCTL,__func__,"exit routine was not set, so this will not call any exit routine");
	}

	/* user has closed the window, cleanup and leave with success. */
//	dbgprint(MOD_WVIEWCTL,__func__,"destroying the wview window");
//	ws = wview_destroy_window();
//	if( ws != WSTATUS_SUCCESS ) {
//		dbgprint(MOD_WVIEWCTL,__func__,"unable to destroy wview window");
//		dbgprint(MOD_WVIEWCTL,__func__,"Returning with no status.");
//		return;
//	}

	ws = wview_process(WVIEW_ASYNCHRONOUS);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"wview_process failed");
		dbgprint(MOD_WVIEWCTL,__func__,"Returning with no status.");
		return;
	}

	dbgprint(MOD_WVIEWCTL,__func__,"Returning with no status.");
	return;
}

/*
   wvctl_keyboard_routine

   Request the keyboard callback list lock, then call all the client callbacks.
*/
wstatus
wvctl_keyboard_routine(wvkey_t key,wvkey_mode_t key_mode,void *param)
{
	wstatus ws;
	jmlist_status jmls;
	wvkeyboardcb_t *keycbt_ptr;

	dbgprint(MOD_WVIEWCTL,__func__,"called with key=%02X and key_mode=%d",key,key_mode);

	if( ! wvctl_loaded ) {
		dbgprint(MOD_WVIEWCTL,__func__,"unexpected call to this function, module not loaded yet");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	if( wvctl_unloading ) {
		dbgprint(MOD_WVIEWCTL,__func__,"module is unloading, ignoring this call");
		DBGRET_SUCCESS(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"acquiring keyboard callback list lock");
	ws = wlock_acquire(&keyboard_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"unable to acquire keyboard callback list lock");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	/* now that we've the lock, parse the keybaord callback list */
	
	jmlist_index entry_count;

	jmls = jmlist_entry_count(keyboard_cbl,&entry_count);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"unable to get keyboard callback list entry count!");
		goto free_lock_fail;
	}

	dbgprint(MOD_WVIEWCTL,__func__,"keyboard callback list has %u entries now",entry_count);
	
	for( jmlist_index i = 0 ; i < entry_count ; i++ )
	{
		jmls = jmlist_get_by_index(keyboard_cbl,i,(void*)&keycbt_ptr);
		if( jmls != JMLIST_ERROR_SUCCESS ) {
			dbgprint(MOD_WVIEWCTL,__func__,"unable to get keyboard callback list entry #%u",i);
			goto free_lock_fail;
		}

		/* call the client routine */
		dbgprint(MOD_WVIEWCTL,__func__,"calling keyboard client routine %p (index %u, param=%p)",
				keycbt_ptr->cb,i,keycbt_ptr->param);
		keycbt_ptr->cb(key,key_mode,keycbt_ptr->param);
		dbgprint(MOD_WVIEWCTL,__func__,"client routine returned");
	}

	dbgprint(MOD_WVIEWCTL,__func__,"all keyboard callbacks called, freeing lock");
	ws = wlock_release(&keyboard_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"failed to release lock");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"keyboard lock released successfully");
	DBGRET_SUCCESS(MOD_WVIEWCTL);

free_lock_fail:
	wlock_release(&keyboard_cbl_lock);
	DBGRET_FAILURE(MOD_WVIEWCTL);
}

wstatus
wvctl_mouse_routine(wvmouse_t mouse,void *param)
{
	wstatus ws;
	jmlist_status jmls;
	wvmousecb_t *mousecbt_ptr;

	dbgprint(MOD_WVIEWCTL,__func__,"called with mouse.button=%d, mouse.coord=(%d,%d) and mouse.mode=%d",
			mouse.button,mouse.x,mouse.y,mouse.mode);

	if( ! wvctl_loaded ) {
		dbgprint(MOD_WVIEWCTL,__func__,"unexpected call to this function, module not loaded yet");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	if( wvctl_unloading ) {
		dbgprint(MOD_WVIEWCTL,__func__,"module is unloading, ignoring this call");
		DBGRET_SUCCESS(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"acquiring mouse callback list lock");
	ws = wlock_acquire(&mouse_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"unable to acquire mouse callback list lock");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	/* now that we've the lock, parse the keybaord callback list */
	
	jmlist_index entry_count;

	jmls = jmlist_entry_count(mouse_cbl,&entry_count);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"unable to get mouse callback list entry count!");
		goto free_lock_fail;
	}

	dbgprint(MOD_WVIEWCTL,__func__,"mouse callback list has %u entries now",entry_count);
	
	for( jmlist_index i = 0 ; i < entry_count ; i++ )
	{
		jmls = jmlist_get_by_index(mouse_cbl,i,(void*)&mousecbt_ptr);
		if( jmls != JMLIST_ERROR_SUCCESS ) {
			dbgprint(MOD_WVIEWCTL,__func__,"unable to get mouse callback list entry #%u",i);
			goto free_lock_fail;
		}

		/* call the client routine */
		dbgprint(MOD_WVIEWCTL,__func__,"calling mouse client routine %p (index %u, param=%p)",
				mousecbt_ptr->cb,i,mousecbt_ptr->param);
		mousecbt_ptr->cb(mouse,mousecbt_ptr->param);
		dbgprint(MOD_WVIEWCTL,__func__,"mouse client routine returned");
	}

	dbgprint(MOD_WVIEWCTL,__func__,"all mouse callbacks called, freeing lock");
	ws = wlock_release(&mouse_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"failed to release mouse callback list lock");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"mouse lock released successfully");
	DBGRET_SUCCESS(MOD_WVIEWCTL);

free_lock_fail:
	wlock_release(&mouse_cbl_lock);
	DBGRET_FAILURE(MOD_WVIEWCTL);
}

/*
   wvctl_unload

   Unloading this module requires to unload wview, to do that it should
   call wview_unload after destroying the window (with wview_destroy_window).

   TODO check that wview_destroy_window can be called from another thread
   than the one being used in the loop?!
*/
wstatus
wvctl_unload(void)
{
	wstatus ws;

	dbgprint(MOD_WVIEWCTL,__func__,"called");

	if( !wvctl_loaded ) {
		dbgprint(MOD_WVIEWCTL,__func__,"Cannot unload module since it was not loaded yet");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	/* set the unloading flag */
	wvctl_unloading = true;
	dbgprint(MOD_WVIEWCTL,__func__,"unloading flag set to TRUE");

	/* destroy window */
	dbgprint(MOD_WVIEWCTL,__func__,"destroying window");
	ws = wview_destroy_window();
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"Unable to destroy window");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	/* free all callbacks registered */
	dbgprint(MOD_WVIEWCTL,__func__,"freeing keyboard callback list");
	wvctl_free_keyboard_cbl();
	dbgprint(MOD_WVIEWCTL,__func__,"freeing keyboard callback list lock");
	wvctl_free_keyboard_cbl_lock();
	dbgprint(MOD_WVIEWCTL,__func__,"freeing mouse callback list");
	wvctl_free_mouse_cbl();
	dbgprint(MOD_WVIEWCTL,__func__,"freeing mouse callback list lock");
	wvctl_free_mouse_cbl_lock();
	dbgprint(MOD_WVIEWCTL,__func__,"freeing draw callback list");
	wvctl_free_draw_cbl();
	dbgprint(MOD_WVIEWCTL,__func__,"freeing draw callback list lock");
	wvctl_free_draw_cbl_lock();

	dbgprint(MOD_WVIEWCTL,__func__,"changing exit_routine to NULL");
	exit_routine = 0;

	dbgprint(MOD_WVIEWCTL,__func__,"changing module state to 'unloaded'");
	wvctl_loaded = false;

	dbgprint(MOD_WVIEWCTL,__func__,"changing unloading flag to false");
	wvctl_unloading = false;

	dbgprint(MOD_WVIEWCTL,__func__,"module unloaded successfully");
	DBGRET_SUCCESS(MOD_WVIEWCTL);
}

/*
   iwvctl_find_mousecb

   Internal callback to be used with jmlist to find a specific mouse
   callback in the callback list.
*/
jmlist_status
iwvctl_find_mousecb(void *ptr,void *param,jmlist_lookup_result *result)
{
	wvmousecb_t *mousecbt_ptr;
	wvmouse_cb mousecb;

	/* initialize variables */
	mousecbt_ptr = (wvmousecb_t*)ptr;
	memcpy(&mousecb,param,sizeof(wvmouse_cb));

	if( mousecbt_ptr->cb == mousecb )
		*result = jmlist_entry_found;
	else
		*result = jmlist_entry_not_found;

	return JMLIST_ERROR_SUCCESS;
}

/*
   iwvctl_find_keycb

   Internal callback to be used with jmlist to find a specific keyboard
   callback in the callback list.
*/
jmlist_status
iwvctl_find_keycb(void *ptr,void *param,jmlist_lookup_result *result)
{
	wvkeyboardcb_t *keycb_data;
	wvkeyboard_cb keycb;
	
	/* initialize variables */
	keycb_data = (wvkeyboardcb_t*)ptr;
	memcpy(&keycb,param,sizeof(wvkeyboard_cb));

	if( keycb_data->cb == keycb )
		*result = jmlist_entry_found;
	else
		*result = jmlist_entry_not_found;

	return JMLIST_ERROR_SUCCESS;
}

/*
   wvctl_register_keyboard_cb

   First check if the module is unloading, if so, return with error since
   no callback should be registered in that state.

   Then, try to acquire the lock on keyboard callback list so exclusive
   access is granted. A lookup for existent entries with same keycb must
   be done in order to avoid duplicate entries in the list.
   
   If there is no entry with same keycb in the list, add the keyboard
   callback to the list.

   Finally free the keyboard list lock.
*/
wstatus
wvctl_register_keyboard_cb(wvkeyboard_cb keycb,void *param)
{
	wstatus ws;
	jmlist_status jmls;
	void *keycb_cpy;
	wvkeyboardcb_t *new_keycbt;

	memcpy((void*)&keycb_cpy,(void*)&keycb,sizeof(wvkeyboard_cb));

	dbgprint(MOD_WVIEWCTL,__func__,"called with keycb=%p and param=%p",keycb,param);

	if( wvctl_unloading ) {
		dbgprint(MOD_WVIEWCTL,__func__,"module is unloading, cannot register keyboard callback now");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	if( !wvctl_loaded ) {
		dbgprint(MOD_WVIEWCTL,__func__,"cannot use this function while the module was not loaded yet");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"acquiring keyboard callback list lock");
	ws = wlock_acquire(&keyboard_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"unable to acquire keyboard callback list lock");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"looking up if callback keycb=%p exists in the list",keycb);
	jmlist_lookup_result lookup_result;
	jmls = jmlist_find(keyboard_cbl,iwvctl_find_keycb,keycb_cpy,&lookup_result,0);
	if( jmls != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"jmlist_find failed");
		goto release_and_fail;
	}
	
	if( lookup_result == jmlist_entry_found ) {
		dbgprint(MOD_WVIEWCTL,__func__,"callback with keycb=%p already exists in the list",keycb);
		goto release_and_fail;
	}

	new_keycbt = (wvkeyboardcb_t*)malloc(sizeof(wvkeyboardcb_t));
	if( !new_keycbt ) {
		dbgprint(MOD_WVIEWCTL,__func__,"malloc failed");
		goto release_and_fail;
	}

	dbgprint(MOD_WVIEWCTL,__func__,"allocated new keyboard callback structure (new_keycbt=%p)",new_keycbt);

	new_keycbt->cb = keycb;
	new_keycbt->param = param;

	dbgprint(MOD_WVIEWCTL,__func__,"filled new structure with cb=%p and param=%p",
			new_keycbt->cb,new_keycbt->param);

	dbgprint(MOD_WVIEWCTL,__func__,"adding keyboard callback structure to the list");
	jmls = jmlist_insert(keyboard_cbl,new_keycbt);

	dbgprint(MOD_WVIEWCTL,__func__,"releasing keyboard callback list lock");
	ws = wlock_release(&keyboard_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"failed to release lock");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"failed to add keyboard callback to the list");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"keyboard lock released successfully");
	DBGRET_SUCCESS(MOD_WVIEWCTL);

release_and_fail:
	dbgprint(MOD_WVIEWCTL,__func__,"releasing keyboard callback list lock");
	ws = wlock_release(&keyboard_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"failed to release lock");
	}
	DBGRET_FAILURE(MOD_WVIEWCTL);
}

/*
   wvctl_unregister_keyboard_cb

   First it should check if wvctl module is unloading, if it is, ignore the call
   returning with failure. Then, acquire the keyboard callback list lock, so
   it can access to the keyboard list and remove the specific callback. Callbacks
   are identified only by the function address (keycb) so there shouldn't exist
   two callbacks inside the callback list with the same value (this should be
   checked in wvctl_register_keyboard_cb function when registering the callback).
   After acquiring the callback list lock, the first thing it does is to lookup
   for the entry in the callback list, if its not found report and return with
   failure, otherwise proceed to the removal of the entry in the list.

   Finally free the callback list lock.

*/
wstatus
wvctl_unregister_keyboard_cb(wvkeyboard_cb keycb)
{
	jmlist_status jmls;
	wstatus ws;
	void *keycb_cpy;
	wvkeyboardcb_t *keycbt_ptr;

	/* workaround for ISO C unsupport of void* to function pointer conversion,
	this will only work if sizeof(void*) == sizeof(wvkeyboard_cb) */
	memcpy((void*)&keycb_cpy,(void*)&keycb,sizeof(wvkeyboard_cb));

	if( wvctl_unloading ) {
		dbgprint(MOD_WVIEWCTL,__func__,"module is unloading, cannot unregister keyboard callback now");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	if( !wvctl_loaded ) {
		dbgprint(MOD_WVIEWCTL,__func__,"cannot use this function while the module was not loaded yet");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"acquiring keyboard callback list lock");
	ws = wlock_acquire(&keyboard_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"unable to acquire keyboard callback list lock");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"looking up if callback keycb=%p exists in the list",keycb);
	jmlist_lookup_result lookup_result;
	jmls = jmlist_find(keyboard_cbl,iwvctl_find_keycb,keycb_cpy,&lookup_result,(void*)&keycbt_ptr);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"jmlist_ptr_exists failed");
		goto release_and_fail;
	}

	if( lookup_result == jmlist_entry_not_found ) {
		dbgprint(MOD_WVIEWCTL,__func__,"the entry keycb=%p is not in the callback list",keycb);
		goto release_and_fail;
	}

	jmls = jmlist_remove_by_ptr(keyboard_cbl,keycbt_ptr);

	dbgprint(MOD_WVIEWCTL,__func__,"releasing keyboard callback list lock");
	ws = wlock_release(&keyboard_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"failed to release lock");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"failed to remove keyboard callback to the list");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"removed keyboard callback from the list successfully");

	free(keycbt_ptr);
	dbgprint(MOD_WVIEWCTL,__func__,"freed keyboard callback structure memory");

	DBGRET_SUCCESS(MOD_WVIEWCTL);

release_and_fail:
	dbgprint(MOD_WVIEWCTL,__func__,"releasing keyboard callback list lock");
	ws = wlock_release(&keyboard_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"failed to release lock");
	}
	DBGRET_FAILURE(MOD_WVIEWCTL);
}

/*
   wvctl_register_mouse_cb

   This function should work in the same way as of the keyboard register
   callback function.

   Before the callback is added, a search is done in the list so no
   callbacks are inserted twice. One callback (identified only by
   the routine address) can only be inserted once to the list,
   otherwise this function will return error!
*/
wstatus
wvctl_register_mouse_cb(wvmouse_cb mousecb,void *param)
{
	void *mousecb_cpy;
	wstatus ws;
	jmlist_status jmls;
	wvmousecb_t *new_mousecbt;

	/* workaround for ISO C unsupport of void* to function pointer conversion,
	this will only work if sizeof(void*) == sizeof(wvmouse_cb) */
	memcpy((void*)&mousecb_cpy,(void*)&mousecb,sizeof(wvmouse_cb));

	if( wvctl_unloading ) {
		dbgprint(MOD_WVIEWCTL,__func__,"module is unloading, cannot register mouse callback now");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	if( !wvctl_loaded ) {
		dbgprint(MOD_WVIEWCTL,__func__,"cannot use this function while the module was not loaded yet");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"acquiring mouse callback list lock");
	ws = wlock_acquire(&mouse_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"unable to acquire mouse callback list lock");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"looking up if callback mousecb=%p exists in the list",mousecb);
	jmlist_lookup_result lookup_result;
	jmls = jmlist_find(mouse_cbl,iwvctl_find_mousecb,mousecb_cpy,&lookup_result,0);
	if( jmls != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"jmlist_find failed");
		goto release_and_fail;
	}
	
	if( lookup_result == jmlist_entry_found ) {
		dbgprint(MOD_WVIEWCTL,__func__,"callback with mousecb=%p already exists in the list",mousecb);
		goto release_and_fail;
	}
	
	new_mousecbt = (wvmousecb_t*)malloc(sizeof(wvmousecb_t));
	if( !new_mousecbt ) {
		dbgprint(MOD_WVIEWCTL,__func__,"malloc failed");
		goto release_and_fail;
	}
	
	dbgprint(MOD_WVIEWCTL,__func__,"allocated new mouse callback structure (new_mousecbt=%p)",new_mousecbt);
	
	new_mousecbt->cb = mousecb;
	new_mousecbt->param = param;
	
	dbgprint(MOD_WVIEWCTL,__func__,"filled new structure with cb=%p and param=%p",
			 new_mousecbt->cb,new_mousecbt->param);
	
	dbgprint(MOD_WVIEWCTL,__func__,"adding mouse callback structure to the list");
	jmls = jmlist_insert(mouse_cbl,new_mousecbt);
	
	dbgprint(MOD_WVIEWCTL,__func__,"releasing mouse callback list lock");
	ws = wlock_release(&mouse_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"failed to release mouse callback list lock");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"failed to add mouse callback to the list");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"mouse callback list lock released successfully");
	DBGRET_SUCCESS(MOD_WVIEWCTL);

release_and_fail:
	dbgprint(MOD_WVIEWCTL,__func__,"releasing mouse callback list lock");
	ws = wlock_release(&mouse_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"failed to release lock");
	}
	DBGRET_FAILURE(MOD_WVIEWCTL);
}

/*
   wvctl_unregister_mouse_cb

   This function is called whenever the client code wants to unregister
   a mouse callback of the wviewctl. This function is multithread-safe
   thanks to the synchronization objects used in this module.

   This function cannot be used when the module was not loaded yet
   or when the module is unloading (wvctl_unload was called).

   Before the entry is removed this function first check if it actually
   exists, if it doens't exist in the list this function returns error.
*/
wstatus
wvctl_unregister_mouse_cb(wvmouse_cb mousecb)
{
	jmlist_status jmls;
	wstatus ws;
	void *mousecb_cpy;
	wvmousecb_t *mousecbt_ptr;

	dbgprint(MOD_WVIEWCTL,__func__,"called with mousecb=%p",mousecb);

	/* workaround for ISO C unsupport of void* to function pointer conversion,
	this will only work if sizeof(void*) == sizeof(wvkeyboard_cb) */
	memcpy((void*)&mousecb_cpy,(void*)&mousecb,sizeof(wvmouse_cb));

	if( wvctl_unloading ) {
		dbgprint(MOD_WVIEWCTL,__func__,"module is unloading, cannot unregister mouse callback now");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	if( !wvctl_loaded ) {
		dbgprint(MOD_WVIEWCTL,__func__,"cannot use this function while the module was not loaded yet");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"acquiring mouse callback list lock");
	ws = wlock_acquire(&mouse_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"unable to acquire mouse callback list lock");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"looking up if callback mousecb=%p exists in the list",mousecb);
	jmlist_lookup_result lookup_result;
	jmls = jmlist_find(mouse_cbl,iwvctl_find_mousecb,mousecb_cpy,&lookup_result,(void*)&mousecbt_ptr);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"jmlist_ptr_exists failed");
		goto release_and_fail;
	}

	if( lookup_result == jmlist_entry_not_found ) {
		dbgprint(MOD_WVIEWCTL,__func__,"the entry mousecb=%p is not in the callback list",mousecb);
		goto release_and_fail;
	}

	jmls = jmlist_remove_by_ptr(mouse_cbl,mousecbt_ptr);

	dbgprint(MOD_WVIEWCTL,__func__,"releasing mouse callback list lock");
	ws = wlock_release(&mouse_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"failed to release mouse callback list lock");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"failed to remove the mouse callback from the list");
		goto release_and_fail;
	}
	
	free(mousecbt_ptr);
	dbgprint(MOD_WVIEWCTL,__func__,"freed mouse callback structure memory");

	DBGRET_SUCCESS(MOD_WVIEWCTL);

release_and_fail:
	dbgprint(MOD_WVIEWCTL,__func__,"releasing mouse callback list lock");
	ws = wlock_release(&mouse_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"failed to release mouse callback list lock");
	}
	DBGRET_FAILURE(MOD_WVIEWCTL);
}

wstatus
wvctl_redraw(void)
{
	dbgprint(MOD_WVIEWCTL,__func__,"called");

	wvctl_redraw_flag = true;
	dbgprint(MOD_WVIEWCTL,__func__,"redraw flag set to %u",wvctl_redraw_flag);

	DBGRET_SUCCESS(MOD_WVIEWCTL);
}

/*
   wvctl_free_draw_cb

   Internal helper function to free a single draw callback structure.
*/
wstatus
wvctl_free_draw_cb(wvdrawcb_t *drawcb)
{
	dbgprint(MOD_WVIEWCTL,__func__,"called with drawcb=%p",drawcb);

	dbgprint(MOD_WVIEWCTL,__func__,"freeing drawcb=%p\n",drawcb);
	free(drawcb);
	dbgprint(MOD_WVIEWCTL,__func__,"draw callback with drawcb=%p was freed successfully",drawcb);

	DBGRET_SUCCESS(MOD_WVIEWCTL);
}

/*
   wvctl_free_draw_cbl

   Internal helper function to free draw callback list and its lock. This function
   doesn't acquire the draw callback list lock, the caller is responsible for that.
*/
wstatus
wvctl_free_draw_cbl(void)
{
	jmlist_status jmls;
	wstatus ws;
	dbgprint(MOD_WVIEWCTL,__func__,"called");

	dbgprint(MOD_WVIEWCTL,__func__,"freeing draw callback list");
	
	jmlist_index cbcount;
	jmls = jmlist_entry_count(draw_cbl,&cbcount);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		/* something went wrong with getting entry count! */
		dbgprint(MOD_WVIEWCTL,__func__,"unable to get number of callback entries in draw callback list");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	if( cbcount == 0 ) {
//		dbgprint(MOD_WVIEWCTL,__func__,"cannot free draw callback list because its empty");
//		DBGRET_FAILURE(MOD_WVIEWCTL);
		dbgprint(MOD_WVIEWCTL,__func__,"list of callbacks is empty");
		DBGRET_SUCCESS(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"freeing %u callbacks from draw callback list",cbcount);

	wvdrawcb_t *drawcbt_ptr;
	jmlist_index cbcount_org = cbcount-1;
	while( cbcount-- )
	{
		jmls = jmlist_pop(draw_cbl,(void*)&drawcbt_ptr);
	
		if( jmls != JMLIST_ERROR_SUCCESS )
	   	{
			dbgprint(MOD_WVIEWCTL,__func__,"unable to pop draw list entry");
			if( cbcount_org != cbcount )
				dbgprint(MOD_WVIEWCTL,__func__,"warning!! draw callback list was partially freed");
			DBGRET_FAILURE(MOD_WVIEWCTL);
		}
	
		dbgprint(MOD_WVIEWCTL,__func__,"calling wvctl_free_draw_cb(drawcbt=%p)",drawcbt_ptr);
		ws = wvctl_free_draw_cb(drawcbt_ptr);
		dbgprint(MOD_WVIEWCTL,__func__,"wvctl_free_draw_cb returned with ws=%u",ws);
	
		if( ws != WSTATUS_SUCCESS ) 
		{
			dbgprint(MOD_WVIEWCTL,__func__,"wvctl_free_draw_cb failed");
			if( cbcount_org != cbcount )
				dbgprint(MOD_WVIEWCTL,__func__,"warning!! draw callback list was partially freed");
			DBGRET_FAILURE(MOD_WVIEWCTL);
		}

		/* continue to the next callback entry.. */
	}

	/* check if the entry count is zero now. */
	jmls = jmlist_entry_count(draw_cbl,&cbcount);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		/* something went wrong with getting entry count! */
		dbgprint(MOD_WVIEWCTL,__func__,"unable to get number of callback entries in draw callback list (after free loop)");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	/* still have entries... means some other thread is adding entries to the list
	and the code is broken because its not verifying that we're unloading the module! */
	if( cbcount ) {
		dbgprint(MOD_WVIEWCTL,__func__,"after free loop there are still entries in the list, is *anyone* adding entries to the list?");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	/* all entries removed successfully. */
	dbgprint(MOD_WVIEWCTL,__func__,"all draw callback list entries were freed successfully");
	DBGRET_SUCCESS(MOD_WVIEWCTL);
}

/*
   wvctl_free_draw_cbl_lock

   Internal helper function to free draw callback list lock.
*/
wstatus
wvctl_free_draw_cbl_lock(void)
{
	dbgprint(MOD_WVIEWCTL,__func__,"called");

	dbgprint(MOD_WVIEWCTL,__func__,"freeing draw callback list lock");

	if( wlock_free(&draw_cbl_lock) != WSTATUS_SUCCESS ) {
		/* something went wrong with the free, program should abort ASAP.. */
		dbgprint(MOD_WVIEWCTL,__func__,"unable to free draw callback list lock");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"draw callback list lock was freed successfully");
	DBGRET_SUCCESS(MOD_WVIEWCTL);
}

wstatus
wvctl_draw_routine(wvdraw_t draw,void *param)
{
	wstatus ws;
	jmlist_status jmls;
	wvdrawcb_t *drawcbt_ptr;

	dbgprint(MOD_WVIEWCTL,__func__,"called with draw.flags=%d",draw.flags);

	if( !wvctl_loaded ) {
		dbgprint(MOD_WVIEWCTL,__func__,"unexpected call to this function, module not loaded yet");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	if( wvctl_unloading ) {
		dbgprint(MOD_WVIEWCTL,__func__,"module is unloading, ignoring this call");
		DBGRET_SUCCESS(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"acquiring draw callback list lock");
	ws = wlock_acquire(&draw_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"unable to acquire draw callback list lock");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	/* now that we've the lock, parse the drawbaord callback list */
	
	jmlist_index entry_count;

	jmls = jmlist_entry_count(draw_cbl,&entry_count);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"unable to get draw callback list entry count!");
		goto free_lock_fail;
	}

	dbgprint(MOD_WVIEWCTL,__func__,"draw callback list has %u entries now",entry_count);
	
	for( jmlist_index i = 0 ; i < entry_count ; i++ )
	{
		jmls = jmlist_get_by_index(draw_cbl,i,(void*)&drawcbt_ptr);
		if( jmls != JMLIST_ERROR_SUCCESS ) {
			dbgprint(MOD_WVIEWCTL,__func__,"unable to get draw callback list entry #%u",i);
			goto free_lock_fail;
		}

		/* call the client routine */
		dbgprint(MOD_WVIEWCTL,__func__,"calling draw client routine %p (index %u, param=%p)",
				drawcbt_ptr->cb,i,drawcbt_ptr->param);
		drawcbt_ptr->cb(draw,drawcbt_ptr->param);
		dbgprint(MOD_WVIEWCTL,__func__,"draw client routine returned");
	}

	dbgprint(MOD_WVIEWCTL,__func__,"all draw callbacks called, freeing lock");
	ws = wlock_release(&draw_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"failed to release draw callback list lock");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"draw lock released successfully");
	DBGRET_SUCCESS(MOD_WVIEWCTL);

free_lock_fail:
	wlock_release(&draw_cbl_lock);
	DBGRET_FAILURE(MOD_WVIEWCTL);
}

/*
   iwvctl_find_drawcb

   Internal callback to be used with jmlist to find a specific draw
   callback in the callback list.
*/
jmlist_status
iwvctl_find_drawcb(void *ptr,void *param,jmlist_lookup_result *result)
{
	wvdrawcb_t *drawcbt_ptr;
	wvdraw_cb drawcb;

	/* initialize variables */
	drawcbt_ptr = (wvdrawcb_t*)ptr;
	memcpy(&drawcb,param,sizeof(wvdraw_cb));

	if( drawcbt_ptr->cb == drawcb )
		*result = jmlist_entry_found;
	else
		*result = jmlist_entry_not_found;

	return JMLIST_ERROR_SUCCESS;
}

/*
   wvctl_register_draw_cb

   First check if the module is unloading, if so, return with error since
   no callback should be registered in that state.

   Then, try to acquire the lock on draw callback list so exclusive
   access is granted. A lookup for existent entries with same drawcb must
   be done in order to avoid duplicate entries in the list.
   
   If there is no entry with same drawcb in the list, add the draw
   callback to the list.

   Finally free the draw list lock.
*/
wstatus
wvctl_register_draw_cb(wvdraw_cb drawcb,void *param)
{
	wstatus ws;
	jmlist_status jmls;
	void *drawcb_cpy;
	wvdrawcb_t *new_drawcbt;

	memcpy((void*)&drawcb_cpy,(void*)&drawcb,sizeof(wvdraw_cb));

	dbgprint(MOD_WVIEWCTL,__func__,"called with drawcb=%p and param=%p",drawcb,param);

	if( wvctl_unloading ) {
		dbgprint(MOD_WVIEWCTL,__func__,"module is unloading, cannot register draw callback now");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	if( !wvctl_loaded ) {
		dbgprint(MOD_WVIEWCTL,__func__,"cannot use this function while the module was not loaded yet");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"acquiring draw callback list lock");
	ws = wlock_acquire(&draw_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"unable to acquire draw callback list lock");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"looking up if callback drawcb=%p exists in the list",drawcb);
	jmlist_lookup_result lookup_result;
	jmls = jmlist_find(draw_cbl,iwvctl_find_drawcb,drawcb_cpy,&lookup_result,0);
	if( jmls != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"jmlist_find failed");
		goto release_and_fail;
	}
	
	if( lookup_result == jmlist_entry_found ) {
		dbgprint(MOD_WVIEWCTL,__func__,"callback with drawcb=%p already exists in the list",drawcb);
		goto release_and_fail;
	}

	new_drawcbt = (wvdrawcb_t*)malloc(sizeof(wvdrawcb_t));
	if( !new_drawcbt ) {
		dbgprint(MOD_WVIEWCTL,__func__,"malloc failed");
		goto release_and_fail;
	}

	dbgprint(MOD_WVIEWCTL,__func__,"allocated new draw callback structure (new_drawcbt=%p)",new_drawcbt);

	new_drawcbt->cb = drawcb;
	new_drawcbt->param = param;

	dbgprint(MOD_WVIEWCTL,__func__,"filled new structure with cb=%p and param=%p",
			new_drawcbt->cb,new_drawcbt->param);

	dbgprint(MOD_WVIEWCTL,__func__,"adding draw callback structure to the list");
	jmls = jmlist_insert(draw_cbl,new_drawcbt);

	dbgprint(MOD_WVIEWCTL,__func__,"releasing draw callback list lock");
	ws = wlock_release(&draw_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"failed to release lock");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"failed to add draw callback to the list");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"draw lock released successfully");
	DBGRET_SUCCESS(MOD_WVIEWCTL);

release_and_fail:
	dbgprint(MOD_WVIEWCTL,__func__,"releasing draw callback list lock");
	ws = wlock_release(&draw_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"failed to release lock");
	}
	DBGRET_FAILURE(MOD_WVIEWCTL);
}

/*
   wvctl_unregister_draw_cb

   First it should check if wvctl module is unloading, if it is, ignore the call
   returning with failure. Then, acquire the draw callback list lock, so
   it can access to the draw list and remove the specific callback. Callbacks
   are identified only by the function address (drawcb) so there shouldn't exist
   two callbacks inside the callback list with the same value (this should be
   checked in wvctl_register_draw_cb function when registering the callback).
   After acquiring the callback list lock, the first thing it does is to lookup
   for the entry in the callback list, if its not found report and return with
   failure, otherwise proceed to the removal of the entry in the list.

   Finally free the callback list lock.

*/
wstatus
wvctl_unregister_draw_cb(wvdraw_cb drawcb)
{
	jmlist_status jmls;
	wstatus ws;
	void *drawcb_cpy;
	wvdrawcb_t *drawcbt_ptr;

	dbgprint(MOD_WVIEWCTL,__func__,"called with drawcb=%p",drawcb);

	/* workaround for ISO C unsupport of void* to function pointer conversion,
	this will only work if sizeof(void*) == sizeof(wvdraw_cb) */
	memcpy((void*)&drawcb_cpy,(void*)&drawcb,sizeof(wvdraw_cb));

	if( wvctl_unloading ) {
		dbgprint(MOD_WVIEWCTL,__func__,"module is unloading, cannot unregister draw callback now");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	if( !wvctl_loaded ) {
		dbgprint(MOD_WVIEWCTL,__func__,"cannot use this function while the module was not loaded yet");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"acquiring draw callback list lock");
	ws = wlock_acquire(&draw_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"unable to acquire draw callback list lock");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"looking up if callback drawcb=%p exists in the list",drawcb);
	jmlist_lookup_result lookup_result;
	jmls = jmlist_find(draw_cbl,iwvctl_find_drawcb,drawcb_cpy,&lookup_result,(void*)&drawcbt_ptr);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"jmlist_ptr_exists failed");
		goto release_and_fail;
	}

	if( lookup_result == jmlist_entry_not_found ) {
		dbgprint(MOD_WVIEWCTL,__func__,"the entry drawcb=%p is not in the callback list",drawcb);
		goto release_and_fail;
	}

	jmls = jmlist_remove_by_ptr(draw_cbl,drawcbt_ptr);

	dbgprint(MOD_WVIEWCTL,__func__,"releasing draw callback list lock");
	ws = wlock_release(&draw_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"failed to release lock");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	if( jmls != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"failed to remove draw callback to the list");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	dbgprint(MOD_WVIEWCTL,__func__,"removed draw callback from the list successfully");

	free(drawcbt_ptr);
	dbgprint(MOD_WVIEWCTL,__func__,"freed draw callback structure memory");

	DBGRET_SUCCESS(MOD_WVIEWCTL);

release_and_fail:
	dbgprint(MOD_WVIEWCTL,__func__,"releasing draw callback list lock");
	ws = wlock_release(&draw_cbl_lock);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"failed to release lock");
	}
	DBGRET_FAILURE(MOD_WVIEWCTL);
}

/*
   wvctl_closewnd_routine

   Callback function called by wview when the window is closed.
*/
wstatus wvctl_closewnd_routine(bool *ignore)
{
	dbgprint(MOD_WVIEWCTL,__func__,"called with ignore=%p",ignore);

	dbgprint(MOD_WVIEWCTL,__func__,"setting ignore flag to false");
	*ignore = true;
	dbgprint(MOD_WVIEWCTL,__func__,"ignore flag set to %u",*ignore);

	wvctl_closed = true;
	wvctl_unloading = true;

	DBGRET_SUCCESS(MOD_WVIEWCTL);
}

