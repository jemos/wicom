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

#ifndef _APCHANNEL_H
#define _APCHANNEL_H

#include <stdbool.h>
#include "wstatus.h"

typedef struct _apch_t
{
	double channel;
	bool param_changed;
	bool param_set;
} *apch_t;

typedef struct _apch_action_get_t {
	double channel;
	void *value_ptr;
	size_t value_size;
	bool set_flag;
	bool changed_flag;
} apch_action_get_t;

typedef struct _apch_action_set_t {
	double channel;
	void *value_ptr;
	size_t value_size;
	bool set_flag;
	bool changed_flag;
} apch_action_get_t;

typedef enum _apch_action_t {
	APCH_ACTION_PRESET,
	APCH_ACTION_POSTSET,
	APCH_ACTION_PREGET,
	APCH_ACTION_POSTGET
	APCH_ACTION_PREDUMP,
	APCH_ACTION_POSTDUMP
} apch_action_t;

typedef struct _apch_action_info_t {
	union {
		apch_action_get_t get;
		apch_action_set_t set;
	} data;
	apch_action_t action;
} apch_action_info_t;

typedef wstatus (*APHHOOKROUTINE)(apch_t apch,apch_action_info_t action_info,void *param);

wstatus apch_alloc(apch_t *apch);
wstatus apch_free(apch_t self);
wstatus apch_set(apch_t self,apch_action_t action,void *value_ptr,size_t value_size);
wstatus apch_get(apch_t self,apch_action_t action,void *value_ptr,size_t value_size);
wstatus apch_dump(apch_t self);
wstatus apch_attach_hook(apch_t self,APCHHOOKROUTINE routine,void *param);
wstatus apch_dettach_hook(apch_t self,APCHHOOKROUTINE routine);
wstatus apch_was_changed(apch_t self,bool *changed_flag);
wstatus apch_isset(apch_t self,bool *set_flag);

#endif

