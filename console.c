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

#include "wstatus.h"
#include "console.h"
#include "debug.h"

/* global console object */
static con_t g_con;

wstatus
con_initialize(con_init_t *con_init)
{
	dbgprint(MOD_CONSOLE,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

wstatus
con_unintialize(void)
{
	dbgprint(MOD_CONSOLE,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   con_process_key

   In the case of a raw console being used (e.g.: opengl simulated
   console) its possible to have more functionality in the command
   line by using special keys. In normal operation it will save
   the line characters in a buffer and when a return key is detected
   it calls con_process_line function.
*/
wstatus
con_process_key(int key)
{
	dbgprint(MOD_CONSOLE,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   con_process_line

   When return is pressed on the command line that line should be
   processed for execution. Each module can attach a command name+
   callback routine to the console. Everytime than that command
   is to be processed the callback routine is called.

   When the command name is not in the list an error is printed
   and this function ends.

   Attach the command using the function con_add_cmd.
*/
wstatus
con_process_line(const char *line)
{
	dbgprint(MOD_CONSOLE,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   con_insert_line

   Description NA.
*/
wstatus
con_insert_line(const char *line)
{
	dbgprint(MOD_CONSOLE,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   con_draw_lines

   Internal function to draw the last used console lines. This
   is used by the wopengl callback that is called whenever GL
   wants to draw a frame.

   Don't forget to NOT draw anything if the GL console is off.
*/
wstatus
con_draw_lines(void)
{
	dbgprint(MOD_CONSOLE,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   con_draw_carret

   Internal function to draw the carret, useful for guidance
   in the console. This is used by the wopengl callback that
   is called whenever GL wants to draw a frame.
*/
wstatus
con_draw_carret(void)
{
	dbgprint(MOD_CONSOLE,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   con_hide

   When a specific key is pressed in GL window its possible
   to hide the console. Thats what this function does, also
   this function should be called from con_process_key.
*/
wstatus
con_hide(void)
{
	dbgprint(MOD_CONSOLE,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   con_show

   When a specific key is pressed in GL window its possible
   to show the console. Thats what this function does, also
   this function should be called from con_process_key.
*/
wstatus
con_show(void)
{
	dbgprint(MOD_CONSOLE,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   con_toggle

   Toggles console window visibility, this should be called
   by con_process_key when the console visibility toggle key
   is pressed.
*/
wstatus
con_toggle(void)
{
	dbgprint(MOD_CONSOLE,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   con_draw

   Draws the console in the GL window, it should use wopengl
   primitive functions to draw everying so we'll need some
   basic functions from wopengl.
   
   This function is a callback routine registered when the
   console initializing function is called, that means that
   wopengl module should be initialized first and also
   con_initialize should be called also.
*/
wstatus
con_draw(void)
{
	dbgprint(MOD_CONSOLE,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   con_attach_hook

   Like every module, this also have hooking functionality.
   The caller should provide an callback routine address and
   the param is optional, will be passed to the callback
   routine whenever its called by the console module.
*/
wstatus
con_attach_hook(CONHOOKROUTINE routine,void *param)
{
	dbgprint(MOD_CONSOLE,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   con_dettach_hook

   It should be also possible to disable an hook, that is
   the same of unattaching it.
*/
wstatus
con_dettach_hook(CONHOOKROUTINE routine)
{
	dbgprint(MOD_CONSOLE,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   con_printf

   Support function to print something on the console output
   channels. Format is the same of printf standard function.
*/
wstatus
con_printf(const char *fmt,...)
{
	dbgprint(MOD_CONSOLE,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

/*
   con_puts

   Support function to print something on the console output
   channels. Format is the same of puts standard function.
*/
wstatus
con_puts(const char *text)
{
	dbgprint(MOD_CONSOLE,__func__,"unimplemented function called!");
	return STATUS_UNIMPLEMENTED;
}

