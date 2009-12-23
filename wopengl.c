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

static jmlist shapelist;
static jmlist plantlist;

/* internal functions to wopengl module */
GLvoid iwgl_draw_frame(GLvoid);
GLvoid iwgl_resize_window(GLsizei width, GLsizei height);
wstatus iwgl_viewport_init(void);
wstatus wgl_shapeid_alloc(shapeid_t *shapeid);
wstatus wgl_plantid_alloc(plantid_t *plantid);
wstatus wgl_validate_shapeid(shapeid_t shapeid,bool *result);
bool wgl_validate_layer(layer_t layer);
bool wgl_validate_shapetype(shapetype_t type);
wstatus wgl_validate_plantid(plantid_t plantid,bool *result);
wstatus wgl_shapeid2index(shapeid_t shapeid,bool *result,jmlist_index *index);
wstatus wgl_plantid2index(plantid_t plantid,bool *result,jmlist_index *index);

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

	dbgprint(MOD_WOPENGL,__func__,"creating jmlist for shapelist");

	struct _jmlist_params params = { .flags = JMLIST_INDEXED | JMLIST_IDX_USE_SHIFT };
	if( jmlist_create(&shapelist,&params) != JMLIST_ERROR_SUCCESS )
	{
		dbgprint(MOD_WOPENGL,__func__,"jmlist_create failed!");
		dbgprint(MOD_WOPENGL,__func__,"returning with failure.");
		return WSTATUS_FAILURE;
	}

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

