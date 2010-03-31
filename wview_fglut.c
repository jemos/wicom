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

References:
	[1] http://www.opengl.org/resources/faq/technical/selection.htm
*/


#include <GL/freeglut.h>
#include <stdbool.h>
#include <string.h>

#include "posh.h"
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
static double win_depth = 1000.0;

/* wview_load funcion called? */
static bool wview_loaded = false;

/* flag that is set whenever the client can draw shapes */
static bool accept_shapes = false;

/* local options */
static v3d_t opt_translate_vector;
static v4d_t opt_rotate_vector;
static int opt_tr_order;

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
wview_load(wview_load_t load)
{
	int argc = 1;
	char *argv = "wicom";

	dbgprint(MOD_WVIEW,__func__,"called");

	/* set this to false before doing anything.. */
	wview_loaded = false;

	/* call freeglut initializing functions */
	glutInit(&argc,(char**)&argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

	/* initialize window_id to null */
	window_id = 0;

	/* set some glut options */
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE,GLUT_ACTION_GLUTMAINLOOP_RETURNS);

	/* initialize options with default values */
	V3_SET(opt_translate_vector,0,0,0);
	V4_SET(opt_rotate_vector,0,0,0,0);
	opt_tr_order = WVIEW_ROTATE_TRANSLATE;

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

	/* set window size before creating it */
	dbgprint(MOD_WVIEW,__func__,"Setting window size to %u x %u",window.width,window.height);
	win_width = window.width;
	win_height = window.height;
	glutInitWindowSize(window.width,window.height);

	/* create the window */
	window_id = glutCreateWindow("wicom");

	if( !window_id )
	{
		dbgprint(MOD_WVIEW,__func__,"Failed to create window (glutCreateWindow=0)");
		dbgprint(MOD_WVIEW,__func__,"Returning with failure.");
		return WSTATUS_FAILURE;
	}

	dbgprint(MOD_WVIEW,__func__,"Created window successfully (window id is %d)",window_id);

	/* initialize viewport */
	dbgprint(MOD_WVIEW,__func__,"Initializing viewport...");

	glClearColor(0.0, 0.0, 0.0, 0.5);
	glShadeModel(GL_SMOOTH);
	glClearDepth(1.0);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_COLOR_MATERIAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);

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


	/* start with perspective mode */
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f,(GLfloat)win_width/(GLfloat)win_height,0.1f,win_depth);
	glMatrixMode(GL_MODELVIEW);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);

	if( opt_tr_order == WVIEW_TRANSLATE_ROTATE ) {

		/* apply translate vector first */
		glTranslated(opt_translate_vector.x,
				opt_translate_vector.y,
				opt_translate_vector.z);

		/* apply rotate vector second */
		glRotated(opt_rotate_vector.t,
				opt_rotate_vector.x,
				opt_rotate_vector.y,
				opt_rotate_vector.z);
	} else
	{
		/* otherwise and default is to rotate first */

		/* apply rotate vector second */
		glRotated(opt_rotate_vector.t,
				opt_rotate_vector.x,
				opt_rotate_vector.y,
				opt_rotate_vector.z);

		/* apply translate vector first */
		glTranslated(opt_translate_vector.x,
				opt_translate_vector.y,
				opt_translate_vector.z);
	}


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

	gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.1f,100.0f);

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
	cltmouse_cb(mouse,0);
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
	cltkeyboard_cb(key_event,WV_KEY_DOWN,0);
	dbgprint(MOD_WVIEW,__func__,"Client routine returned");

	dbgprint(MOD_WVIEW,__func__,"filling wvkey_t structure");
	key_event.key = key;
	key_event.x = x;
	key_event.y = y;
	key_event.flags = 0;

	/* call the client callback for the WV_KEY_UP */
	dbgprint(MOD_WVIEW,__func__,"Calling client keyboard routine with mode=WV_KEY_UP");
	cltkeyboard_cb(key_event,WV_KEY_UP,0);
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

	/* In this specific case I'll not be intrusive to demand the client to know if the
	   window was destroyed or not before calling this function. So if it was destroyed
	   already, this function will simply return success. */

