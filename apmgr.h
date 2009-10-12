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

   The difference between this module and aplist is that aplist is
   just a container of data, it doesn't process commands from the user.
   apmgr is global and static, aplist is like a class, it can be
   allocated and used by apmgr. This module will be associated to the
   console and also the copengl module for drawing access points.
   Each map has a aplist object.
*/
#ifndef _APMGR_H
#define _APMGR_H

#include <stdbool.h>
#include <cfgmgr.h>
#include <apl.h>

typedef struct _apmgr_init_t {
	char junk;
} apmgr_init_t;

typedef enum _apmgr_params_t {
	APMGR_CURMAPL
} apmgr_params_t;

typedef enum _apmgr_action_t {
	APMGR_ACTION_PRESET,
	APMGR_ACTION_POSTSET,
	APMGR_ACTION_PREGET,
	APMGR_ACTION_POSTGET,
	APMGR_ACTION_PREINSERT,
	APMGR_ACTION_POSTINSERT,
	APMGR_ACTION_PREREPLACE,
	APMGR_ACTION_POSTREPLACE,
	APMGR_ACTION_PREREMOVE,
	APMGR_ACTION_POSTREMOVE
} apmgr_action_t;

typedef struct _apmgr_action_get_t {
	apl_t current_apl;
	apmgr_param_t param;
	void *value_ptr;
	size_t value_size;
} apmgr_action_get_t;

typedef struct _apmgr_action_set_t {
	apl_t current_apl;
	apmgr_param_t param;
	void *value_ptr;
	size_t value_size;
	bool param_set;
	bool param_changed;
} apmgr_action_set_t;

typedef struct _apmgr_action_insert_t {
	apl_t current_apl;
	apl_t insert_apl;
	ap_t ap;
} apmgr_action_insert_t;

/* both index and name should be filled */
typedef struct _apmgr_action_replace_t {
	apl_t current_apl;
	apl_t replace_apl;
	const char *ap_name;
	ap_index_t ap_index;
	ap_t new_ap;
} apmgr_action_replace_t;

/* both index and name should be filled */
typedef struct _apmgr_action_remove_t {
	apl_t current_apl;
	apl_t remove_apl;
	const char *ap_name;
	ap_index_t ap_index;
	ap_t ap;
} apmgr_action_remove_t;

typedef struct _apmgr_action_info_t {
	union {
		apmgr_action_get_t get;
		apmgr_action_set_t set;
		apmgr_action_insert_t insert;
		apmgr_action_replace_t replace;
		apmgr_action_remove_t remove;
	} data;
	apmgr_action_t action;
} apmgr_action_info_t;

typedef wstatus (*APMGRHOOKROUTINE)(apl_t ap_list,apmgr_action_info_t action_info,void *param);

wstatus apmgr_initialize(apmgr_init_t apmgr);
wstatus apmgr_uninitialize();
wstatus apmgr_config_load(cfgmgr_t cfgmgr,char *section);
wstatus apmgr_config_save(cfgmgr_t cfgmgr,char *section);
wstatus apmgr_insert(apl_t ap_list,ap_t ap);
wstatus apmgr_replace_by_name(apl_t ap_list,char *name,ap_t new_ap);
wstatus apmgr_replace_by_index(apl_t ap_list,ap_index_t index,ap_t new_ap);
wstatus apmgr_remove_by_name(apl_t ap_list,char *name);
wstatus apmgr_remove_by_index(apl_t ap_list,ap_index_t index);
wstatus apmgr_lookup_by_ip(apl_t apl,apip_t ip,ap_t *ap);
wstatus apmgr_set(apmgr_param_t param,void *value_ptr,size_t value_size);
wstatus apmgr_get(apmgr_param_t param,void *value_ptr,size_t value_size);
wstatus apmgr_param_isset(apmgr_params_t param,bool *set_flag);
wstatus apmgr_param_changed(apmgr_params_t param,bool *change_flag);
wstatus apmgr_attach_hook(APMGRHOOKROUTINE routine,void *param);
wstatus apmgr_dettach_hook(APMGRHOOKROUTINE routine,void *param);
wstatus apmgr_draw(apl_t ap_list);
wstatus apmgr_dump(apl_t ap_list);

#endif