/*
	wgl_shapemgr

	Possible actions are: SHAPE_ADD, SHAPE_REMOVE, SHAPE_GET, SHAPE_SET.

	SHAPE_ADD requires a filled data and pointer to shapeid_t variable, which
	will receive the shapeid as it is created. If shapeid = 0, this function
	will not pass the shapeid to the caller.

	The shapedata structure contains the layer identifier, plant identifier
	which is only required for LAYER_MAP*, shape type and the shape specific
	information (which depends on type). All these fields should be filled.

	SHAPE_REMOVE requires shapeid to be filled with a shapeid of an existing
	shape, shapedata argument is ignored.

	SHAPE_GET requires shapeid to be filled with a shapeid of an existing
	shape, shapedata pointing to a shapedata_t structure (contents ignored).
	It will fill the shapedata structure with the current shape data. One
	can then change that data and then call SHAPE_SET to modify some
	properties.

	SHAPE_SET requires shapeid to be filled with a shapeid of an existing
	shape, shapedata poiting to a shapedata_t structure filled. It will
	replace the current shape properties with those passed in. You can
	even change the type of the shape, or the layer associated...

	Returns:
	WSTATUS_FAILURE on failure, WSTATUS_SUCCESS otherwise.

	On failure, some possible errors are:
	Invalid arguments passed in (data=0 or shapeid=0).
	Invalid or unsupported action specified.
*/
wstatus
wgl_shapemgr(shapemgr_action_t action,shapedata_t *data,shapeid_t *shapeid)
{
	bool result;
	jmlist_status status;
	jmlist_index index;
	void *ptr;

	switch(action)
	{
		case SHAPE_ADD:
			/* do some data validation */
			if( !data || !shapeid )
			{
				dbgprint(MOD_WOPENGL,__func__,"Invalid data pointer or shapeid pointer");
				break;
			}
			/* on the shape type */
			if( !wgl_validate_shapetype(data->type) )
			{
				dbgprint(MOD_WOPENGL,__func__,"Invalid shape type in shapemgr_data structure! (type=%d)",data->type);
				break;
			}

			/* check shape layer */
			if( !wgl_validate_layer(data->layer) )
			{
				dbgprint(MOD_WOPENGL,__func__,"Invalid shape layer in shapemgr_data structure! (layer=%u)",data->layer);
				break;
			}

			/* check plant identifier */
			if( wgl_validate_plantid(data->plantid,&result) == WSTATUS_SUCCESS )
			{
				if( !result )
				{
					dbgprint(MOD_WOPENGL,__func__,"Invalid shape plantid in shapemgr_data structure! (plant=%u)",data->plantid);
					break;
				}
			} else {
				break;
			}

			shapeid_t shapeid_temp;
			dbgprint(MOD_WOPENGL,__func__,"asking for a new shapeid");
			if( wgl_shapeid_alloc(&shapeid_temp) != WSTATUS_SUCCESS )
				break;

			dbgprint(MOD_WOPENGL,__func__,"got new shapeid=%d updating shapeid argument",shapeid_temp);
			*shapeid = shapeid_temp;

			/* finally add the shape to the list */
			dbgprint(MOD_WOPENGL,__func__,"updating data->shapeid to %d",shapeid_temp);
			data->shapeid = shapeid_temp;

			dbgprint(MOD_WOPENGL,__func__,"adding the new shape to shapelist using ptr=%p",data);
			if( (status = jmlist_insert(shapelist,data)) != JMLIST_ERROR_SUCCESS )
			{
				dbgprint(MOD_WOPENGL,__func__,"jmlist_insert returned error (%X)",status);
				break;
			}

			dbgprint(MOD_WOPENGL,__func__,"jmlist_insert returned success");
			dbgprint(MOD_WOPENGL,__func__,"Returning with success.");
			return WSTATUS_SUCCESS;
		case SHAPE_REMOVE:
			if( !shapeid )
			{
				dbgprint(MOD_WOPENGL,__func__,"Invalid shapeid pointer");
				break;
			}

			/* check if shape exists */
			if( wgl_validate_shapeid(*shapeid,&result) == WSTATUS_SUCCESS )
			{
				if( !result )
				{
					/* shapeid doesn't exist... */
					dbgprint(MOD_WOPENGL,__func__,"The shapeid specified (%d) doesn't exist in shapelist!",*shapeid);
					break;
				}
			} else break;

			/* lookup shape index and remove it, for now these operations arent quite optimized... */
			if( wgl_shapeid2index(*shapeid,0,&index) != WSTATUS_SUCCESS )
			{
				dbgprint(MOD_WOPENGL,__func__,"unable to translate shapeid to index?!");
				break;
			}

			/* remove entry */
			dbgprint(MOD_WOPENGL,__func__,"calling jmlist_remove_by_index...");
			if( (status = jmlist_remove_by_index(shapelist,index)) != JMLIST_ERROR_SUCCESS )
			{
				dbgprint(MOD_WOPENGL,__func__,"jmlist_insert returned error (%X)",status);
				break;
			}
			dbgprint(MOD_WOPENGL,__func__,"removed shape successfuly");
			dbgprint(MOD_WOPENGL,__func__,"returning with success.");
			return WSTATUS_SUCCESS;
		case SHAPE_GET:
			if( !shapeid )
			{
				dbgprint(MOD_WOPENGL,__func__,"Invalid shapeid pointer");
				break;
			}

			if( !data )
			{
				dbgprint(MOD_WOPENGL,__func__,"Invalid data pointer, this should point to shapedata_t structure to be filled");
				break;
			}

			/* check if shape exists */
			if( wgl_validate_shapeid(*shapeid,&result) == WSTATUS_SUCCESS )
			{
				if( !result )
				{
					/* shapeid doesn't exist... */
					dbgprint(MOD_WOPENGL,__func__,"The shapeid specified (%d) doesn't exist in shapelist!",*shapeid);
					break;
				}
			} else break;

			/* lookup shape index and remove it, for now these operations arent quite optimized... */
			if( wgl_shapeid2index(*shapeid,0,&index) != WSTATUS_SUCCESS )
			{
				dbgprint(MOD_WOPENGL,__func__,"unable to translate shapeid to index?!");
				break;
			}

			/* access the shapelist by index and get the shape */
			if( (status = jmlist_get_by_index(shapelist,index,&ptr)) != JMLIST_ERROR_SUCCESS )
			{
				dbgprint(MOD_WOPENGL,__func__,"jmlist_get_by_index returned error (%X)",status);
				break;
			}

			/* we got the shape in ptr pointer */
			dbgprint(MOD_WOPENGL,__func__,"copying shapedata structure into data pointer...");
			memcpy(data,ptr,sizeof(shapedata_t));
			dbgprint(MOD_WOPENGL,__func__,"copied data successfuly");

			dbgprint(MOD_WOPENGL,__func__,"returning with success.");
			return WSTATUS_SUCCESS;
		case SHAPE_SET:
			if( !shapeid )
			{
				dbgprint(MOD_WOPENGL,__func__,"Invalid shapeid pointer");
				break;
			}

			if( !data )
			{
				dbgprint(MOD_WOPENGL,__func__,"Invalid data pointer, this should point to shapedata_t structure to be filled");
				break;
			}

			/* check if shape exists */
			if( wgl_validate_shapeid(*shapeid,&result) == WSTATUS_SUCCESS )
			{
				if( !result )
				{
					/* shapeid doesn't exist... */
					dbgprint(MOD_WOPENGL,__func__,"The shapeid specified (%d) doesn't exist in shapelist!",*shapeid);
					break;
				}
			} else break;

			/* lookup shape index and remove it, for now these operations arent quite optimized... */
			jmlist_index index;
			if( wgl_shapeid2index(*shapeid,0,&index) != WSTATUS_SUCCESS )
			{
				dbgprint(MOD_WOPENGL,__func__,"unable to translate shapeid to index?!");
				break;
			}

			/* overwrite any shapeid specified in data structure */
			dbgprint(MOD_WOPENGL,__func__,"Overwritting data->shapeid=%u to %u",data->shapeid,*shapeid);
			data->shapeid = *shapeid;

			/* get shape pointer */
			if( (status = jmlist_get_by_index(shapelist,index,&ptr)) != JMLIST_ERROR_SUCCESS )
			{
				dbgprint(MOD_WOPENGL,__func__,"jmlist_get_by_index returned error (%X)",status);
				break;
			}

			/* we got the shape in ptr pointer */
			dbgprint(MOD_WOPENGL,__func__,"copying shapedata structure into data pointer...");
			memcpy(data,ptr,sizeof(shapedata_t));
			dbgprint(MOD_WOPENGL,__func__,"copied data successfully");

			dbgprint(MOD_WOPENGL,__func__,"returning with success.");
			return WSTATUS_SUCCESS;
		case SHAPE_DUMP:
			/* dump shape information */
			break;
		default:
			dbgprint(MOD_WOPENGL,__func__,"invalid or unsupported action (%d)",action);
	}
	dbgprint(MOD_WOPENGL,__func__,"returning with failure.");
	return WSTATUS_FAILURE;
}

