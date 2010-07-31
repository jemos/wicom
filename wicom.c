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
#include "wviewctl.h"
#include "wview.h"
#include "wchannel.h"

double vtest = 50.0;

wstatus
draw_routine2(wvdraw_t draw,void *param)
{
	shape_t s;
	memset(&s,0,sizeof(s));

	if( draw.flags & WVDRAW_FLAG_ORTHOGONAL )
	{
		s.type = SHAPE_TEXT;
		s.data.text.content = "XPTO IN ORTHO!";
		s.data.text.position.x = 200.0;
		s.data.text.position.y = 200.0;
		s.data.text.position.z = 0;
		s.data.text.font = TEXT_FONT_SMALL;
		V3_SET(s.data.text.color,0,0,1);
		wview_draw_shape(s);
	}

	return WSTATUS_SUCCESS;
}

wstatus
draw_routine(wvdraw_t draw,void *param)
{
	shape_t s;
	memset(&s,0,sizeof(s));

	if( draw.flags & WVDRAW_FLAG_ORTHOGONAL )
	{
		v3d_t plist[] = {
			{-50,50,0},
			{50,50,0},
			{50,-50,0},
			{-50,-50,0}
		};

		s.type = SHAPE_POLYGON;
		V3_SET(s.data.polygon.color,1.0,0,0)
		s.data.polygon.point_list = plist;
		s.data.polygon.point_count = 4;
		wview_draw_shape(s);

		s.type = SHAPE_LINE;
		s.data.line.color.x = 1.0; s.data.line.color.y = 1.0; s.data.line.color.z = 1.0;
		s.data.line.p1.x = -5.0+800.0/2; s.data.line.p1.y = 600/2; s.data.line.p1.z = 0;
		s.data.line.p2.x = +5.0+800.0/2; s.data.line.p2.y = 600/2; s.data.line.p2.z = 0;
		wview_draw_shape(s);

		s.type = SHAPE_LINE;
		s.data.line.color.x = 1.0; s.data.line.color.y = 1.0; s.data.line.color.z = 1.0;
		s.data.line.p1.x = 800/2; s.data.line.p1.y = -5.0+600.0/2.0; s.data.line.p1.z = 0;
		s.data.line.p2.x = 800/2; s.data.line.p2.y = +5.0+600.0/2.0; s.data.line.p2.z = 0;
		wview_draw_shape(s);

		s.type = SHAPE_POINT;
		V3_SET(s.data.point.p1,0,0,0);
		V3_SET(s.data.point.color,1,0,0);
		wview_draw_shape(s);

		s.type = SHAPE_TEXT;
		s.data.text.content = "XPTO IN ORTHO!";
		s.data.text.position.x = vtest;
		s.data.text.position.y = 50.0;
		s.data.text.position.z = 0;
		s.data.text.font = TEXT_FONT_SMALL;
		V3_SET(s.data.text.color,0,1,0);
		wview_draw_shape(s);
	} else if( draw.flags & WVDRAW_FLAG_PERSPECTIVE )
	{
		s.type = SHAPE_LINE;
		s.data.line.color.x = 1.0; s.data.line.color.y = 1.0; s.data.line.color.z = 1.0;
		s.data.line.p1.x = -5.0; s.data.line.p1.y = 0; s.data.line.p1.z = 0;
		s.data.line.p2.x = +5.0; s.data.line.p2.y = 0; s.data.line.p2.z = 0;
		wview_draw_shape(s);

		s.type = SHAPE_LINE;
		s.data.line.color.x = 1.0; s.data.line.color.y = 1.0; s.data.line.color.z = 1.0;
		s.data.line.p1.x = 0; s.data.line.p1.y = -5.0; s.data.line.p1.z = 0;
		s.data.line.p2.x = 0; s.data.line.p2.y = +5.0; s.data.line.p2.z = 0;
		wview_draw_shape(s);

		s.type = SHAPE_TEXT;
		s.data.text.content = "XPTO SMALL";
		s.data.text.position.x = 50.0;
		s.data.text.position.y = 50.0;
		s.data.text.position.z = 0;
		s.data.text.font = TEXT_FONT_SMALL;
		V3_SET(s.data.text.color,0,1,0);
		wview_draw_shape(s);

		s.type = SHAPE_TEXT;
		s.data.text.content = "XPTO-Z=20";
		s.data.text.position.x = 50.0;
		s.data.text.position.y = 50.0;
		s.data.text.position.z = 20.0;
		s.data.text.font = TEXT_FONT_SMALL;
		V3_SET(s.data.text.color,0,0.5,1);
		wview_draw_shape(s);
	}

	return WSTATUS_SUCCESS;
}

wstatus POSH_CDECL
keyboard_routine(wvkey_t key,wvkey_mode_t key_mode,void *param)
{
	printf("key pressed: <%02X>\n",key.key);
	
	vtest += 10.0;

	wvctl_redraw();

	return WSTATUS_SUCCESS;
}

wstatus POSH_CDECL
mouse_routine(wvmouse_t mouse,void *param)
{
	printf("mouse used: button %d (param=%p)\n",mouse.button,param); /*TODO*/
	return WSTATUS_SUCCESS;
}

void wchannel_test(void)
{
	wchannel_load_t load;
	wstatus ws;
	char buffer[64];
	
	ws = wchannel_load(load);
	if( ws != WSTATUS_SUCCESS ) {
		return;
	}

	wchannel_t wch;
	wchannel_opt_t wch_opt = {
		.type = WCHANNEL_TYPE_SOCKUDP,
		.host_src = "localhost",
		.port_src = "5000",
		.debug_opts = WCHANNEL_NO_DEBUG
	};

	ws = wchannel_create(&wch_opt,&wch);
	if( ws != WSTATUS_SUCCESS ) {
		return;
	}

	unsigned int msg_used;
	ws = wchannel_send(wch,"localhost 1234","HELLO",4,&msg_used);
	if( ws != WSTATUS_SUCCESS ) {
		goto return_wch;
	}

	ws = wchannel_receive(wch,buffer,sizeof(buffer),&msg_used);

return_wch:
	wchannel_destroy(wch);
	wchannel_unload();
	return;
}

int main(int argc,char *argv[])
{
	wstatus s;
	wview_load_t load;
	char buffer[32];

	wchannel_test();

	return EXIT_SUCCESS;

	wvctl_load_t wvctl_l;
	wvctl_l.exit_routine = 0;

	s = wvctl_load(wvctl_l);

	printf("wviewctl_load returned..");

	wvctl_register_mouse_cb(mouse_routine,(void*)123);
	wvctl_register_keyboard_cb(keyboard_routine,(void*)123);
	wvctl_register_draw_cb(draw_routine,(void*)456);
	wvctl_register_draw_cb(draw_routine2,(void*)456);

	fgets(buffer,sizeof(buffer),stdin);

	wvctl_unload();
	
	return EXIT_SUCCESS;

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

	v3d_t v;
	V3_SET(v,10,0,-800.0);
	wview_set(WVOPTION_TRANSLATE_VECTOR,&v,sizeof(v));

	v4d_t v4;
	V4_SET(v4,0,0,1.0,45.0);
	wview_set(WVOPTION_ROTATE_VECTOR,&v4,sizeof(v4));

	int rt_order = WVIEW_TRANSLATE_ROTATE;
	wview_set(WVOPTION_TR_ORDER,&rt_order,sizeof(rt_order));

	/* enter draw loop */
	wview_process(WVIEW_SYNCHRONOUS);

	/* close window */
	wview_destroy_window();

	/* unload wview */
	s = wview_unload();
	return EXIT_SUCCESS;
}

