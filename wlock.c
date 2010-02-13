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

#include "wstatus.h"
#include "debug.h"
#include "wlock.h"

wstatus
wlock_create(wlock_t *lock)
{
	dbgprint(MOD_WLOCK,__func__,"called with lock=%p",(void*)lock);

	if( lock == 0 ) {
		/* lock argument was not initialized. */
		dbgprint(MOD_WLOCK,__func__,"Invalid wlock_t pointer specified (lock=0)");
		return WSTATUS_INVALID_ARGUMENT;
	}

#if LOCK_API == 1
	int status;
	status = pthread_mutex_init(lock,NULL);

	switch(status)
	{
		case 0:
			dbgprint(MOD_WLOCK,__func__,"New mutex initialized (mutex=%u) successfully",*lock);
			dbgprint(MOD_WLOCK,__func__,"Returning with success.");
			return WSTATUS_SUCCESS;
		case EAGAIN:
			/* The system temporarily lacks the resources to create another mutex. */
			dbgprint(MOD_WLOCK,__func__,"Lack of resources to create another mutex");
			dbgprint(MOD_WLOCK,__func__,"Returning with failure.");
			return WSTATUS_FAILURE;
		case EINVAL:
			/* The value specified by attr is invalid, this shouldn't happen... */
			dbgprint(MOD_WLOCK,__func__,"The value specified by attr is invalid (used NULL)");
			dbgprint(MOD_WLOCK,__func__,"Returning with failure.");
			return WSTATUS_FAILURE;
		case ENOMEM:
			/* The process cannot allocate enough memory to create another mutex. */
			dbgprint(MOD_WLOCK,__func__,"Unable to allocate enough memory for another mutex");
			dbgprint(MOD_WLOCK,__func__,"Returning with failure.");
			return WSTATUS_FAILURE;
		default:
			/* Unable to process return value, this is
			   implementation/code problem! */
			dbgprint(MOD_WLOCK,__func__,"Unable to evaluate pthread_mutex_destroy return value (0x%X)",status);
			dbgprint(MOD_WLOCK,__func__,"Returning with failure.");
			return WSTATUS_FAILURE;
	}
#else
	dbgprint(MOD_WLOCK,__func__,"This function is unimplemented.");
	return WSTATUS_UNIMPLEMENTED;
#endif
}

wstatus
wlock_free(wlock_t *lock)
{
	dbgprint(MOD_WLOCK,__func__,"called with lock=%p",(void*)lock);
	if( lock == 0 ) {
		dbgprint(MOD_WLOCK,__func__,"Invalid wlock_t pointer specified (lock=0)");
		/* lock argument was not initialized. */
		return WSTATUS_INVALID_ARGUMENT;
	}

#ifdef LOCK_API == 1
	int status;
	status = pthread_mutex_destroy(lock);
	switch(status)
	{
		case 0:
			/* *lock = 0;*/
			dbgprint(MOD_WLOCK,__func__,"Mutex destroyed successfully");
			dbgprint(MOD_WLOCK,__func__,"Returning with success.");
			return WSTATUS_SUCCESS;
		case EBUSY:
			/* Mutex is locked by a thread. */
			dbgprint(MOD_WLOCK,__func__,"Cannot destroy the mutex because it's locked by some thread");
			dbgprint(MOD_WLOCK,__func__,"Returning with failure.");
			return WSTATUS_FAILURE;
		case EINVAL:
			/* The value specified by mutex is invalid. */
			dbgprint(MOD_WLOCK,__func__,"Mutex is not valid (mutex=%p)",*lock);
			dbgprint(MOD_WLOCK,__func__,"Returning with failure.");
			return WSTATUS_INVALID_ARGUMENT;
		default:
			/* Unable to process return value, this is 
			   implementation/code problem! */
			dbgprint(MOD_WLOCK,__func__,"Unable to evaluate pthread_mutex_destroy return value (0x%X)",status);
			dbgprint(MOD_WLOCK,__func__,"Returning with failure.");
			return WSTATUS_FAILURE;
	}
#else
	dbgprint(MOD_WLOCK,__func__,"This function is unimplemented.");
	return WSTATUS_UNIMPLEMENTED;
#endif
}