bool wgl_validate_shapetype(shapetype_t type)
{
	switch(type)
	{
		case SHAPE_POINT:
		case SHAPE_LINE:
		case SHAPE_POLYLINE:
		case SHAPE_POLYGON:
		case SHAPE_TEXT:
			return true;
		default:
			return false;
	}
	return false;
}

bool wgl_validate_layer(layer_t layer)
{
	switch(layer)
	{
		case LAYER_MAP:
		case LAYER_MAP_GUIDES:
		case LAYER_MAP_TEXT:
		case LAYER_CONTROL_GUIDES:
		case LAYER_CONTROL_TEXT:
			return true;
		default:
			return false;
	}
	return false;
}

/*
   wgl_validate_shapeid

   Shapeid validation is used just to check if a specific shapeid exists in the list.
   If caller also want the index of this specific shapeid caller should instead use the
   alternative function, wgl_shapeid2index.

   No result should be considered if this function returns failure, in other meaning,
   result should only be considered if this function returns WSTATUS_SUCCCESS.
*/
wstatus
wgl_validate_shapeid(shapeid_t shapeid,bool *result)
{
	shapedata_t *shape;

	dbgprint(MOD_WOPENGL,__func__,"called with shapeid=%d and result=%p",shapeid,result);

	if( !result )
	{
		dbgprint(MOD_WOPENGL,__func__,"invalid result bool pointer (result=0), check the arguments");
		dbgprint(MOD_WOPENGL,__func__,"returning with failure.");
		return WSTATUS_FAILURE;
	}

	dbgprint(MOD_WOPENGL,__func__,"updating result to false, the default value");
	*result = false;

	/*
	   The shapelist is jmlist indexed with shift feature, meaning
	   that even when an entry is removed there wont be any hole
	   because entries will be shifted. With that we can parse the
	   list simply from 0 to the number of entries.
	 */

	jmlist_index shapecount = 0;
	jmlist_entry_count(shapelist,&shapecount);
	dbgprint(MOD_WOPENGL,__func__,"shape count is %d",shapecount);

	dbgprint(MOD_WOPENGL,__func__,"starting lookup look for shapeid (from 0 to %d)",shapecount);
	for( jmlist_index i = 0 ; i < shapecount ; i++ )
	{
		if( jmlist_get_by_index(shapelist,i,(void*)&shape) 
				!= JMLIST_ERROR_SUCCESS)
		{
			jmlist_index shapecountn = 0;
			jmlist_entry_count(shapelist,&shapecount);
			dbgprint(MOD_WOPENGL,__func__,"error retrieving entry from indexed list, "
					"old shapecount=%d, i=%d, now shapecount=%d",shapecount,i,shapecountn);
			dbgprint(MOD_WOPENGL,__func__,"if the old shapecount is different from now "
					"shapecount you've some thread removing entries while this one is seeking throughout the list!");
			dbgprint(MOD_WOPENGL,__func__,"returning with failure.");
			return WSTATUS_FAILURE;
		}

		/* lets not blow-up the program with null-ptr memory access */
		if( !shape )
		{
			dbgprint(MOD_WOPENGL,__func__,"jmlist access by index returned null ptr");
			dbgprint(MOD_WOPENGL,__func__,"didn't you actually deactivated shift feature of shapelist?!");
			dbgprint(MOD_WOPENGL,__func__,"returning with failure.");
			return WSTATUS_FAILURE;
		}

		/* test for shapeid */
		if( shape->shapeid == shapeid )
		{
			dbgprint(MOD_WOPENGL,__func__,"found shapeid=%d it has index=%d in shapelist",shapeid,i);
			dbgprint(MOD_WOPENGL,__func__,"updating result argument to true");
			*result = true;
			dbgprint(MOD_WOPENGL,__func__,"returning with success.");
			return WSTATUS_SUCCESS;
		}
	}

	dbgprint(MOD_WOPENGL,__func__,"entry with shapeid=%d was not found in shapelist! (processed %d entries successfully)");	
	dbgprint(MOD_WOPENGL,__func__,"returning with success.");
	return WSTATUS_SUCCESS;
}

