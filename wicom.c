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
#include "modmgr.h"
#include "reqbuf.h"

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

void request_test(void)
{
	request_t req_text1,req_text2,req_text3,req_text4;
	request_t req_text1B,req_text2B,req_text3B,req_text4B;
	request_t req_bin1,req_bin2,req_bin3,req_bin4;
	request_t req_bin1T,req_bin2T,req_bin3T,req_bin4T;
//	char *req_raw = "123 modFrom modTo reqCode";
	char *req_raw1 = "123 modFrom modTo reqCode name1=value1 name2=\"value2 with spaces\" name3=value3";
	char *req_raw2 = "123 modFrom modTo reqCode";
	char *req_raw3 = "123 modFrom modTo reqCode name1=\"LALA LELE\" name2=VALUE2XXPTO name3";
	char *req_raw4 = "456 fromXpto toXtpo reqCODE nvpname1=#6566672A686970 name2=\"lala lele\"";

	struct _jmlist_init_params init = { .flags = 0, .fverbose = 0, .fdump = stdout, .fdebug = 0 };

	jmlist_initialize(&init);

	_req_from_string(req_raw1,&req_text1);
	_req_from_string(req_raw2,&req_text2);
	_req_from_string(req_raw3,&req_text3);
	_req_from_string(req_raw4,&req_text4);

	_req_to_bin(req_text1,&req_bin1);
	_req_to_text(req_bin1,&req_text1B);
	_req_to_bin(req_text1B,&req_bin1T);
	_req_diff(req_bin1,"req_bin1",req_bin1T,"req_bin1T");

	_req_to_bin(req_text2,&req_bin2);
	_req_to_text(req_bin2,&req_text2B);
	_req_to_bin(req_text2B,&req_bin2T);
	_req_diff(req_bin2,"req_bin2",req_bin2T,"req_bin2T");

	_req_to_bin(req_text3,&req_bin3);
	_req_dump(req_bin3);
	_req_to_text(req_bin3,&req_text3B);
	_req_dump(req_text3B);
	_req_to_bin(req_text3B,&req_bin3T);
	_req_diff(req_bin3,"req_bin3",req_bin3T,"req_bin3T");

	_req_to_bin(req_text4,&req_bin4);
	_req_dump(req_bin4);
	_req_to_text(req_bin4,&req_text4B);
	_req_dump(req_text4B);
	_req_to_bin(req_text4B,&req_bin4T);
	_req_diff(req_bin4,"req_bin4",req_bin4T,"req_bin4T");

	jmlist_uninitialize();
	return;
}

wstatus reqbuf_read1(void *param,void *buffer_ptr,unsigned int buffer_size,unsigned int *buffer_used)
{
	static int state = 0;
	char req_text1[] = {"123 modFrom modTo"};
	char req_text2[] = {" reqCode\0" "456 modFrom modTo reqCode"};
	char req_text3[] = {" name1=\"LALA LELE\" name2=VALUE2XXPTO name3\0"};

	switch(state)
	{
		case 0:
		memcpy(buffer_ptr,req_text1,sizeof(req_text1)-1);
		*buffer_used = sizeof(req_text1)-1;
		state++;
		break;
		case 1:
		memcpy(buffer_ptr,req_text2,sizeof(req_text2)-1);
		*buffer_used = sizeof(req_text2)-1;
		state++;
		break;
		case 2:
		memcpy(buffer_ptr,req_text3,sizeof(req_text3)-1);
		*buffer_used = sizeof(req_text3)-1;
		state++;
		break;
		default:
		*buffer_used = 0;
	}

	return WSTATUS_SUCCESS;
}

/* This read function breaks a request list of 3 requests into 4 chunks. */
#define REQREAD2_DIV 4
#define REQREAD2_CHUNK sizeof(struct _request_t)*3/REQREAD2_DIV
wstatus reqbuf_read2(void *param,void *buffer_ptr,unsigned int buffer_size,unsigned int *buffer_used)
{
	static int state = 0;

	if( state == (REQREAD2_DIV) ) {
		*buffer_used = 0;
		return WSTATUS_SUCCESS;
	}

	memcpy(buffer_ptr,(char*)param + state*REQREAD2_CHUNK,REQREAD2_CHUNK);
	*buffer_used = REQREAD2_CHUNK;

	/* if this is the last copy, check if we've remainder bytes to copy */
	if( ((sizeof(struct _request_t)*3) % REQREAD2_DIV) && (state == (REQREAD2_DIV-1)) )
	{
		memcpy((char*)buffer_ptr + REQREAD2_CHUNK,
				(char*)param + state*REQREAD2_CHUNK + REQREAD2_CHUNK,
				((sizeof(struct _request_t)*3) % REQREAD2_DIV) );
		*buffer_used += ((sizeof(struct _request_t)*3) % REQREAD2_DIV);
	}

	state++;
	return WSTATUS_SUCCESS;
}

void reqbuf_test(void)
{
	reqbuf_t rb;
	request_t req;
	request_t req_list;
	wstatus ws;
	char *req_raw1 = "123 modFrom modTo reqCode name1=value1 name2=\"value2 with spaces\" name3=value3";
	request_t req1T,req1B,req1R;
	char *req_raw2 = "123 modFrom modTo reqCode";
	request_t req2T,req2B,req2R;
	char *req_raw3 = "123 modFrom modTo reqCode name1=\"LALA LELE\" name2=VALUE2XXPTO name3";
	request_t req3T,req3B,req3R;

	ws = reqbuf_create(reqbuf_read1,0,REQBUF_TYPE_TEXT,&rb);
	ws = reqbuf_read(rb,&req);
	_req_dump(req);
	ws = reqbuf_read(rb,&req);
	_req_dump(req);
	ws = reqbuf_destroy(rb);

	/* create list of requests in binary stype */

	_req_from_string(req_raw1,&req1T);
	_req_to_bin(req1T,&req1B);
	_req_from_string(req_raw2,&req2T);
	_req_to_bin(req2T,&req2B);
	_req_from_string(req_raw3,&req3T);
	_req_to_bin(req3T,&req3B);

	req_list = (request_t)malloc(sizeof(struct _request_t)*3);
	memcpy(req_list,req1B,sizeof(struct _request_t));
	memcpy(req_list+1,req2B,sizeof(struct _request_t));
	memcpy(req_list+2,req3B,sizeof(struct _request_t));

	/* create reqbuf and test it */
	ws = reqbuf_create(reqbuf_read2,req_list,REQBUF_TYPE_BINARY,&rb);
	ws = reqbuf_read(rb,&req1R);
	ws = reqbuf_read(rb,&req2R);
	ws = reqbuf_read(rb,&req3R);

	_req_diff(req1B,"req1B",req1R,"req1R");
	_req_diff(req2B,"req2B",req2R,"req2R");
	_req_diff(req3B,"req3B",req3R,"req3R");

	/* TODO: TEST REQ LIST WITH TEXT STYPE'd REQUESTS */

	return;
}

int main(int argc,char *argv[])
{
	wstatus s;
	wview_load_t load;
	char buffer[32];

	//request_test();

	//wchannel_test();

	reqbuf_test();

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

