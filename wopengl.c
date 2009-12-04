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

#ifdef WIN32
#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include <glut.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#endif

#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "wstatus.h"
#include "wopengl.h"
#include "jmlist.h"
#include "geometry.h"
#include "debug.h"

#define GL_PI 3.1415f

static v2d_t cursor_pos;
static v2u_t viewport = { .x = DEFAULT_VIEWPORT_WIDTH, 
	.y = DEFAULT_VIEWPORT_HEIGHT};
static bool init_ok = false;
static bool window_ok = false;
static jmlist hooklist;

/* internal functions to wopengl module */
GLvoid iwgl_draw_frame(GLvoid);
GLvoid iwgl_resize_window(GLsizei width, GLsizei height);
wstatus iwgl_viewport_init(void);

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
		return WSTATUS_INVALID_ARGUMENT;
	}

	/* call glut initializing functions */
	dbgprint(MOD_WOPENGL,__func__,"calling glutInit...");

	glutInit(&wgl_init->argc,wgl_init->argv);
	
	dbgprint(MOD_WOPENGL,__func__,"calling glutInitDisplayMode...");
	
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

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
	return WSTATUS_SUCCESS;
}

/*
   wgl_uninitialize

   Free any used memory, also uninitialize glut library.
*/
wstatus
wgl_uninitialize(void)
{
	dbgprint(MOD_WOPENGL,__func__,"unimplemented function called!");
	return WSTATUS_UNIMPLEMENTED;
}

/*
   wgl_draw_frame

   Callback function for glut library to draw a single frame.
   Client code should not call this function directly!
*/
wstatus wgl_draw_frame(void)
{
	dbgprint(MOD_WOPENGL,__func__,"unimplemented function called!");
	return WSTATUS_UNIMPLEMENTED;
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
		return WSTATUS_MOD_UNINITIALIZED;
	}

	/* configure window settings */
	dbgprint(MOD_WOPENGL,__func__,"setting window size to width %u and height %u",
			viewport.x,viewport.y);

	glutInitWindowSize(viewport.x,viewport.y);
	
	dbgprint(MOD_WOPENGL,__func__,"setting window position to (0,0)");
	
	glutInitWindowPosition(0,0);

	dbgprint(MOD_WOPENGL,__func__,"creating window");

	glutCreateWindow("wicom");

	dbgprint(MOD_WOPENGL,__func__,"calling viewport initialization function...");

	iwgl_viewport_init();

	dbgprint(MOD_WOPENGL,__func__,"setting glut callback functions...");
	/* call glut functions to initialize some callbacks */
	glutDisplayFunc(iwgl_draw_frame);
	glutReshapeFunc(iwgl_resize_window);
//	glutMouseFunc(iwgl_mouse_button_event);
//	glutKeyboardFunc(iwgl_keyboard_event);
//	glutSpecialFunc(iwgl_special_key_event);
//	glutPassiveMotionFunc(iwgl_mouse_move_event);

	/* update window_ok flag */
	dbgprint(MOD_WOPENGL,__func__,"Updating window_ok flag to TRUE");
	window_ok = true;

	dbgprint(MOD_WOPENGL,__func__,"returning with success.");
	return WSTATUS_SUCCESS;
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

   In future this module should use freeglut and that will provide
   glutMainLoopEvent which DOES return when the user closes the
   window.
*/
wstatus wgl_main_loop(void)
{
	dbgprint(MOD_WOPENGL,__func__,"called");

	if( !window_ok )
	{
		dbgprint(MOD_WOPENGL,__func__,"window was not created yet!");
		dbgprint(MOD_WOPENGL,__func__,"returning with failure.");
		return WSTATUS_FAILURE;
	}

	dbgprint(MOD_WOPENGL,__func__,"passing control to glutMainLoop...");
	//glutMainLoopEvent();
	glutMainLoop();

	dbgprint(MOD_WOPENGL,__func__,"returning with success.");
	return WSTATUS_SUCCESS;
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
	dbgprint(MOD_WOPENGL,__func__,"unsupported yet");
	return WSTATUS_UNSUPPORTED;

	dbgprint(MOD_WOPENGL,__func__,"called");

	if( !window_ok )
	{
		dbgprint(MOD_WOPENGL,__func__,"window was not created yet!");
		dbgprint(MOD_WOPENGL,__func__,"returning with failure.");
		return WSTATUS_FAILURE;
	}

	dbgprint(MOD_WOPENGL,__func__,"calling glutMainLoopEvent");
	//glutMainLoop();
	dbgprint(MOD_WOPENGL,__func__,"function glutMainLoopEvent returned");

	dbgprint(MOD_WOPENGL,__func__,"returning with success.");
	return WSTATUS_SUCCESS;
}