wstatus
wgl_shapeid2index(shapeid_t shapeid,bool *result,jmlist_index *index)
{
	dbgprint(MOD_WOPENGL,__func__,"called with shapeid=%d and index=%p",shapeid,index);

	if( !index )
	{
		dbgprint(MOD_WOPENGL,__func__,"invalid index pointer specified (index=0) check the arguments");
		dbgprint(MOD_WOPENGL,__func__,"returning with failure.");
		return WSTATUS_FAILURE;
	}

	shapedata_t *shape;
	jmlist_index shapecount;

	if( jmlist_entry_count(shapelist,&shapecount) != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_WOPENGL,__func__,"returning with failure.");
		return WSTATUS_FAILURE;
	}
	
	dbgprint(MOD_WOPENGL,__func__,"starting loop from 0 to %d",shapecount);
	for( jmlist_index i = 0 ; i < shapecount ; i++ )
	{
		if( jmlist_get_by_index(shapelist,i,(void*)&shape) 
				!= JMLIST_ERROR_SUCCESS)
		{
			jmlist_index shapecountn = 0;
			jmlist_entry_count(shapelist,&shapecountn);
			dbgprint(MOD_WOPENGL,__func__,"error retrieving entry from indexed list, "
					"old shapecount=%d, i=%d, now shapecount=%d",shapecount,i,shapecountn);
			dbgprint(MOD_WOPENGL,__func__,"if the old shapecount is different from now "
					"shapecount you've some thread removing entries while this one is seeking throughout the list!");
			dbgprint(MOD_WOPENGL,__func__,"returning with failure.");
			return WSTATUS_FAILURE;
		}

		/* lets not blow-up the program with null-ptr memory access */
		if( !shape )
		{
			dbgprint(MOD_WOPENGL,__func__,"jmlist access by index returned null ptr");
			dbgprint(MOD_WOPENGL,__func__,"didn't you actually deactivated shift feature of shapelist?!");
			dbgprint(MOD_WOPENGL,__func__,"returning with failure.");
			return WSTATUS_FAILURE;
		}

		if( shape->shapeid == shapeid )
		{
			dbgprint(MOD_WOPENGL,__func__,"found entry with shapeid=%d at index=%d",shapeid,i);
			if( result )
			{
				dbgprint(MOD_WOPENGL,__func__,"updating result argument to true");
				*result = true;
			}
			dbgprint(MOD_WOPENGL,__func__,"updating index argument to %d",i);
			*index = i;
			dbgprint(MOD_WOPENGL,__func__,"returning with success.");
			return WSTATUS_SUCCESS;
		}
	}

	/* reached max number of shapes and didn't found the shapeid .. */
	dbgprint(MOD_WOPENGL,__func__,"reached max index number of shape, didn't found shapeid=%d",shapeid);
	dbgprint(MOD_WOPENGL,__func__,"returning with failure.");
	return WSTATUS_FAILURE;
}

wstatus
wgl_shapeid_alloc(shapeid_t *shapeid)
{
	/* lookup a free shapeid... */
	dbgprint(MOD_WOPENGL,__func__,"starting loop from %d to %d",SHAPEID_ZERO_OFFSET,MAX_SHAPEID);
	for( shapeid_t id = SHAPEID_ZERO_OFFSET ; id < MAX_SHAPEID ; id++ )
	{
		bool result;

		if( wgl_validate_shapeid(id,&result) == WSTATUS_SUCCESS )
		{
			if( result == false )
			{
				dbgprint(MOD_WOPENGL,__func__,"Found free shapeid = %d",id);
				*shapeid = id;
				return WSTATUS_SUCCESS;
			}
		} else return WSTATUS_FAILURE;
	}

	dbgprint(MOD_WOPENGL,__func__,"Unable to find a free shapeid!");
	dbgprint(MOD_WOPENGL,__func__,"Returning with failure.");
	return WSTATUS_FAILURE;
}

