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
#ifndef _MAP_H
#define _MAP_H

#include <stdbool.h>
#include "apl.h"

typedef struct _map_t {
	jmlist shape_list;
	apl_t ap_list;
	v3d origin;
	bool loaded_map;
	char *name;
} *map_t;

typedef enum _map_action_t {
	MAP_ACTION_ALLOC,
	MAP_ACTION_PRELOAD,
	MAP_ACTION_POSTLOAD,
	MAP_ACTION_PREFREE,
	MAP_ACTION_POSTFREE,
	MAP_ACTION_PRESET,
	MAP_ACTION_POSTSET,
	MAP_ACTION_PREGET,
	MAP_ACTION_POSTGET,
	MAP_ACTION_PREDUMP,
	MAP_ACTION_POSTDUMP
} map_action_t;

typedef struct _map_action_info_t {
	union {
		map_action_load_t load;
		map_action_alloc_t alloc;
		map_action_free_t free;
		map_action_get_t get;
		map_action_set_t set;
	} data;
	map_action_t action;
} map_action_info_t;

typedef wstatus (*MAPHOOKROUTINE)(map_t map,map_action_info_t action_info,void *param);

typedef enum _map_params_t {
	MAP_NAME = 1,
	MAP_ORIGIN,
	MAP_ORDER
} map_params_t;

wstatus map_dump(map_t self);
wstatus map_set_param(map_t self,map_params_t param,void *value_ptr,size_t value_size);
wstatus map_get_param(map_t self,map_params_t param,void *value_ptr,size_t value_size);
wstatus map_free(map_t self);
wstatus map_alloc(map_t *map);
wstatus map_config_load(cfgmgr_t cfgmgr,char *section,map_t *map);
wstatus map_config_save(map_t self,cfgmgr_t cfgmgr,char *section);
wstatus map_attach_hook(MAPHOOKROUTINE routine,void *param);
wstatus map_dettach_hook(MAPHOOKROUTINE routine);
wstatus map_draw(map_t self);

#endif