//	if( !window_id ) {
//		dbgprint(MOD_WVIEW,__func__,"Cannot destroy window because no window was created yet");
//		dbgprint(MOD_WVIEW,__func__,"Returning with failure.");
//		return WSTATUS_FAILURE;
//	}

	if( !window_id ) {
		dbgprint(MOD_WVIEW,__func__,"There is no window to destroy");
		dbgprint(MOD_WVIEW,__func__,"Returning with success.");
		return WSTATUS_SUCCESS;
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

/*
   draw_point

   Helper function of wview_draw_shape to draw a single point.
   TODO Point size is ignored for now.
*/
wstatus
draw_point(shape_t shape)
{
	dbgprint(MOD_WVIEW,__func__,"called");

	/* set point color */
	glColor3d(shape.data.point.color.x,
			shape.data.point.color.y,
			shape.data.point.color.z );

	dbgprint(MOD_WVIEW,__func__,"point color is (%0.2g,%0.2g,%0.2g) location is (%0.2g,%0.2g,%0.2g)",
			shape.data.point.color.x,
			shape.data.point.color.y,
			shape.data.point.color.z,
			shape.data.point.p1.x,
			shape.data.point.p1.y,
			shape.data.point.p1.z );

	/* draw the point */
	glBegin(GL_POINTS);
	glVertex3d(shape.data.point.p1.x,shape.data.point.p1.y,shape.data.point.p1.z);
	glEnd();

	dbgprint(MOD_WVIEW,__func__,"Returning with success.");
	return WSTATUS_SUCCESS;
}

/*
   draw_line

   Helper function of wview_draw_shape to draw a single line.
*/
wstatus
draw_line(shape_t shape)
{
	dbgprint(MOD_WVIEW,__func__,"called");

	/* set line color */
	glColor3d(shape.data.line.color.x,
			shape.data.line.color.y,
			shape.data.line.color.z);

	dbgprint(MOD_WVIEW,__func__,"line color is (%0.2g,%0.2g,%0.2g) vertexes are (%0.2g,%0.2g,%0.2g) to (%0.2g,%0.2g,%0.2g)",
			shape.data.line.color.x,shape.data.line.color.y,shape.data.line.color.z,
			shape.data.line.p1.x,shape.data.line.p1.y,shape.data.line.p1.z,
			shape.data.line.p2.x,shape.data.line.p2.y,shape.data.line.p2.z );

	/* draw the line */
	glBegin(GL_LINES);
	glVertex3d(shape.data.line.p1.x,shape.data.line.p1.y,shape.data.line.p1.z);
	glVertex3d(shape.data.line.p2.x,shape.data.line.p2.y,shape.data.line.p2.z);
	glEnd();

	dbgprint(MOD_WVIEW,__func__,"Returning with success.");
	return WSTATUS_SUCCESS;
}

/*
   draw_polyline

   Helper function of wview_draw_shape to draw a polyline.
   The polyline vertexes are passed in a vector (point_list).
   Make sure that point_count is set OK.
*/
wstatus
draw_polyline(shape_t shape)
{
	dbgprint(MOD_WVIEW,__func__,"called");

	/* set the line color */
	glColor3d(shape.data.polyline.color.x,
			shape.data.polyline.color.y,
			shape.data.polyline.color.z);

	dbgprint(MOD_WVIEW,__func__,"line color is (%0.2g,%0.2g,%0.2g)");

	dbgprint(MOD_WVIEW,__func__,"inserting %u vertexes",shape.data.polyline.point_count);

	/* draw the polyline */
	glBegin(GL_LINE_STRIP);
	for( unsigned int i = 0 ; i < shape.data.polyline.point_count ; i++ )
	{
		dbgprint(MOD_WVIEW,__func__,"vertex #%u is at (%0.2g,%0.2g,%0.2g)",i,
				shape.data.polyline.point_list[i].x,
				shape.data.polyline.point_list[i].y,
				shape.data.polyline.point_list[i].z );

		glVertex3d(shape.data.polyline.point_list[i].x,
				shape.data.polyline.point_list[i].y,
				shape.data.polyline.point_list[i].z);
	}
	glEnd();

	dbgprint(MOD_WVIEW,__func__,"Returning with success.");
	return WSTATUS_SUCCESS;
}

wstatus
draw_polygon(shape_t shape)
{
	dbgprint(MOD_WVIEW,__func__,"called");

	/* get the line color */
	glColor3d(shape.data.polygon.color.x,
			shape.data.polygon.color.y,
			shape.data.polygon.color.z );

	dbgprint(MOD_WVIEW,__func__,"polygon color is (%0.2g, %0.2g, %0.2g)");

	dbgprint(MOD_WVIEW,__func__,"inserting %u vertexes",shape.data.polygon.point_count);

	/* draw the polygon */
	glBegin(GL_POLYGON);
	for( unsigned int i = 0 ; i < shape.data.polygon.point_count ; i++ )
	{
		dbgprint(MOD_WVIEW,__func__,"vertex #%u is at (%0.2g,%0.2g,%0.2g)",i,
				shape.data.polygon.point_list[i].x,
				shape.data.polygon.point_list[i].y,
				shape.data.polygon.point_list[i].z );

		glVertex3d(shape.data.polygon.point_list[i].x,
				shape.data.polygon.point_list[i].y,
				shape.data.polygon.point_list[i].z);
	}
	glEnd();

	dbgprint(MOD_WVIEW,__func__,"Returning with success.");
	return WSTATUS_SUCCESS;
}

wstatus
draw_text(shape_t shape)
{
	dbgprint(MOD_WVIEW,__func__,"Called");

	if( !strlen(shape.data.text.content) ) {
		dbgprint(MOD_WVIEW,__func__,"Cannot draw empty line of text (strlen(content) = 0)");
		DBGRET_FAILURE(MOD_WVIEW)
	}

	glColor3d(shape.data.text.color.x,
			shape.data.text.color.y,
			shape.data.text.color.z);

	glRasterPos3d(shape.data.text.position.x,
			shape.data.text.position.y,
			shape.data.text.position.z);

	void *font;
	switch(shape.data.text.font)
	{
		case TEXT_FONT_SMALL:
			dbgprint(MOD_WVIEW,__func__,"Using small font size for the text");
			font = GLUT_BITMAP_8_BY_13;
			break;
		case TEXT_FONT_NORMAL:
			dbgprint(MOD_WVIEW,__func__,"Using normal font size for the text");
			font = GLUT_BITMAP_HELVETICA_10;
			break;
		default:
			dbgprint(MOD_WVIEW,__func__,"Invalid or unsupported font (%d)",shape.data.text.font);
			DBGRET_FAILURE(MOD_WVIEW)
	}

	dbgprint(MOD_WVIEW,__func__,"Printing text using GLUT, content is: %s",shape.data.text.content);

	for( unsigned int i = 0 ; i < strlen(shape.data.text.content) ; i++ )
		glutBitmapCharacter(font,shape.data.text.content[i]);

	DBGRET_SUCCESS(MOD_WVIEW)
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
			return draw_point(shape);
		case SHAPE_LINE:
			return draw_line(shape);
		case SHAPE_POLYLINE:
			return draw_polyline(shape);
		case SHAPE_POLYGON:
			return draw_polygon(shape);
		case SHAPE_TEXT:
			return draw_text(shape);
		default:
			dbgprint(MOD_WVIEW,__func__,"Invalid or unsupported shape type (%d)",shape.type);
			dbgprint(MOD_WVIEW,__func__,"Returning with failure.");
			return WSTATUS_FAILURE;
	}

	/* shouldn't get here, in any case...return failure. */
	return WSTATUS_FAILURE;
}


