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
	bool param_set;
	bool param_changed;
} *aps_t;

typedef enum _aps_action_t {
	APS_ACTION_PREGET,
	APS_ACTION_POSTGET,
	APS_ACTION_PRESET,
	APS_ACTION_POSTSET,
	APS_ACTION_PRECHANGE,
	APS_ACTION_POSTCHANGE,
	APS_ACTION_PREDUMP,
	APS_ACTION_POSTDUMP
} aps_action_t;

typedef struct _aps_action_get_t {
	aps_t aps;
	const char *serial;
	bool param_set;
	bool param_changed;
} aps_action_get_t;

typedef struct _aps_action_set_t {
	aps_t aps;
	const char *serial;
	bool param_set;
	bool param_changed;
} aps_action_set_t;

typedef struct _aps_action_change_t {
	aps_t aps;
	const char *serial_from;
	const char *serial_to;
	bool param_set;
} aps_action_change_t;

typedef struct _aps_action_info_t {
	union {
		aps_action_set_t set;
		aps_action_get_t get;
		aps_action_change_t change;
	} data;
	aps_action_t action;
} aps_action_info_t;

typedef wstatus (*APSHOOKROUTINE)(aps_action_info_t action_info,void *param);

wstatus aps_get(aps_t self,char *serial);
wstatus aps_set(aps_t self,char *serial,bool dup_string);
wstatus aps_attach_hook(aps_t self,APSHOOKROUTINE routine,void *param);
wstatus aps_dettach_hook(aps_t self,APSHOOKROUTINE routine);
wstatus aps_dump(aps_t self);
wstatus aps_alloc(aps_t *aps);
wstatus aps_free(aps_t self);

#endif

