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
#include "wthread.h"

wstatus
wthread_create(wthread_routine_t routine,void *param,wthread_t *thread)
{
	dbgprint(MOD_WTHREAD,__func__,"called with routine=%p, param=%p, thread=%p",routine,param,thread);

#if THREAD_API == 1
	int status;
	status = pthread_create(thread,NULL,(void*(*)(void*))routine,param);

	switch(status)
	{
		case EAGAIN:
			/* The system lacked the necessary resources to create
			   another thread, or the system-imposed limit on the
			   total number of threads in a process [PTHREAD_THREADS_MAX]
			   would be exceeded. */
			dbgprint(MOD_WTHREAD,__func__,"Resources unavailable to create another thread");
			dbgprint(MOD_WTHREAD,__func__,"Returning with failure.");
			return WSTATUS_FAILURE;
		case EINVAL:
			dbgprint(MOD_WTHREAD,__func__,"Returning with failure.");
			return WSTATUS_INVALID_ARGUMENT;
		case 0:
			/* Success! */
			dbgprint(MOD_WTHREAD,__func__,"Thread created successfully");
			dbgprint(MOD_WTHREAD,__func__,"Returning with success.");
			return WSTATUS_SUCCESS;
		default:
			/* Unable to process return value of pthread_create! */
			dbgprint(MOD_WTHREAD,__func__,"Unable to process pthread_create return value (status=%X)",status);
			dbgprint(MOD_WTHREAD,__func__,"Returning with failure.");
			return WSTATUS_FAILURE;
	}
#else
	dbgprint(MOD_WTHREAD,__func__,"This function is unimplemented.");
	return WSTATUS_UNIMPLEMENTED;
#endif

	return WSTATUS_FAILURE;
}

/*
   wthread_wait

   This function will not return to the caller until the thread
   finishes. As of Linux threads, the value_ptr feature is not
   supported in wthreads for wider portability.
*/
wstatus
wthread_wait(wthread_t thread)
{
	dbgprint(MOD_WTHREAD,__func__,"called with thread=%p",thread);

#if THREAD_API == 1
	int status;
	
	status = pthread_join(thread,0);
	switch(status)
	{
		case 0:
			dbgprint(MOD_WTHREAD,__func__,"Wait on thread successful");
			dbgprint(MOD_WTHREAD,__func__,"Returning with success.");
			return WSTATUS_SUCCESS;
		case EDEADLK:
			/* A deadlock was detected or the value of thread 
			   specifies the calling thread. */
			dbgprint(MOD_WTHREAD,__func__,"Deadlock detected or thread is calling thread");
			dbgprint(MOD_WTHREAD,__func__,"Returning with failure.");
			return WSTATUS_FAILURE;
		case EINVAL:
			/* The implementation has detected that the value
			   specified by thread does not refer to a joinable 
			   thread. */
			dbgprint(MOD_WTHREAD,__func__,"The thread specified cannot be joined");
			dbgprint(MOD_WTHREAD,__func__,"Returning with failure.");
			return WSTATUS_FAILURE;
		case ESRCH:
			/* No thread could be found corresponding to that 
			   specified by the given thread ID, thread. */
			dbgprint(MOD_WTHREAD,__func__,"Invalid thread specified");
			dbgprint(MOD_WTHREAD,__func__,"Returning with failure.");
			return WSTATUS_INVALID_ARGUMENT;
		default:
			/* Unable to process return value, this is
			   implementation/code problem! */
			dbgprint(MOD_WTHREAD,__func__,"Unable to process pthread_join return value (status=%X)",status);
			dbgprint(MOD_WTHREAD,__func__,"Returning with failure.");
			return WSTATUS_FAILURE;
	}
#else
	dbgprint(MOD_WTHREAD,__func__,"This function is unimplemented.");
	return WSTATUS_UNIMPLEMENTED;
#endif

	return WSTATUS_FAILURE;
}

/* 
   wthread_exit

   Alternative way of exiting a thread is calling this function.
   Although it has wstatus return type it actually only returns to
   the caller when some error occured.
*/
wstatus
wthread_exit(void)
{
	dbgprint(MOD_WTHREAD,__func__,"called");

#if THREAD_API == 1
	dbgprint(MOD_WTHREAD,__func__,"Passing control to pthread_exit.");
	pthread_exit(0);

	dbgprint(MOD_WTHREAD,__func__,"Unexpected pthread_exit return!?");
	dbgprint(MOD_WTHREAD,__func__,"Returning with failure.");
	return WSTATUS_FAILURE;
#else
	dbgprint(MOD_WTHREAD,__func__,"This function is unimplemented.");
	return WSTATUS_UNIMPLEMENTED;
#endif

	/* it should not come here.. */
	return WSTATUS_FAILURE;
}

