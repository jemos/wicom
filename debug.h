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

#ifndef _DEBUG_H
#define _DEBUG_H

#ifndef DEBUG_LOOPS
#define DEBUG_LOOPS 0
#endif

/* this enum is to identify each module and will be useful for
   debugging messages filtering. */
typedef enum _debug_mod_t {
	MOD_WICOM = 1,
	MOD_WOPENGL = 2,
	MOD_CONSOLE = 4,
	MOD_MAPMGR = 8,
	MOD_APMGR = 16,
	MOD_MODMGR = 32,
	MOD_DEBUG = 64,
	MOD_WTHREAD = 128,
	MOD_WLOCK = 256,
	MOD_WVIEW = 512,
	MOD_WVIEWCTL = 1024,
	MOD_SHAPEMGR = 2048,
	MOD_WCHANNEL = 4096
} debug_mod_t;
/* maximum modules for debug... 32 */

typedef struct _modname {
	debug_mod_t mod;
	const char *name;
} modname;

#define DBGRET_SUCCESS(mod) dbgprint(mod,__func__,"Returning with success."); return WSTATUS_SUCCESS;
#define DBGRET_FAILURE(mod) dbgprint(mod,__func__,"Returning with failure."); return WSTATUS_FAILURE;

wstatus modt2name(debug_mod_t module,const char **name);
void dbgprint(debug_mod_t module,const char *func,char *fmt,...);

#endif