wstatus
wgl_validate_plantid(plantid_t plantid,bool *result)
{
	plantdata_t *plant;

	dbgprint(MOD_WOPENGL,__func__,"called with plantid=%d and result=%p",plantid,result);

	if( !result )
	{
		dbgprint(MOD_WOPENGL,__func__,"invalid result bool pointer (result=0), check the arguments");
		dbgprint(MOD_WOPENGL,__func__,"returning with failure.");
		return WSTATUS_FAILURE;
	}

	dbgprint(MOD_WOPENGL,__func__,"updating result to false, the default value");
	*result = false;

	/*
	   The plantlist is jmlist indexed with shift feature, meaning
	   that even when an entry is removed there wont be any hole
	   because entries will be shifted. With that we can parse the
	   list simply from 0 to the number of entries.
	 */

	jmlist_index plantcount = 0;
	
	if( jmlist_entry_count(plantlist,&plantcount) != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_WOPENGL,__func__,"returning with failure.");
		return WSTATUS_FAILURE;
	}

	dbgprint(MOD_WOPENGL,__func__,"plant count is %d",plantcount);

	dbgprint(MOD_WOPENGL,__func__,"starting lookup look for plantid (from 0 to %d)",plantcount);
	for( jmlist_index i = 0 ; i < plantcount ; i++ )
	{
		if( jmlist_get_by_index(plantlist,i,(void*)&plant) 
				!= JMLIST_ERROR_SUCCESS)
		{
			jmlist_index plantcountn = 0;
			jmlist_entry_count(plantlist,&plantcountn);
			dbgprint(MOD_WOPENGL,__func__,"error retrieving entry from indexed list, "
					"old plantcount=%d, i=%d, now plantcount=%d",plantcount,i,plantcountn);
			dbgprint(MOD_WOPENGL,__func__,"if the old plantcount is different from now "
					"plantcount you've some thread removing entries while this one is seeking throughout the list!");
			dbgprint(MOD_WOPENGL,__func__,"returning with failure.");
			return WSTATUS_FAILURE;
		}

		/* lets not blow-up the program with null-ptr memory access */
		if( !plant )
		{
			dbgprint(MOD_WOPENGL,__func__,"jmlist access by index returned null ptr");
			dbgprint(MOD_WOPENGL,__func__,"didn't you actually deactivated shift feature of plantlist?!");
			dbgprint(MOD_WOPENGL,__func__,"returning with failure.");
			return WSTATUS_FAILURE;
		}

		/* test for shapeid */
		if( plant->plantid == plantid )
		{
			dbgprint(MOD_WOPENGL,__func__,"found plantid=%d it has index=%d in plantlist",plantid,i);
			dbgprint(MOD_WOPENGL,__func__,"updating result argument to true");
			*result = true;
			dbgprint(MOD_WOPENGL,__func__,"returning with success.");
			return WSTATUS_SUCCESS;
		}
	}

	dbgprint(MOD_WOPENGL,__func__,"entry with plantid=%d was not found in plantlist! (processed %d entries successfully)");	
	dbgprint(MOD_WOPENGL,__func__,"returning with success.");
	return WSTATUS_SUCCESS;
}

