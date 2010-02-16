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

/*
   Module Description

   wview interface implementation using freeglut library.
*/

#include "posh.h"

#include <GL/freeglut.h>
#include <stdbool.h>

#include "wstatus.h"
#include "debug.h"
#include "shape.h"
#include "wview.h"
#include "wviewcb.h"

#if (defined POSH_OS_WIN32 || defined POSH_OS_WIN64)
/* Windows Includes */
#endif

#if (defined POSH_OS_LINUX || defined POSH_OS_MACOSX)
/* Linux and MacOS X Includes */
#include <GL/freeglut.h>
#endif

/* these callbacks are set whenever the window is created,
   its the callbacks passed in wview_window_t structure. */
static wvkeyboard_cb cltkeyboard_cb = 0;
static wvmouse_cb cltmouse_cb = 0;
static wvdraw_cb cltdraw_cb = 0;

/* window identifier */
static int window_id = 0;

/* current window size variables */
static int win_width = 0;
static int win_height = 0;
static double win_depth = 100.0;

/* wview_load funcion called? */
static bool wview_loaded = false;

/* flag that is set whenever the client can draw shapes */
static bool accept_shapes = false;

/* some local functions declarations */
void wview_mouse_func(int button, int state, int x, int y);
void wview_keyboard_func(unsigned char key, int x, int y);
void wview_reshape_func(int width,int height);
void close_func(void);
void wview_display_func(void);

/*
   wview_load

   Load the module, in this case it calls glut initialization
   functions, where argc=0 and argv=0 are passed to glutInit
   because they are not supported in this implementation.
*/
wstatus
wview_load(wview_init_t init)
{
	dbgprint(MOD_WVIEW,__func__,"called");

	/* set this to false before doing anything.. */
	wview_loaded = false;

	/* call freeglut initializing functions */
	glutInit(0,0);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

	/* initialize window_id to null */
	window_id = 0;

	/* change loaded flag to true */
	wview_loaded = true;

	dbgprint(MOD_WVIEW,__func__,"Returning with success.");
	return WSTATUS_SUCCESS;
}

/*
   wview_unload

   Fails to unload if the window was not destroyed yet, after
   that free any allocated memory for the module.
*/
wstatus
wview_unload(void)
{
	dbgprint(MOD_WVIEW,__func__,"called");

	if( !wview_loaded )
	{
		dbgprint(MOD_WVIEW,__func__,"Cannot unload wview when it was not loaded yet");
		dbgprint(MOD_WVIEW,__func__,"Returning with failure.");
		return WSTATUS_FAILURE;
	}

	if( window_id )
	{
		dbgprint(MOD_WVIEW,__func__,"Cannot unload where there's still a window created");
		dbgprint(MOD_WVIEW,__func__,"Returning with failure.");
		return WSTATUS_FAILURE;
	}

	/* now its safe to unload... */
	wview_loaded = false;

	/* glut/freeglut don't have any uninitializing function ? */

	dbgprint(MOD_WVIEW,__func__,"Returning with success.");
	return WSTATUS_SUCCESS;
}

