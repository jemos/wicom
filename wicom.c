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

//#include "cfgmgr.h"
//#include "mapmgr.h"
//#include "apmgr.h"
//#include "console.h"
#include "wstatus.h"
#include "debug.h"
#include "wview.h"

int main(int argc,char *argv[])
{
	wstatus s;
	wview_load_t load;

	/* load wview */
	s = wview_load(load);
	if( s != WSTATUS_SUCCESS )
		return EXIT_SUCCESS;


	/* unload wview */
	s = wview_unload();
	return EXIT_SUCCESS;
}

