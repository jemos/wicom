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

#ifndef _WVIEWCTL_H
#define _WVIEWCTL_H

#include "posh.h"
#include "wstatus.h"
#include "wviewcb.h"

wstatus wvctl_load(void);
wstatus wvctl_unload(void);

wstatus wvctl_register_keyboard_cb(wvkeyboard_cb keycb,void *param); 
wstatus wvctl_unregister_keyboard_cb(wvkeyboard_cb keycb,void *param); 
wstatus wvctl_register_mouse_cb(wvmouse_cb mousecb,void *param); 
wstatus wvctl_unregister_mouse_cb(wvmouse_cb mousecb,void *param); 
wstatus wvctl_redraw(void);

#endif

