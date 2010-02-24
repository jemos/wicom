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

//#include "cfgmgr.h"
//#include "mapmgr.h"
//#include "apmgr.h"
//#include "console.h"
#include "posh.h"
#include "wstatus.h"
#include "debug.h"
#include "wview.h"

wstatus
draw_routine(wvdraw_t draw)
{
	if( draw.flags & WVDRAW_FLAG_ORTHOGONAL )
	{
		shape_t s;
		memset(&s,0,sizeof(s));
		s.type = SHAPE_LINE;
		s.data.line.color.x = 1.0; s.data.line.color.y = 1.0; s.data.line.color.z = 1.0;
		s.data.line.p1.x = -10.0; s.data.line.p1.y = 0; s.data.line.p1.z = 0;
		s.data.line.p2.x = +10.0; s.data.line.p2.y = 0; s.data.line.p2.z = 0;
		wview_draw_shape(s);
	} else if( draw.flags & WVDRAW_FLAG_PERSPECTIVE )
	{
	}

	return WSTATUS_SUCCESS;
}

wstatus POSH_CDECL
keyboard_routine(wvkey_t key,wvkey_mode_t key_mode)
{
	printf("key pressed: <%02X>\n",key.key);
	return WSTATUS_SUCCESS;
}

wstatus POSH_CDECL
mouse_routine(wvmouse_t mouse)
{
	return WSTATUS_SUCCESS;
}

int main(int argc,char *argv[])
{
	wstatus s;
	wview_load_t load;

	/* load wview */
	s = wview_load(load);
	if( s != WSTATUS_SUCCESS )
		return EXIT_SUCCESS;


	wview_window_t window;
	window.width = 800;
	window.height = 600;
	window.draw_routine = draw_routine;
	window.keyboard_routine = keyboard_routine;
	window.mouse_routine = mouse_routine;
	wview_create_window(window);

	/* enter draw loop */
	wview_loop();

	/* close window */
	//no need to call this with wview_loop()
	//wview_destroy_window();

	/* unload wview */
	s = wview_unload();
	return EXIT_SUCCESS;
}

