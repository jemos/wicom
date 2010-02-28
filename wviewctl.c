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

#include "posh.h"
#include "wstatus.h"
#include "wviewcb.h"
#include "wviewctl.h"
#include "wthread.h"
#include "wlock.h"
#include "jmlist.h"

/* define the callback lists and their associated synchronization object */

static jmlist keyboard_cbl;
static wlock_t keyboard_cbl_lock;
static jmlist mouse_cbl;
static wlock_t mouse_cbl_lock;
static bool wvctl_loaded = false;

/* structure that is temporarly used to store thread initialization status */

static struct _thread_init_status {
	bool init_finished_flag;
	wstatus init_finished_status;
} thread_init_status;

/* some internal functions used by this module */

wstatus wvctl_free_keyboard_cbl(void);
wstatus wvctl_free_keyboard_cbl_lock(void);
wstatus wvctl_free_mouse_cbl(void);
wstatus wvctl_free_mouse_cbl_lock(void);
void wvctl_worker_routine(void *param);

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
wvctl_load(void)
{
	struct _jmlist_params jmlp;
	jmlist_status jmls;
	wstatus ws;

	dbgprint(MOD_WVIEWCTL,__func__,"called");

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
		DBRET_FAILURE(MOD_WVIEWCTL);
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
	return;
}

wstatus
wvctl_unload(void)
{
}

wstatus
wvctl_register_keyboard_cb(wvkeyboard_cb keycb,void *param)
{
}

wstatus
wvctl_unregister_keyboard_cb(wvkeyboard_cb keycb,void *param)
{
}

wstatus
wvctl_register_mouse_cb(wvmouse_cb mousecb,void *param)
{
}

wstatus
wvctl_unregister_mouse_cb(wvmouse_cb mousecb,void *param)
{
}

wstatus
wvctl_redraw(void)
{
}