/*
   wgl_plantmgr

   Plant Manager routine, this function provides various actions that can be applied to plants.

   PLANT_ADD
	- data_ptr points to plantdata_t structure, all members should be prefilled except plantid.
	- plantid should point to plantid_t type.
	- data_size should be sizeof(plantdata_t).

   PLANT_REMOVE
*/
wstatus
wgl_plantmgr(plantmgr_action_t action,void *data_ptr,size_t data_size,plantid_t *plantid)
{
	bool result;
	jmlist_status status;
	plantdata_t *plant;
	jmlist_index index;

	dbgprint(MOD_WOPENGL,__func__,"called with action=%d, data_ptr=%p, data_size=%u,plantid=%p",action,data_ptr,data_size,plantid);

	switch(action)
	{
		case PLANT_ADD:

			/* validate data */
			if( !data_ptr ) {
				dbgprint(MOD_WOPENGL,__func__,"invalid data_ptr specified (data_ptr=0)");
				break;
			}

			/* expected data is pointer to plantdata_t */
			if( data_size != sizeof(plantdata_t) ) {
				dbgprint(MOD_WOPENGL,__func__,"expected data size was %d and got %d bytes",sizeof(plantdata_t),data_size);
				break;
			}

			/* update local pointer to plantdata_t */
			plant = (plantdata_t*)data_ptr;

			/* get a new plantid for this plant */
			plantid_t plantid_temp;
			dbgprint(MOD_WOPENGL,__func__,"asking for a new plantid");
			if( wgl_plantid_alloc(&plantid_temp) != WSTATUS_SUCCESS )
				break;

			dbgprint(MOD_WOPENGL,__func__,"got new plantid=%d updating plantid argument",plantid_temp);
			*plantid = plantid_temp;

			/* finally add the shape to the list */
			dbgprint(MOD_WOPENGL,__func__,"updating data->plantid to %d",plantid_temp);
			plant->plantid = plantid_temp;

			dbgprint(MOD_WOPENGL,__func__,"adding the new plant to plantlist using ptr=%p",plant);
			if( (status = jmlist_insert(plantlist,plant)) != JMLIST_ERROR_SUCCESS ) {
				dbgprint(MOD_WOPENGL,__func__,"jmlist_insert returned error (%X)",status);
				break;
			}

			dbgprint(MOD_WOPENGL,__func__,"plant inserted successfuly");

			dbgprint(MOD_WOPENGL,__func__,"Returning with success.");
			return WSTATUS_SUCCESS;
			
			break;

		case PLANT_GET:
			
			if( !plantid ) {
				dbgprint(MOD_WOPENGL,__func__,"invalid *plantid specified (*plantid=0)");
				break; /* this will return with error */
			}

			if( wgl_validate_plantid(*plantid,&result) != WSTATUS_SUCCESS )
			{
				if( result == false ) {
					dbgprint(MOD_WOPENGL,__func__,"plantid is not valid! (plantid=%u)",*plantid);
					break;
				}
			} else
				break; /* wgl_validate_plant failed... */

			/* convert plantid to index for now these operations are not that optimized */
			if( wgl_plantid2index(*plantid,0,&index) != WSTATUS_SUCCESS ) {
				dbgprint(MOD_WOPENGL,__func__,"unable to translate plantid to index?!");
				break;
			}

			/* check data_ptr and data_size arguments */

			if( !data_ptr ) {
				dbgprint(MOD_WOPENGL,__func__,"invalid data_ptr specified (data_ptr=0)");
				break;
			}

			if( data_size != sizeof(plantdata_t) ) {
				dbgprint(MOD_WOPENGL,__func__,"unexpected data_size used=%u, expected=%u",data_size,sizeof(plantdata_t));
				break;
			}

			/* get plantdata_t structure pointer */
			if( jmlist_get_by_index(plantlist,index,(void*)&plant) != JMLIST_ERROR_SUCCESS ) {
				dbgprint(MOD_WOPENGL,__func__,"indexed access on plantlist failed for entry with index=%u!",index);
				break;
			}

			dbgprint(MOD_WOPENGL,__func__,"copying plant data structure to data_ptr...");
			memcpy(data_ptr,plant,sizeof(plantdata_t));
			dbgprint(MOD_WOPENGL,__func__,"copied successfully");
			dbgprint(MOD_WOPENGL,__func__,"returning with success.");
			return WSTATUS_SUCCESS;

			break;
		case PLANT_SET:

			if( !plantid ) {
				dbgprint(MOD_WOPENGL,__func__,"invalid *plantid specified (*plantid=0)");
				break; /* this will return with error */
			}

			if( wgl_validate_plantid(*plantid,&result) != WSTATUS_SUCCESS )
			{
				if( result == false ) {
					dbgprint(MOD_WOPENGL,__func__,"plantid is not valid! (plantid=%u)",*plantid);
					break;
				}
			} else
				break; /* wgl_validate_plant failed... */

			/* convert plantid to index for now these operations are not that optimized */
			if( wgl_plantid2index(*plantid,0,&index) != WSTATUS_SUCCESS ) {
				dbgprint(MOD_WOPENGL,__func__,"unable to translate plantid to index?!");
				break;
			}

			/* check data_ptr and data_size arguments */

			if( !data_ptr ) {
				dbgprint(MOD_WOPENGL,__func__,"invalid data_ptr specified (data_ptr=0)");
				break;
			}

			if( data_size != sizeof(plantdata_t) ) {
				dbgprint(MOD_WOPENGL,__func__,"unexpected data_size used=%u, expected=%u",data_size,sizeof(plantdata_t));
				break;
			}

			plant = (plantdata_t*)data_ptr;
			dbgprint(MOD_WOPENGL,__func__,"fixing plantid from received plant data structure from %u to %u",
					plant->plantid, *plantid);

			plant->plantid = *plantid;

			dbgprint(MOD_WOPENGL,__func__,"updating plant data structure");

			if( jmlist_replace_by_index(plantlist,index,plant) != JMLIST_ERROR_SUCCESS ) {
				break;
			}

			dbgprint(MOD_WOPENGL,__func__,"updated successfully");
			dbgprint(MOD_WOPENGL,__func__,"returning with success.");
			return WSTATUS_SUCCESS;
			break;

		case PLANT_REMOVE:

			if( !plantid ) {
				dbgprint(MOD_WOPENGL,__func__,"invalid *plantid specified (*plantid=0)");
				break; /* this will return with error */
			}

			if( wgl_validate_plantid(*plantid,&result) != WSTATUS_SUCCESS )
			{
				if( result == false ) {
					dbgprint(MOD_WOPENGL,__func__,"plantid is not valid! (plantid=%u)",*plantid);
					break;
				}
			} else
				break; /* wgl_validate_plant failed... */

			/* convert plantid to index for now these operations are not that optimized */
			if( wgl_plantid2index(*plantid,0,&index) != WSTATUS_SUCCESS ) {
				dbgprint(MOD_WOPENGL,__func__,"unable to translate plantid to index?!");
				break;
			}

			/* remove plant */
			dbgprint(MOD_WOPENGL,__func__,"calling jmlist_remove_by_index...");
			if( (status = jmlist_remove_by_index(plantlist,index)) != JMLIST_ERROR_SUCCESS )
			{
				dbgprint(MOD_WOPENGL,__func__,"jmlist_insert returned error (%X)",status);
				break;
			}

			dbgprint(MOD_WOPENGL,__func__,"removed plant successfuly");
			dbgprint(MOD_WOPENGL,__func__,"returning with success.");
			return WSTATUS_SUCCESS;
			break;
		
		case PLANT_SETVISIBLE:

			if( !plantid ) {
				dbgprint(MOD_WOPENGL,__func__,"invalid *plantid specified (*plantid=0)");
				break; /* this will return with error */
			}

			if( wgl_validate_plantid(*plantid,&result) != WSTATUS_SUCCESS )
			{
				if( result == false ) {
					dbgprint(MOD_WOPENGL,__func__,"plantid is not valid! (plantid=%u)",*plantid);
					break;
				}
			} else
				break; /* wgl_validate_plant failed... */

			/* convert plantid to index for now these operations are not that optimized */
			if( wgl_plantid2index(*plantid,0,&index) != WSTATUS_SUCCESS ) {
				dbgprint(MOD_WOPENGL,__func__,"unable to translate plantid to index?!");
				break;
			}

			/* check data_ptr and data_size arguments */

			if( !data_ptr ) {
				dbgprint(MOD_WOPENGL,__func__,"invalid data_ptr specified (data_ptr=0)");
				break;
			}

			if( data_size != sizeof(plant_visibility_t) ) {
				dbgprint(MOD_WOPENGL,__func__,"unexpected data_size used=%u, expected=%u",data_size,sizeof(plant_visibility_t));
				break;
			}

			/* now update plant visibility parameter */

			plant_visibility_t *visibility_new = (plant_visibility_t*)data_ptr;
			if( jmlist_get_by_index(plantlist,index,(void*)&plant) != JMLIST_ERROR_SUCCESS ) {
				dbgprint(MOD_WOPENGL,__func__,"indexed access on plantlist failed for entry with index=%u!",index);
				break;
			}

			dbgprint(MOD_WOPENGL,__func__,"got plant=%p updating visibility from %u to %u",
					(void*)plant,plant->visibility,*visibility_new);
			plant->visibility = *visibility_new;
			dbgprint(MOD_WOPENGL,__func__,"updated successfuly, new visibility is now %u",
					plant->visibility);

			dbgprint(MOD_WOPENGL,__func__,"returning with success.");
			return WSTATUS_SUCCESS;

			break;
		case PLANT_SETSCALE:
			if( !plantid ) {
				dbgprint(MOD_WOPENGL,__func__,"invalid *plantid specified (*plantid=0)");
				break; /* this will return with error */
			}

			if( wgl_validate_plantid(*plantid,&result) != WSTATUS_SUCCESS )
			{
				if( result == false ) {
					dbgprint(MOD_WOPENGL,__func__,"plantid is not valid! (plantid=%u)",*plantid);
					break;
				}
			} else
				break; /* wgl_validate_plant failed... */

			/* convert plantid to index for now these operations are not that optimized */
			if( wgl_plantid2index(*plantid,0,&index) != WSTATUS_SUCCESS ) {
				dbgprint(MOD_WOPENGL,__func__,"unable to translate plantid to index?!");
				break;
			}

			/* check data_ptr and data_size arguments */

			if( !data_ptr ) {
				dbgprint(MOD_WOPENGL,__func__,"invalid data_ptr specified (data_ptr=0)");
				break;
			}

			if( data_size != sizeof(v3d_t) ) {
				dbgprint(MOD_WOPENGL,__func__,"unexpected data_size used=%u, expected=%u",data_size,sizeof(v3d_t));
				break;
			}

			/* now update plant visibility parameter */

			v3d_t *scale = (v3d_t*)data_ptr;
			if( jmlist_get_by_index(plantlist,index,(void*)&plant) != JMLIST_ERROR_SUCCESS ) {
				dbgprint(MOD_WOPENGL,__func__,"indexed access on plantlist failed for entry with index=%u!",index);
				break;
			}

			dbgprint(MOD_WOPENGL,__func__,"got plant=%p updating scale from (%0.2g, %0.2g, %0.2g) to (%0.2g, %0.2g, %0.2g)",
					(void*)plant,plant->scale.x,plant->scale.y,plant->scale.z,scale->x,scale->y,scale->z);

			memcpy(&plant->scale,scale,sizeof(v3d_t));

			dbgprint(MOD_WOPENGL,__func__,"updated successfuly, new visibility is now (%0.2g, %0.2g, %0.2g)",
					plant->scale.x,plant->scale.y,plant->scale.z);

			dbgprint(MOD_WOPENGL,__func__,"returning with success.");
			return WSTATUS_SUCCESS;
			break;
		case PLANT_SETTRANSLATE:
			if( !plantid ) {
				dbgprint(MOD_WOPENGL,__func__,"invalid *plantid specified (*plantid=0)");
				break; /* this will return with error */
			}

			if( wgl_validate_plantid(*plantid,&result) != WSTATUS_SUCCESS )
			{
				if( result == false ) {
					dbgprint(MOD_WOPENGL,__func__,"plantid is not valid! (plantid=%u)",*plantid);
					break;
				}
			} else
				break; /* wgl_validate_plant failed... */

			/* convert plantid to index for now these operations are not that optimized */
			if( wgl_plantid2index(*plantid,0,&index) != WSTATUS_SUCCESS ) {
				dbgprint(MOD_WOPENGL,__func__,"unable to translate plantid to index?!");
				break;
			}

			/* check data_ptr and data_size arguments */

			if( !data_ptr ) {
				dbgprint(MOD_WOPENGL,__func__,"invalid data_ptr specified (data_ptr=0)");
				break;
			}

			if( data_size != sizeof(v3d_t) ) {
				dbgprint(MOD_WOPENGL,__func__,"unexpected data_size used=%u, expected=%u",data_size,sizeof(v3d_t));
				break;
			}

			/* now update plant visibility parameter */

			v3d_t *translate = (v3d_t*)data_ptr;
			if( jmlist_get_by_index(plantlist,index,(void*)&plant) != JMLIST_ERROR_SUCCESS ) {
				dbgprint(MOD_WOPENGL,__func__,"indexed access on plantlist failed for entry with index=%u!",index);
				break;
			}

			dbgprint(MOD_WOPENGL,__func__,"got plant=%p updating translate from (%0.2g, %0.2g, %0.2g) to (%0.2g, %0.2g, %0.2g)",
					(void*)plant,plant->translate.x,plant->translate.y,plant->translate.z,translate->x,translate->y,translate->z);

			memcpy(&plant->translate,translate,sizeof(v3d_t));

			dbgprint(MOD_WOPENGL,__func__,"updated successfuly, new visibility is now (%0.2g,%0.2g,%0.2g)",
					plant->translate.x,plant->translate.y,plant->translate.z);

			dbgprint(MOD_WOPENGL,__func__,"returning with success.");
			return WSTATUS_SUCCESS;
			break;
		default:
			dbgprint(MOD_WOPENGL,__func__,"invalid or unsupported action (%d)",action);
	}

	dbgprint(MOD_WOPENGL,__func__,"returning with failure.");
	return WSTATUS_FAILURE;;
}

