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

/*
   Module Description

   Lock is a syncronization object, which can only have 2 states,
   locked or unlocked. The client can:
   - create and free locks
   - acquire and release

   I decided to call lock instead of mutex since in different OS
   the name of this syncronization object changes (in Linux is
   a mutex, in Windows its called event).

   -- Jean Mousinho (Feb. 2010)
*/

#ifndef _WLOCK_H
#define _WLOCK_H

#include "posh.h"

#if (defined POSH_OS_LINUX || defined POSH_OS_OSX)
#include <pthread.h>
#include <errno.h>
#define LOCK_API 1
#endif

#if (defined POSH_OS_WIN32 || defined POSH_OS_WIN64)
#include <windows.h>
#define LOCK_API 2
#endif

#ifndef LOCK_API
#error Wicom doesn't support your OS synchronization objects.
#endif

#if LOCK_API == 1
typedef pthread_mutex_t wlock_t;
#elif LOCK_API == 2
typedef HANDLE wlock_t;
#endif

wstatus wlock_create(wlock_t *lock);
wstatus wlock_free(wlock_t *lock);
wstatus wlock_acquire(wlock_t *lock);
wstatus wlock_release(wlock_t *lock);

#endif

