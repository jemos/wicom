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
#include <assert.h>

#include "wstatus.h"
#include "debug.h"

modname modname_list[] = {
	{MOD_WICOM,"wicom"},
	{MOD_WOPENGL,"wopengl"},
	{MOD_REQ,"request"},
	{MOD_NVPAIR,"nvpair"},
	{MOD_REQBUF,"reqbuf"},
	{MOD_MODMGR,"modmgr"},
	{MOD_DEBUG,"debug"},
	{MOD_WTHREAD,"wthread"},
	{MOD_WLOCK,"wlock"},
	{MOD_WVIEW,"wview"},
	{MOD_WVIEWCTL,"wviewctl"},
	{MOD_SHAPEMGR,"shapemgr"},
	{MOD_WCHANNEL,"wchannel"}
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

const char *z_ptr(const char *ptr)
{
	if( !ptr )
		return "NULLPTR";

	if( !strlen(ptr) )
		return "NULLSIZE";

	return ptr;
}

const char *array2z(const char *buf_ptr,unsigned int buf_size)
{
	static char *l_buf_ptr = 0;
	static size_t l_buf_size = 0;

	if( !buf_ptr ) return "NULLPTR";
	
	if( !l_buf_ptr ) {
		l_buf_ptr = (char*)malloc(sizeof(char)*256);
		l_buf_size = 256;
	}

	if( buf_size > l_buf_size ) {
		assert(l_buf_ptr != 0);
		free(l_buf_ptr);
		l_buf_ptr = (char*)malloc(sizeof(char)*(buf_size+1));
		l_buf_size = buf_size + 1;
	}

	memset(l_buf_ptr,l_buf_size,0);
	assert(l_buf_size >= buf_size);
	memcpy(l_buf_ptr,buf_ptr,buf_size);
	
	return l_buf_ptr;
}

