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
	module description

	mapmgr is global and is just one, thats why it isn't a class.
	inside the mapmgr there's a list of maps and its saved using
	jmlist library. mapmgr is able to manage this list.
	
	mapmgr also installs an hook to copengl so when a frame is
	drawn mapmgr is called to draw each map (which will also make
	draw each map objects like shapes, access points, etc.)

*/

#ifndef _MAPMGR_H
#define _MAPMGR_H

#include "wstatus.h"

typedef const char* map_name_t;
typedef unsigned int map_index_t;

typedef enum _mapmgr_flags_t {
	//...
	JUNK
} mapmgr_flags_t;

typedef struct _mapmgr_init_t {
	mapmgr_flags_t flags;
} mapmgr_init_t;

typedef enum _mapmgr_action_t {
	mapmgr_action_preinsert,
	mapmgr_action_postinsert,
	mapmgr_action_prereplace,
	mapmgr_action_postreplace,
	mapmgr_action_preremove,
	mapmgr_action_postremove,
	mapmgr_action_preset,
	mapmgr_action_postset,
	mapmgr_action_preget,
	mapmgr_action_postget,
	mapmgr_action_predump,
	mapmgr_action_postdump
} mapmgr_action_t;

typedef struct _mapmgr_action_insert_t {
	map_t map;
	map_index_t index;
} mapmgr_action_insert_t;

typedef struct _mapmgr_action_replace_t {
	map_t map;
	map_index_t index;
} mapmgr_action_replace_t;

typedef struct _mapmgr_action_remove_t {
	map_index_t index;
} mapmgr_action_replace_t;

typedef struct _mapmgr_action_set_t {
	mapmgr_param_t param;
	void *value_ptr;
	size_t value_size;
} mapmgr_action_set_t ;

typedef struct _mapmgr_action_get_t {
	mapmgr_param_t param;
	void *value_ptr;
	size_t value_size;
} mapmgr_action_get_t;

typedef struct _mapmgr_action_info_t {
	union {
		mapmgr_action_insert_t insert;
		mapmgr_action_replace_t replace;
		mapmgr_action_remove_t remove;
		mapmgr_action_set_t set;
		mapmgr_action_get_t get;
	} data;
	mapmgr_action_t action;
} mapmgr_action_info_t;

typedef enum _mapmgr_param_t {
	mapmgr_current_map_index = 1
} mapmgr_param_t;

typedef wstatus (*MAPMGRHOOKROUTINE)(mapmgr_action_info_t action_info,void *param);

wstatus mapmgr_insert(map_t map,map_index_t index);
wstatus mapmgr_remove_by_name(map_name_t name);
wstatus mapmgr_remove_by_index(map_index_t index);
wstatus mapmgr_replace_by_name(map_t map,map_name_t name);
wstatus mapmgr_replace_by_index(map_t map,map_index_t index);
wstatus mapmgr_set(mapmgr_param_t param,void *value_ptr,size_t value_size);
wstatus mapmgr_get(mapmgr_param_t param,void *value_ptr,size_t value_size);
wstatus mapmgr_lookup_by_name(map_name_t name,map_t *map);
wstatus mapmgr_lookup_by_index(map_index_t index,map_t *map);
wstatus mapmgr_changed(bool *changed_flag);
wstatus mapmgr_initialize(apl_init_t init);
wstatus mapmgr_uninitialize();
wstatus mapmgr_draw();
wstatus mapmgr_attach_hook(MAPMGRHOOKROUTINE routine,void *param);
wstatus mapmgr_dettach_hook(MAPMGRHOOKROUTINE routine);
wstatus mapmgr_dump(void);

#endif