wstatus
wview_create_window(wview_window_t window)
{
	dbgprint(MOD_WVIEW,__func__,"called");

	if( !wview_loaded ) {
		dbgprint(MOD_WVIEW,__func__,"Cannot use this function before wview_load");
		dbgprint(MOD_WVIEW,__func__,"Returning with failure.");
		return WSTATUS_FAILURE;
	}

	/* validate window structure */
	
	if( !window.draw_routine ) {
		dbgprint(MOD_WVIEW,__func__,"Invalid draw routine pointer specified in window structure");
		dbgprint(MOD_WVIEW,__func__,"Returning with failure.");
		return WSTATUS_INVALID_ARGUMENT;
	}
	cltdraw_cb = window.draw_routine;
	dbgprint(MOD_WVIEW,__func__,"Set client draw routine pointer to %p",window.draw_routine);

	if( !window.keyboard_routine ) {
		dbgprint(MOD_WVIEW,__func__,"Invalid keyboard routine pointer specified in window structure");
		dbgprint(MOD_WVIEW,__func__,"Returning with failure.");
		return WSTATUS_INVALID_ARGUMENT;
	}
	cltkeyboard_cb = window.keyboard_routine;
	dbgprint(MOD_WVIEW,__func__,"Set client keyboard routine pointer to %p",window.keyboard_routine);

	if( !window.mouse_routine ) {
		dbgprint(MOD_WVIEW,__func__,"Invalid mouse routine pointer specified in window structure");
		dbgprint(MOD_WVIEW,__func__,"Returning with failure.");
		return WSTATUS_INVALID_ARGUMENT;
	}
	cltmouse_cb = window.mouse_routine;
	dbgprint(MOD_WVIEW,__func__,"Set client mouse routine pointer to %p",window.mouse_routine);

	window_id = glutCreateWindow("wicom");

	if( !window_id )
	{
		dbgprint(MOD_WVIEW,__func__,"Failed to create window (glutCreateWindow=0)");
		dbgprint(MOD_WVIEW,__func__,"Returning with failure.");
		return WSTATUS_FAILURE;
	}

	dbgprint(MOD_WVIEW,__func__,"Created window successfully (window id is %d)",window_id);

	dbgprint(MOD_WVIEW,__func__,"Setting window size to %u x %u",window.width,window.height);
	win_width = window.width;
	win_height = window.height;
	glutInitWindowSize(window.width,window.height);

	/* initialize viewport */
	dbgprint(MOD_WVIEW,__func__,"Initializing viewport...");

	glClearColor (0.0, 0.0, 0.0, 0.0);
	glOrtho(0.0f,win_width,win_height,0.0f,-1.0f,1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	/* initialize callbacks */
	dbgprint(MOD_WVIEW,__func__,"Setting up window callbacks");
	glutDisplayFunc(wview_display_func);
	glutReshapeFunc(wview_reshape_func);
	glutMouseFunc(wview_mouse_func);
	glutKeyboardFunc(wview_keyboard_func);

	dbgprint(MOD_WVIEW,__func__,"Returning with success.");
	return WSTATUS_SUCCESS;
}

/*
   wview_display_func

   Internal function called by freeglut library whenever a frame should be drawn.
   Redirect processing to the window callback set in wview_window_t structure.
*/
void wview_display_func(void)
{
	wvdraw_t draw;

	dbgprint(MOD_WVIEW,__func__,"called");

	if(!cltdraw_cb) {
		dbgprint(MOD_WVIEW,__func__,"cltdraw_cb is invalid (are you using multithreading?)!");
		dbgprint(MOD_WVIEW,__func__,"Finished.");
		return; 
	}

	/* setup OpenGL for a new draw */
	glLoadIdentity();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	/* start with perspective mode */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f,(GLfloat)win_width/(GLfloat)win_height,0.1f,win_depth);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	draw.flags = WVDRAW_FLAG_PERSPECTIVE;

	accept_shapes = true;
	cltdraw_cb(draw);
	accept_shapes = false;

	/* then orthogonal mode */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f,win_width,win_height,0.0f,-1.0f,1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	draw.flags = WVDRAW_FLAG_ORTHOGONAL;
	
	accept_shapes = true;
	cltdraw_cb(draw);
	accept_shapes = false;

	/* swap buffers in GL */
	glutSwapBuffers();
	dbgprint(MOD_WVIEW,__func__,"Finished.");
}

/*
   wview_reshape_func

   Internal function called by freeglut library whenever the window is resized.
   Process the necessary steps to do the window reshaping.
*/
void wview_reshape_func(int width,int height)
{
	dbgprint(MOD_WVIEW,__func__,"called with width=%u and height=%d",
			width,height);

	glViewport(0,0,width,height);

	dbgprint(MOD_WVIEW,__func__,"Updated viewport to (0,0) (%d,%d)",width,height);

	glMatrixMode(GL_PROJECTION);

	dbgprint(MOD_WVIEW,__func__,"Changed to projection matrix mode");

	glLoadIdentity();

	dbgprint(MOD_WVIEW,__func__,"Updating internal window size variables");

	win_width = width;
	win_height = height;

	dbgprint(MOD_WVIEW,__func__,"Updated internal window size variables successfully");

	dbgprint(MOD_WVIEW,__func__,"Calling glOrtho with new viewport size");

	glOrtho(0.0f,width,height,0.0f,-1.0f,1.0f);

	dbgprint(MOD_WVIEW,__func__,"Switching to model view matrix mode");

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	dbgprint(MOD_WVIEW,__func__,"Finished.");
	return;

}