/*
   wgl_mouse_pos

   Provides the cursor position, there should be two referentials
   available, the ortho (i.e.: window screen coordinates) and the
   world coordinates which are dependent on the current scale and
   displacement of the map.
*/
wstatus wgl_mouse_pos(v2d_t *pos,wgl_ref_t referential)
{
	dbgprint(MOD_WOPENGL,__func__,"called with pos = (%d,%d) and referential=%u",
			pos->x,pos->y,referential);

	if(!window_ok)
	{
		dbgprint(MOD_WOPENGL,__func__,"window was not created yet!");
		dbgprint(MOD_WOPENGL,__func__,"returning with failure.");
		return WSTATUS_FAILURE;
	}
	
	switch(referential)
	{
		case WGL_REFERENCIAL_ORTHO:
			dbgprint(MOD_WOPENGL,__func__,"updating pos argument");
			memcpy(pos,&cursor_pos,sizeof(cursor_pos));
			break;
		case WGL_REFERENCIAL_WORLD:
		    /* TODO finish this function... */
			dbgprint(MOD_WOPENGL,__func__,"converting mouse coordinates from ortho to world");
//			memcpy(pos,&xpto,sizeof(v2d_t));
			break;
		default:
			dbgprint(MOD_WOPENGL,__func__,"invalid or unsupported referential specified! (%u)",referential);
			dbgprint(MOD_WOPENGL,__func__,"returning with failure.");
			return WSTATUS_FAILURE;
	}

	dbgprint(MOD_WOPENGL,__func__,"returning with success.");
	return WSTATUS_SUCCESS;
}

/*
   wgl_get_viewport_width

   Self explanatory.
*/
wstatus wgl_get_viewport_width(v1d_t *width)
{
	dbgprint(MOD_WOPENGL,__func__,"unimplemented function called!");
	return WSTATUS_UNIMPLEMENTED;
}

/*
   wgl_get_viewport_height

   Self explanatory.
*/
wstatus wgl_get_viewport_height(v1d_t *height)
{
	dbgprint(MOD_WOPENGL,__func__,"unimplemented function called!");
	return WSTATUS_UNIMPLEMENTED;
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
	return WSTATUS_UNIMPLEMENTED;
}

/*
   wgl_dettach_hook

   Dettaches an hook which was previously attached.
*/
wstatus wgl_dettach_hook(WGLHOOKROUTINE routine)
{
	dbgprint(MOD_WOPENGL,__func__,"unimplemented function called!");
	return WSTATUS_UNIMPLEMENTED;
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
	return WSTATUS_UNIMPLEMENTED;
}

/*
   wgl_set_color

   Set the current color, this should be called before drawing a shape.
*/
wstatus wgl_set_color(v3d_t color,v1d_t alpha)
{
	dbgprint(MOD_WOPENGL,__func__,"unimplemented function called!");
	return WSTATUS_UNIMPLEMENTED;
}

/*
   wgl_toggle_blending

   Blending will allow transparent shapes to be drawn.
*/
wstatus wgl_toggle_blending(bool blend)
{
	dbgprint(MOD_WOPENGL,__func__,"unimplemented function called!");
	return WSTATUS_UNIMPLEMENTED;
}

/*
   wgl_line_format

   Changes line format, there are various formats available, check wopengl.h
   for more details.
*/
wstatus wgl_line_format(wgl_line_format_t line_format)
{
	dbgprint(MOD_WOPENGL,__func__,"unimplemented function called!");
	return WSTATUS_UNIMPLEMENTED;
}

/*
   iwgl_viewport_init

   Initialize the viewport of OpenGL, this will set the background color
   to black, and visualization mode to ortho.
*/
wstatus iwgl_viewport_init(void)
{
	dbgprint(MOD_WOPENGL,__func__,"called");

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f );

	dbgprint(MOD_WOPENGL,__func__,"set clear color to black");

	dbgprint(MOD_WOPENGL,__func__,"returning with success.");
	return WSTATUS_SUCCESS;
}

/*
   iwgl_resize_window

   Internal function called by OpenGL library when the GL window is resized
   by the user. New window characteristics is passed in width and height
   arguments.
*/
GLvoid
iwgl_resize_window(GLsizei width, GLsizei height)
{
	dbgprint(MOD_WOPENGL,__func__,"called with width=%u and height=%d",
			width,height);

	glViewport(0,0,width,height);
	
	dbgprint(MOD_WOPENGL,__func__,"updated viewport to (0,0) (%d,%d)",width,height);

	glMatrixMode(GL_PROJECTION);

	dbgprint(MOD_WOPENGL,__func__,"changed to projection matrix mode");

	glLoadIdentity();

	dbgprint(MOD_WOPENGL,__func__,"updating viewport internal variable");
	
	viewport.x = width;
	viewport.y = height;
	
	dbgprint(MOD_WOPENGL,__func__,"updated viewport internal variable successfuly");

	dbgprint(MOD_WOPENGL,__func__,"calling glOrtho with new viewport size");
	
	glOrtho(0.0f,width,height,0.0f,-1.0f,1.0f);

	dbgprint(MOD_WOPENGL,__func__,"switching to model view matrix mode");
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	dbgprint(MOD_WOPENGL,__func__,"returning.");
	return;
}

/*
   iwgl_draw_frame

   Internal function that is called by OpenGL to draw a single frame.
*/
GLvoid
iwgl_draw_frame(GLvoid)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	GLdouble x,y,z,angle;
	glBegin(GL_LINES);
	z = 0.0f;
	for(angle = 0.0f; angle <= GL_PI; angle += (GL_PI/20.0f))
	{ 
		// Top half of the circle 
		x = 50.0f*sin(angle); 
		y = 50.0f*cos(angle); 
		glVertex3f(x, y, z);        // First endpoint of line 
		// Bottom half of the circle 
		x = 50.0f*sin(angle + GL_PI); 
		y = 50.0f*cos(angle + GL_PI); 
		glVertex3f(x, y, z);        // Second endpoint of line 
	}
	glEnd();

	glutSwapBuffers();
}