wstatus
wview_process(wview_mode_t mode)
{
	if( (mode == WVIEW_ASYNCHRONOUS) && DEBUG_LOOPS )
	dbgprint(MOD_WVIEW,__func__,"Called with mode=%d",mode);

	switch(mode)
	{
		case WVIEW_SYNCHRONOUS:
			glutMainLoop();
			dbgprint(MOD_WVIEW,__func__,"GLUT main loop returned, reseting window_id...");
			window_id = 0;
			break;
		case WVIEW_ASYNCHRONOUS:
			glutMainLoopEvent();
			break;
		default:
			dbgprint(MOD_WVIEW,__func__,"Invalid or unsupported mode (%d)",mode);
			dbgprint(MOD_WVIEW,__func__,"Returning with failure.");
			return WSTATUS_FAILURE;
	}

	if( (mode == WVIEW_ASYNCHRONOUS) && DEBUG_LOOPS )
	dbgprint(MOD_WVIEW,__func__,"Returning with success.");

	return WSTATUS_SUCCESS;
}

wstatus
wview_set(wview_option_t option,void *value_ptr,unsigned int value_size)
{
	dbgprint(MOD_WVIEW,__func__,"Called with option=%d value_ptr=%p value_size=%u",option,value_ptr,value_size);

	switch(option)
	{
		case WVOPTION_TRANSLATE_VECTOR:
			
			/* value_size is expected to be sizeof(v3d) */
			if( value_size != sizeof(v3d_t) ) {
				dbgprint(MOD_WVIEW,__func__,"value_size is not the expected (requires %u and passed was %d)",
						sizeof(v3d_t),value_size);
				DBGRET_FAILURE(MOD_WVIEW)
			}

			/* validate value_ptr, we don't wanna read from void */
			if( !value_ptr ) {
				dbgprint(MOD_WVIEW,__func__,"value_ptr is invalid (value_ptr=0)");
				DBGRET_FAILURE(MOD_WVIEW)
			}

			/* set translation vector */
			v3d_t *new_v3 = (v3d_t*)value_ptr;
			dbgprint(MOD_WVIEW,__func__,"Setting translation vector from " V3DPRINTF " to " V3DPRINTF,
					opt_translate_vector.x,opt_translate_vector.y,opt_translate_vector.z,
					new_v3->x, new_v3->y, new_v3->z);
			V3_SET(opt_translate_vector,new_v3->x,new_v3->y,new_v3->z)

			dbgprint(MOD_WVIEW,__func__,"New translate vector is " V3DPRINTF,
					opt_translate_vector.x,opt_translate_vector.y,opt_translate_vector.z );
			break;

		case WVOPTION_ROTATE_VECTOR:

			/* value_size is expected to be sizeof(v4d) */
			if( value_size != sizeof(v4d_t) ) {
				dbgprint(MOD_WVIEW,__func__,"value_size is not the expected (requires %u and passed was %d)",
						sizeof(v4d_t),value_size);
				DBGRET_FAILURE(MOD_WVIEW)
			}

			/* validate value_ptr, we don't wanna read from void */
			if( !value_ptr ) {
				dbgprint(MOD_WVIEW,__func__,"value_ptr is invalid (value_ptr=0)");
				DBGRET_FAILURE(MOD_WVIEW)
			}

			/* set rotation vector */
			v4d_t *new_v4 = (v4d_t*)value_ptr;
			dbgprint(MOD_WVIEW,__func__,"Setting rotation vector from " V4DPRINTF " to " V4DPRINTF,
					opt_rotate_vector.x,opt_rotate_vector.y,opt_rotate_vector.z,opt_rotate_vector.t,
					new_v4->x, new_v4->y, new_v4->z, new_v4->t);
			V4_SET(opt_rotate_vector,new_v4->x,new_v4->y,new_v4->z,new_v4->t)
			break;
		
		case WVOPTION_TR_ORDER:
		
			/* value_size is expected to be sizeof(bool) */	
			if( value_size != sizeof(int) ) {
				dbgprint(MOD_WVIEW,__func__,"value_size is not the expected (requires %u and passed was %d)",
						sizeof(int),value_size);
				DBGRET_FAILURE(MOD_WVIEW)
			}

			/* validate value_ptr, we don't wanna read from void */
			if( !value_ptr ) {
				dbgprint(MOD_WVIEW,__func__,"value_ptr is invalid (value_ptr=0)");
				DBGRET_FAILURE(MOD_WVIEW)
			}

			/* set translate/rotate order */
			int *new_tr = (int*)value_ptr;

			/* test the possible values */
			switch(*new_tr)
			{
				case WVIEW_TRANSLATE_ROTATE:
					dbgprint(MOD_WVIEW,__func__,"Order detected as translate first then rotate");
					break;
				case WVIEW_ROTATE_TRANSLATE:
					dbgprint(MOD_WVIEW,__func__,"Order detected as rotate first then translate");
					break;
				default:
					dbgprint(MOD_WVIEW,__func__,"Invalid or unsupported value (%d)",*new_tr);
					DBGRET_FAILURE(MOD_WVIEW)
			}

			dbgprint(MOD_WVIEW,__func__,"Setting translate/rotate order from %d to %d",opt_tr_order,*new_tr);
			opt_tr_order = *new_tr;	
			break;

		default:
			dbgprint(MOD_WVIEW,__func__,"Invalid or unsupported option (%d)",option);
			DBGRET_FAILURE(MOD_WVIEW);
	}

	DBGRET_SUCCESS(MOD_WVIEW)
}

