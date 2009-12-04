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
#include "wopengl.h"

int main(int argc,char *argv[])
{
	wgl_init_t wgli = { .argc = argc, .argv = argv };

	wgl_initialize(&wgli);

	wgl_create_window();
	wgl_main_loop();

	wgl_uninitialize();

	return EXIT_SUCCESS;
}