wstatus
wgl_plantid_alloc(plantid_t *plantid)
{
	/* lookup a free shapeid... */
	dbgprint(MOD_WOPENGL,__func__,"starting loop from %d to %d",PLANTID_ZERO_OFFSET,MAX_PLANTID);
	for( plantid_t id = PLANTID_ZERO_OFFSET ; id < MAX_PLANTID ; id++ )
	{
		bool result;

		if( wgl_validate_plantid(id,&result) == WSTATUS_SUCCESS )
		{
			if( result == false )
			{
				dbgprint(MOD_WOPENGL,__func__,"Found free plantid = %d",id);
				*plantid = id;
				return WSTATUS_SUCCESS;
			}
		} else return WSTATUS_FAILURE;
	}

	dbgprint(MOD_WOPENGL,__func__,"Unable to find a free plantid!");
	dbgprint(MOD_WOPENGL,__func__,"Returning with failure.");
	return WSTATUS_FAILURE;
}

wstatus
wgl_plantid2index(plantid_t plantid,bool *result,jmlist_index *index)
{
	dbgprint(MOD_WOPENGL,__func__,"called with plantid=%d and index=%p",plantid,index);

	if( !index )
	{
		dbgprint(MOD_WOPENGL,__func__,"invalid index pointer specified (index=0) check the arguments");
		dbgprint(MOD_WOPENGL,__func__,"returning with failure.");
		return WSTATUS_FAILURE;
	}

	plantdata_t *plant;
	jmlist_index plantcount;

	if( jmlist_entry_count(plantlist,&plantcount) != JMLIST_ERROR_SUCCESS ) {
		dbgprint(MOD_WOPENGL,__func__,"returning with failure.");
		return WSTATUS_FAILURE;
	}

	dbgprint(MOD_WOPENGL,__func__,"starting loop from 0 to %d",plantcount);
	for( jmlist_index i = 0 ; i < plantcount ; i++ )
	{
		if( jmlist_get_by_index(plantlist,i,(void*)&plant) 
				!= JMLIST_ERROR_SUCCESS)
		{
			jmlist_index plantcountn = 0;
			jmlist_entry_count(plantlist,&plantcountn);
			dbgprint(MOD_WOPENGL,__func__,"error retrieving entry from indexed list, "
					"old plantcount=%d, i=%d, now plantcount=%d",plantcount,i,plantcountn);
			dbgprint(MOD_WOPENGL,__func__,"if the old plantcount is different from now "
					"plantcount you've some thread removing entries while this one is seeking throughout the list!");
			dbgprint(MOD_WOPENGL,__func__,"returning with failure.");
			return WSTATUS_FAILURE;
		}

		/* lets not blow-up the program with null-ptr memory access */
		if( !plant )
		{
			dbgprint(MOD_WOPENGL,__func__,"jmlist access by index returned null ptr");
			dbgprint(MOD_WOPENGL,__func__,"didn't you actually deactivated shift feature of shapelist?!");
			dbgprint(MOD_WOPENGL,__func__,"returning with failure.");
			return WSTATUS_FAILURE;
		}

		if( plant->plantid == plantid )
		{
			dbgprint(MOD_WOPENGL,__func__,"found entry with plantid=%d at index=%d",plantid,i);
			if( result )
			{
				dbgprint(MOD_WOPENGL,__func__,"updating result argument to true");
				*result = true;
			}
			dbgprint(MOD_WOPENGL,__func__,"updating index argument to %d",i);
			*index = i;
			dbgprint(MOD_WOPENGL,__func__,"returning with success.");
			return WSTATUS_SUCCESS;
		}
	}

	/* reached max number of shapes and didn't found the shapeid .. */
	dbgprint(MOD_WOPENGL,__func__,"reached max index number of plant, didn't found plantid=%d",plantid);
	dbgprint(MOD_WOPENGL,__func__,"returning with failure.");
	return WSTATUS_FAILURE;
}

