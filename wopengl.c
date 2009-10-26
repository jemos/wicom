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

#include <stdbool.h>
#include "wstatus.h"
#include "wopengl.h"
#include "geometry.h"

static v2d_t cursor_pos;
static v2u_t viewport = { .x = DEFAULT_VIEWPORT_WIDTH, .y = DEFAULT_VIEWPORT_HEIGHT};
static bool init_ok = false;
static bool window_ok = false;

/*
   wgl_initialize

   This function is required to call glut initialization functions,
   we're using GLUT_DOUBLE for double buffering draw, and GLUT_RGB,
   we're gonna use colors. The viewport size is initialized to the
   size specified in wgl_init or, if not set, default of 800x600
   is used.
*/
wstatus
wgl_initialize(wgl_init_t *wgl_init)
{
	if(!wgl_init)
	{
		dbgprint(MOD_WOPENGL,__func__,"invalid wgl_init specified! (wgl_init=0)");
		return STATUS_INVALID_ARGUMENT;
	}

	/* call glut initializing functions */
	dbgprint(MOD_WOPENGL,__func__,"calling glutInit...");
	glutInit(&wgl->init.argc,argv);
	dbgprint(MOD_WOPENGL,__func__,"calling glutInitDisplayMode...");
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

	/* reset cursor position */
	dbgprint(MOD_WOPENGL,__func__,"reseting cursor_pos to (0,0)");
	cursor_pos.x = 0;
	cursor_pos.y = 0;

	/* initialize viewport size */
	if( (wgl_init->viewport.x) && (wgl_init->viewport.y) )
		viewport = wgl_init->viewport;

	dbgprint(MOD_WOPENGL,__func__,"current viewport size is (%d,%d)",
			viewport.x,viewport.y);

	dbgprint(MOD_WOPENGL,__func__,"updating init flag to TRUE");
	init_ok = true;

	dbgprint(MOD_WOPENGL,__func__,"returning with success.");
	return STATUS_SUCCESS;
}

/*
   wgl_uninitialize

   Free any used memory, also uninitialize glut library.
*/
wstatus
wgl_uninitialize(void)
{
	dbgprint(MOD_WOPENGL,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   wgl_draw_frame

   Callback function for glut library to draw a single frame.
   Client code should not call this function directly!
*/
wstatus wgl_draw_frame(void)
{
	dbgprint(MOD_WOPENGL,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   wgl_create_window

   Helper function to create the OpenGL output window.
*/
wstatus wgl_create_window(void)
{
	/* check for module initialization */
	if( !init_ok )
	{
		dbgprint(MOD_WOPENGL,__func__,"wopengl must be initialized before you can use this function!");
		return STATUS_UNINITIALIZED;
	}

	/* configure window settings */
	dbgprint(MOD_WOPENGL,__func__,"setting window size to width %u and height %u",
			viewport.x,viewport.y);

	glutInitWindowSize(viewport.x,viewport.y);
	
	dbgprint(MOD_WOPENGL,__func__,"setting window position to (0,0)");
	glutInitWindowPosition(0,0);

	dbgprint(MOD_WOPENGL,__func__,"calling viewport initialization function...");
	wgl_viewport_init();

	dbgprint(MOD_WOPENGL,__func__,"setting glut callback functions...");
	/* call glut functions to initialize some callbacks */
	glutDisplayFunc(iwgl_draw_frame);
	glutReshapeFunc(iwgl_resize_window);
	glutMouseFunc(iwgl_mouse_button_event);
	glutKeyboardFunc(iwgl_keyboard_event);
	glutSpecialFunc(iwgl_special_key_event);
	glutPassiveMotionFunc(iwgl_mouse_move_event);

	/* update window_ok flag */
	dbgprint(MOD_WOPENGL,__func__,"Updating window_ok flag to TRUE");
	window_ok = true;

	dbgprint(MOD_WOPENGL,__func__,"returning with success.");
	return STATUS_SUCCESS;
}

/*
   wgl_main_loop

   If the console thread doesn't need to receive user input, it
   can call this function and will only return when the user closes
   the OpenGL window or exits using OpenGL output window console
   command line ('exit').

   If the main thread requires also to receive user input, you
   can call wgl_cicle_events() whenever possible to make OpenGL
   draw the window.
*/
wstatus wgl_main_loop(void)
{
	dbgprint(MOD_WOPENGL,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   wgl_cicle_events

   This function will process OpenGL window events (drawing,
   keyboard events, mouse events) and should be called whenever
   possible if you're not using wgl_main_loop (which will do
   the processing all the time).

   The use of this function permits to do OpenGL processing
   only when you know something was changed in OpenGL window,
   for example when you added something that must be draw.
   It will spare processor use since OpenGL is not always
   drawing the window but only when its requested to.
*/
wstatus wgl_cicle_events(void)
{
	dbgprint(MOD_WOPENGL,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   wgl_cursor_pos

   Provides the cursor position, there should be two referentials
   available, the ortho (i.e.: window screen coordinates) and the
   world coordinates which are dependent on the current scale and
   displacement of the map.
*/
wstatus wgl_cursor_pos(v2d_t *pos,wgl_ref_t referential)
{
	dbgprint(MOD_WOPENGL,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   wgl_get_viewport_width

   Self explanatory.
*/
wstatus wgl_get_viewport_width(v1d_t *width)
{
	dbgprint(MOD_WOPENGL,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   wgl_get_viewport_height

   Self explanatory.
*/
wstatus wgl_get_viewport_height(v1d_t *height)
{
	dbgprint(MOD_WOPENGL,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   wgl_attach_hook

   Attaches an hook to wopengl module, this module will call the hook
   whenever the action is triggered. There are various actions available
   the best is to look at wopengl.h for a list and description.
*/
wstatus wgl_attach_hook(WGLHOOKROUTINE routine,void *param)
{
	dbgprint(MOD_WOPENGL,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   wgl_dettach_hook

   Dettaches an hook which was previously attached.
*/
wstatus wgl_dettach_hook(WGLHOOKROUTINE routine)
{
	dbgprint(MOD_WOPENGL,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   wgl_draw_shape

   Draws a primitive (or not?) shape on the world. With the referential
   argument its select whether is to be draw in ortho mode or in the world
   coordinates.
*/
wstatus wgl_draw_shape(wgl_shape_t shape,v1d_t z_coord,wgl_ref_t referential)
{
	dbgprint(MOD_WOPENGL,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   wgl_set_color

   Set the current color, this should be called before drawing a shape.
*/
wstatus wgl_set_color(v3d_t color,v1d_t alpha)
{
	dbgprint(MOD_WOPENGL,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   wgl_toggle_blending

   Blending will allow transparent shapes to be drawn.
*/
wstatus wgl_toggle_blending(bool blend)
{
	dbgprint(MOD_WOPENGL,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   wgl_line_format

   Changes line format, there are various formats available, check wopengl.h
   for more details.
*/
wstatus wgl_line_format(wgl_line_format_t line_format)
{
	dbgprint(MOD_WOPENGL,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   wgl_viewport_init

   Initialize the viewport of OpenGL, this will set the background color
   to black, and visualization mode to ortho.
*/
wstatus wgl_viewport_init(void)
{
	dbgprint(MOD_WOPENGL,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