/*
   wview_mouse_func

   Internal function called by freeglut whenever the user clicks on the window.
   Should call the client callback to process mouse input.
*/
void wview_mouse_func(int button,int state,int x,int y)
{
	wvmouse_t mouse;

	dbgprint(MOD_WVIEW,__func__,"called");

	if(!cltmouse_cb) {
		dbgprint(MOD_WVIEW,__func__,"cltmouse_cb is invalid (are you using multithreading?)!");
		dbgprint(MOD_WVIEW,__func__,"Finished.");
		return;
	}

	mouse.x = (unsigned int)x;
	mouse.y = (unsigned int)y;

	switch(button)
	{
		case GLUT_LEFT_BUTTON:
			dbgprint(MOD_WVIEW,__func__,"Identified button as the left button");
			mouse.button = WV_MOUSE_LEFT;
			break;
		case GLUT_RIGHT_BUTTON:
			dbgprint(MOD_WVIEW,__func__,"Identified button as the right button");
			mouse.button = WV_MOUSE_RIGHT;
			break;
		case GLUT_MIDDLE_BUTTON:
			dbgprint(MOD_WVIEW,__func__,"Identified button as the middle button");
			mouse.button = WV_MOUSE_MIDDLE;
			break;
		default:
			dbgprint(MOD_WVIEW,__func__,"Unable to process mouse button identifier (button=%d)",button);
			dbgprint(MOD_WVIEW,__func__,"Finished.");
			return;
	}

	switch(state)
	{
		case GLUT_UP:
			dbgprint(MOD_WVIEW,__func__,"Identified state as button up");
			mouse.mode = WV_MOUSE_UP;
			break;
		case GLUT_DOWN:
			dbgprint(MOD_WVIEW,__func__,"Identified state as button down");
			mouse.mode = WV_MOUSE_DOWN;
			break;
		default:
			dbgprint(MOD_WVIEW,__func__,"Unable to process mouse state identifier (state=%d)",state);
			dbgprint(MOD_WVIEW,__func__,"Finished.");
			return;
	}

	dbgprint(MOD_WVIEW,__func__,"Calling client mouse routine");
	cltmouse_cb(mouse);
	dbgprint(MOD_WVIEW,__func__,"Returned from the client mouse routine");

	dbgprint(MOD_WVIEW,__func__,"Finished.");
	return;
}

/*
   wview_keyboard_func

   Callback for glut keyboard function.
   "When a user types into the window, each key press generating an ASCII character will generate
   a keyboard callback. The key callback parameter is the generated ASCII character. The state of
   modifier keys such as Shift cannot be determined directly; their only effect will be on the 
   returned ASCII data. The x and y callback parameters indicate the mouse location in window 
   relative coordinates when the key was pressed." -- GLUT Reference.

   Since this function is called when the user presses the key, wview must simulate the key down
   and key up which will equal to a key press with the difference that there is no delay between.
*/
void wview_keyboard_func(unsigned char key,int x,int y)
{
	wvkey_t key_event;
	dbgprint(MOD_WVIEW,__func__,"called with key=0x%02X, x=%d, y=%d",(unsigned int)key,x,y);

	if(!cltkeyboard_cb) {
		dbgprint(MOD_WVIEW,__func__,"cltkeyboard_cb is invalid (are you using multithreading?)!");
		dbgprint(MOD_WVIEW,__func__,"Finished.");
		return;
	}

	dbgprint(MOD_WVIEW,__func__,"filling wvkey_t structure");
	key_event.key = key;
	key_event.x = x;
	key_event.y = y;
	key_event.flags = 0;

	/* call the client callback for the WV_KEY_DOWN */
	dbgprint(MOD_WVIEW,__func__,"Calling client keyboard routine with mode=WV_KEY_DOWN");
	cltkeyboard_cb(key_event,WV_KEY_DOWN);
	dbgprint(MOD_WVIEW,__func__,"Client routine returned");

	dbgprint(MOD_WVIEW,__func__,"filling wvkey_t structure");
	key_event.key = key;
	key_event.x = x;
	key_event.y = y;
	key_event.flags = 0;

	/* call the client callback for the WV_KEY_UP */
	dbgprint(MOD_WVIEW,__func__,"Calling client keyboard routine with mode=WV_KEY_UP");
	cltkeyboard_cb(key_event,WV_KEY_UP);
	dbgprint(MOD_WVIEW,__func__,"Client routine returned");

	dbgprint(MOD_WVIEW,__func__,"Finished.");
}