wstatus
wlock_acquire(wlock_t *lock)
{
	dbgprint(MOD_WLOCK,__func__,"called with lock=%p",(void*)lock);
	if( lock == 0 ) {
		/* lock argument was not initialized. */
		dbgprint(MOD_WLOCK,__func__,"Invalid wlock_t pointer specified (lock=0)");
		return WSTATUS_INVALID_ARGUMENT;
	}

#ifdef LOCK_API == 1
	int status;
	status = pthread_mutex_lock(lock);
	switch(status)
	{	
		case 0:
			dbgprint(MOD_WLOCK,__func__,"Mutex was locked successfully");
			dbgprint(MOD_WLOCK,__func__,"Returning with success.");
			return WSTATUS_SUCCESS;
		case EDEADLK:
			/* A deadlock would occur if the thread blocked waiting for mutex. */
			dbgprint(MOD_WLOCK,__func__,"Anti-deadlock procedure, lock was aborted");
			dbgprint(MOD_WLOCK,__func__,"Returning with failure.");
			return WSTATUS_FAILURE;
		case EINVAL:
			/* The value specified by mutex is invalid. */
			dbgprint(MOD_WLOCK,__func__,"Mutex is not valid (mutex=%p)",*lock);
			dbgprint(MOD_WLOCK,__func__,"Returning with failure.");
			return WSTATUS_INVALID_ARGUMENT;
		default:
			/* Unable to process return value, this is
			   implementation/code problem! */
			dbgprint(MOD_WLOCK,__func__,"Unable to evaluate pthread_mutex_lock return value (0x%X)",status);
			dbgprint(MOD_WLOCK,__func__,"Returning with failure.");
			return WSTATUS_FAILURE;
	}
#else
	dbgprint(MOD_WLOCK,__func__,"This function is unimplemented.");
	return WSTATUS_UNIMPLEMENTED;
#endif
}

wstatus
wlock_release(wlock_t *lock)
{
	dbgprint(MOD_WLOCK,__func__,"called with lock=%p",(void*)lock);
	if( lock == 0 ) {
		/* lock argument was not initialized. */
		return WSTATUS_INVALID_ARGUMENT;
	}

#ifdef LOCK_API == 1
	int status;
	status = pthread_mutex_unlock(lock);
	switch(status)
	{
		case 0:
			dbgprint(MOD_WLOCK,__func__,"Mutex was unlocked successfully");
			dbgprint(MOD_WLOCK,__func__,"Returning with success.");
			return WSTATUS_SUCCESS;
		case EINVAL:
			/* The value specified by mutex is invalid. */
			dbgprint(MOD_WLOCK,__func__,"Mutex is not valid (mutex=%p)",*lock);
			dbgprint(MOD_WLOCK,__func__,"Returning with failure.");
			return WSTATUS_INVALID_ARGUMENT;
		case EPERM:
			/* The current thread does not hold a lock on mutex. */
			dbgprint(MOD_WLOCK,__func__,"This thread does not hold a lock on the mutex");
			dbgprint(MOD_WLOCK,__func__,"Returning with failure.");
			return WSTATUS_FAILURE;
		default:
			/* Unable to process return value, this is
			   implementation/code problem! */
			dbgprint(MOD_WLOCK,__func__,"Unable to evaluate pthread_mutex_lock return value (0x%X)",status);
			dbgprint(MOD_WLOCK,__func__,"Returning with failure.");
			return WSTATUS_FAILURE;
	}
#else
	dbgprint(MOD_WLOCK,__func__,"This function is unimplemented.");
	return WSTATUS_UNIMPLEMENTED;
#endif
}

