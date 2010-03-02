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
#ifndef sleep
#include <unistd.h>
#endif

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
static bool wvctl_loaded = false;
static bool wvctl_unloading = false;

static wvctl_exit_routine_cb exit_routine = 0;
static wthread_t worker_thread;

/* structure that is temporarly used to store thread initialization status */

typedef struct _thread_init_status {
	bool init_finished_flag;
	wstatus init_finished_status;
} thread_init_status;

/* some internal functions used by this module */

wstatus wvctl_free_keyboard_cbl(void);
wstatus wvctl_free_keyboard_cbl_lock(void);
wstatus wvctl_free_mouse_cbl(void);
wstatus wvctl_free_mouse_cbl_lock(void);
void wvctl_worker_routine(void *param);
wstatus wvctl_keyboard_routine(wvkey_t key,wvkey_mode_t key_mode);
wstatus wvctl_mouse_routine(wvmouse_t mouse);
wstatus wvctl_draw_routine(wvdraw_t draw);

/*
   wvctl_free_keyboard_cbl

   Internal helper function to free keyboard callback list and its lock.
*/
wstatus
wvctl_free_keyboard_cbl(void)
{
	jmlist_status jmls;
	dbgprint(MOD_WVIEWCTL,__func__,"called");

	dbgprint(MOD_WVIEWCTL,__func__,"freeing keyboard callback list");
	
	jmls = jmlist_free(keyboard_cbl);
	if( jmls != JMLIST_ERROR_SUCCESS ) {
		/* something went wrong with the free, program should abort ASAP.. */
		dbgprint(MOD_WVIEWCTL,__func__,"unable to free keyboard callback list");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

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

	/* window was created successfully, pass the CPU to the main loop */
	ws = wview_process(WVIEW_SYNCHRONOUS);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"wview_process failed");
		dbgprint(MOD_WVIEWCTL,__func__,"Returning with no status.");
		return;
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
	dbgprint(MOD_WVIEWCTL,__func__,"destroying the wview window");
	ws = wview_destroy_window();
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_WVIEWCTL,__func__,"unable to destroy wview window");
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
wvctl_keyboard_routine(wvkey_t key,wvkey_mode_t key_mode)
{
	wstatus ws;
	jmlist_status jmls;

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
	
	wvkeyboard_cb keyboard_cb;
	for( jmlist_index i = 0 ; i < entry_count ; i++ )
	{
		jmls = jmlist_get_by_index(keyboard_cbl,i,(void*)&keyboard_cb);
		if( jmls != JMLIST_ERROR_SUCCESS ) {
			dbgprint(MOD_WVIEWCTL,__func__,"unable to get keyboard callback list entry #%u",i);
			goto free_lock_fail;
		}

		/* call the client routine */
		dbgprint(MOD_WVIEWCTL,__func__,"calling keyboard client routine %p (index %u)",keyboard_cb,i);
		keyboard_cb(key,key_mode);
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
wvctl_draw_routine(wvdraw_t draw)
{
	DBGRET_SUCCESS(MOD_WVIEWCTL);
}

wstatus
wvctl_mouse_routine(wvmouse_t mouse)
{
	DBGRET_SUCCESS(MOD_WVIEWCTL);
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
	dbgprint(MOD_WVIEWCTL,__func__,"changing exit_routine to NULL");
	exit_routine = 0;

	dbgprint(MOD_WVIEWCTL,__func__,"changing module state to 'unloaded'");
	wvctl_loaded = false;

	dbgprint(MOD_WVIEWCTL,__func__,"changing unloading flag to false");
	wvctl_unloading = false;

	dbgprint(MOD_WVIEWCTL,__func__,"module unloaded successfully");
	DBGRET_SUCCESS(MOD_WVIEWCTL);
}

wstatus
wvctl_register_keyboard_cb(wvkeyboard_cb keycb,void *param)
{
	if( wvctl_unloading ) {
		dbgprint(MOD_WVIEWCTL,__func__,"module is unloading, cannot register keyboard callback now");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	DBGRET_SUCCESS(MOD_WVIEWCTL);
}

wstatus
wvctl_unregister_keyboard_cb(wvkeyboard_cb keycb,void *param)
{
	if( wvctl_unloading ) {
		dbgprint(MOD_WVIEWCTL,__func__,"module is unloading, cannot unregister keyboard callback now");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	DBGRET_SUCCESS(MOD_WVIEWCTL);
}

wstatus
wvctl_register_mouse_cb(wvmouse_cb mousecb,void *param)
{
	if( wvctl_unloading ) {
		dbgprint(MOD_WVIEWCTL,__func__,"module is unloading, cannot register mouse callback now");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	DBGRET_SUCCESS(MOD_WVIEWCTL);
}

wstatus
wvctl_unregister_mouse_cb(wvmouse_cb mousecb,void *param)
{
	if( wvctl_unloading ) {
		dbgprint(MOD_WVIEWCTL,__func__,"module is unloading, cannot unregister mouse callback now");
		DBGRET_FAILURE(MOD_WVIEWCTL);
	}

	DBGRET_SUCCESS(MOD_WVIEWCTL);
}

wstatus
wvctl_redraw(void)
{
	DBGRET_SUCCESS(MOD_WVIEWCTL);
}

