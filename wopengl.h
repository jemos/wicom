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

#ifndef _WOPENGL_H
#define _WOPENGL_H

#include <stdbool.h>
#include "status.h"
#include "geometry.h"

typedef struct _wgl_init_t {
	int junk;
	int argc;
	char *argv[];
} wgl_init_t;

typedef enum _wgl_ref_t {
	WGL_REFERENCIAL_ORTHO,
	WGL_REFERENCIAL_WORLD
} wgl_ref_t;

typedef enum _wgl_action_t {
	WGL_ACTION_PREDRAW,
	WGL_ACTION_POSTDRAW,
	WGL_ACTION_DRAW,
	WGL_ACTION_RESIZE
} wgl_action_t;

typedef enum _wgl_action_draw_t {
} wgl_action_draw_t;

typedef enum _wgl_action_resize_t {
	v2u_t size_current;
	v2u_t size_new;
} wgl_action_resize_t;

typedef struct _wgl_action_info_t {
	union {
		wgl_action_draw_t draw;
		wgl_action_resize_t resize;
	} data;
	wgl_action_t action;
} *wgl_action_info_t;

typedef wstatus (*WGLHOOKROUTINE)(wgl_action_info_t action_info, void *param);

typedef enum _wgl_line_format_t {
	WGL_LINE_DASHED,
	WGL_LINE_SOLID,
	WGL_LINE_DOTTED
} wgl_line_format_t;

wstatus wgl_initialize(wgl_init_t *wgl_init);
wstatus wgl_uninitialize(void);
wstatus wgl_draw_frame(void);
wstatus wgl_create_window(void);
wstatus wgl_main_loop(void);
wstatus wgl_cicle_events(void);
wstatus wgl_cursor_pos(v2d_t *pos,wgl_ref_t referential);
wstatus wgl_get_viewport_width(v1d_t *width);
wstatus wgl_get_viewport_height(v1d_t *height);
wstatus wgl_attach_hook(WGLHOOKROUTINE routine,void *param);
wstatus wgl_dettach_hook(WGLHOOKROUTINE routine);
wstatus wgl_draw_shape(wgl_shape_t shape,v1d_t z_coord,wgl_ref_t referential);
wstatus wgl_set_color(v3d_t color,v1d_t alpha);
wstatus wgl_toggle_blending(bool blend);
wstatus wgl_line_format(wgl_line_format_t line_format);

#endif