wstatus
wview_destroy_window(void)
{
	dbgprint(MOD_WVIEW,__func__,"called");

	if( !wview_loaded ) {
		dbgprint(MOD_WVIEW,__func__,"Cannot destroy window when wview was not loaded yet");
		dbgprint(MOD_WVIEW,__func__,"Returning with failure.");
		return WSTATUS_FAILURE;
	}

	if( !window_id ) {
		dbgprint(MOD_WVIEW,__func__,"Cannot destroy window because no window was created yet");
		dbgprint(MOD_WVIEW,__func__,"Returning with failure.");
		return WSTATUS_FAILURE;
	}

	/* call glut to destroy window */
	dbgprint(MOD_WVIEW,__func__,"Calling glutDestroyWindow with window_id=%d",window_id);
	glutDestroyWindow(window_id);

	/* check if window was destroyed successfully */
	if( glutGetWindow() != 0 )
	{
		/* maybe the code is creating more than one window so the current is not returning 0 */
		dbgprint(MOD_WVIEW,__func__,"Unable to destroy window (are you creating more than one window?)");
		dbgprint(MOD_WVIEW,__func__,"Returning with failure.");
		return WSTATUS_FAILURE;
	}

	dbgprint(MOD_WVIEW,__func__,"Window destroyed successfully");

	/* clear window_id */
	window_id = 0;

	dbgprint(MOD_WVIEW,__func__,"Returning with success.");
	return WSTATUS_SUCCESS;
}

/*
   wview_redraw

   Client code can ask wview to make a new frame, this will make wview call a GLUT function to
   trigger a new draw and that will make GLUT call wview displayFunc callback. That callback
   will then call a client routine that does the actual drawning using wview_draw_shape.
*/
wstatus
wview_redraw(void)
{
	dbgprint(MOD_WVIEW,__func__,"called");

	if( !wview_loaded ) {
		dbgprint(MOD_WVIEW,__func__,"Cannot use this function when wview was not loaded yet");
		dbgprint(MOD_WVIEW,__func__,"Returning with failure.");
		return WSTATUS_FAILURE;
	}

	if( !window_id ) {
		dbgprint(MOD_WVIEW,__func__,"Cannot use this function when no window was created yet");
		dbgprint(MOD_WVIEW,__func__,"Returning with failure.");
		return WSTATUS_FAILURE;
	}

	dbgprint(MOD_WVIEW,__func__,"Calling glutPostRedisplay function");
	glutPostRedisplay();
	dbgprint(MOD_WVIEW,__func__,"glutPostRedisplay returned");

	dbgprint(MOD_WVIEW,__func__,"Returning with success.");
	return WSTATUS_SUCCESS;
}

void draw_point(shape_t shape)
{
	return;
}

/*
   wview_draw_shape

   This should be the most complex function in wview, draw the shape in shape_t structure
   using OpenGL API. The supported shape types are point, line, polyline, polygon and text.
*/
wstatus
wview_draw_shape(shape_t shape)
{
	dbgprint(MOD_WVIEW,__func__,"called with shape.type=%d",shape.type);

	if( !accept_shapes ) {
		dbgprint(MOD_WVIEW,__func__,"Cannot use this function outside client redraw callback");
		dbgprint(MOD_WVIEW,__func__,"Returning with failure.");
		return WSTATUS_FAILURE;
	}

	switch(shape.type)
	{
		case SHAPE_POINT:
			draw_point(shape);
			break;
		case SHAPE_LINE:
			break;
		case SHAPE_POLYLINE:
			break;
		case SHAPE_POLYGON:
			break;
		case SHAPE_TEXT:
			break;
		default:
			dbgprint(MOD_WVIEW,__func__,"Invalid or unsupported shape type (%d)",shape.type);
			dbgprint(MOD_WVIEW,__func__,"Returning with failure.");
			return WSTATUS_FAILURE;
	}

	/* shouldn't get here, in any case...return failure. */
	return WSTATUS_FAILURE;
}

