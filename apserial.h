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

#ifndef _APSERIAL_H
#define _APSERIAL_H

#include "wstatus.h"

typedef struct _aps_t {
	char *serial;
	bool set;
	bool changed;
} *aps_t;

typedef enum _aps_action_t {
	aps_action_preget,
	aps_action_postget,
	aps_action_preset,
	aps_action_postset
} aps_action_t;

typedef struct _aps_action_info_t {
	aps_t aps;
	char *serial;
	aps_action_t action;
} aps_action_info_t;

typedef wstatus (*APSHOOKROUTINE)(aps_action_info_t action_info,void *param);

aps_get(aps_t aps,char *serial);
aps_set(aps_t aps,char *serial,bool dup_string);
aps_attach_hook(APSHOOKROUTINE routine,void *param);
aps_dettach_hook(APSHOOKROUTINE routine);

#endif
