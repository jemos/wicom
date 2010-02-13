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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "wstatus.h"
#include "debug.h"

modname modname_list[] = {
	{MOD_WICOM,"wicom"},
	{MOD_WOPENGL,"wopengl"},
	{MOD_CONSOLE,"console"},
	{MOD_MAPMGR,"mapmgr"},
	{MOD_APMGR,"apmgr"},
	{MOD_SCANMGR,"scanmgr"},
	{MOD_DEBUG,"debug"},
	{MOD_WTHREAD,"wthread"},
	{MOD_WLOCK,"wlock"}
};
#define MOD_COUNT (sizeof(modname_list)/sizeof(modname))

wstatus
modt2name(debug_mod_t module,const char **name)
{
	if( !name )
	{
//		dbgprint(MOD_DEBUG,__func__,"Invalid name specified (expecting pointer to char*)");
//		dbgprint(MOD_DEBUG,__func__,"Returing with failure.");
		return WSTATUS_FAILURE;
	}

	*name = NULL;

	for( unsigned int i = 0 ; i < MOD_COUNT ; i++ )
	{
		if( modname_list[i].mod == module )
		{
			*name = modname_list[i].name;
//			dbgprint(MOD_DEBUG,__func__,"Found module, updated name argument");
//			dbgprint(MOD_DEBUG,__func__,"Returning with success.");
			return WSTATUS_SUCCESS;
		}
	}

//	dbgprint(MOD_DEBUG,__func__,"Module (%d) not found, unsupported?!",module);
//	dbgprint(MOD_DEBUG,__func__,"Returning with failure.");
	return WSTATUS_FAILURE;
}

void
dbgprint(debug_mod_t module,const char *func,char *fmt,...)
{
	va_list vl;

	const char *modn;

	modt2name(module,&modn);

	fprintf(stderr,"%s %s: ",modn,func);

	va_start(vl,fmt);
	vfprintf(stderr,fmt,vl);
	va_end(vl);

	fprintf(stderr,"\n");
	
	return;
}

