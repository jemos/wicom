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

#ifndef _WTHREAD_H
#define _WTHREAD_H

#ifdef pthread_create
#include <pthread.h>
#define THREAD_API 1 /* Linux or MacOS pthread */
#endif

#ifdef WIN32
#include <windows.h>
#define THREAD_API 2 /* Windows */
#endif

#ifndef THREAD_API
#error No thread API supported by wicom found in your OS.
#endif

#include "wstatus.h"

#if THREAD_API == 1
typedef pthread_t wthread_t;
#elif THREAD_API == 2
typedef HANDLE wthread_t;
#endif

wstatus wthread_create(wthread_routine_t routine,void *param,wthread_t *thread);
wstatus wthread_wait(wthread_t thread);

#endif

