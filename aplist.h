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

#ifndef _APLIST_H
#define _APLIST_H

#include <stdbool.h>
#include "wstatus.h"
#include "ap.h"
#include "jmlist.h"

typedef enum _apl_t {
	jmlist ap_list;
} *apl_t;

typedef unsigned int ap_index_t;

typedef enum _apl_param_t {
	/* no params for now.. */
} apl_param_t;

typedef struct _apl_action_get_t {
	apl_param_t param;
	void *value_ptr;
	size_t value_size;
} apl_action_get_t;

typedef struct _apl_action_set_t {
	apl_param_t param;
	void *value_ptr;
	size_t value_size;
} apl_action_set_t;

typedef struct _apl_action_insert_t {
	ap_index_t index;
	ap_t new_ap;
} apl_action_insert_t;

typedef struct _apl_action_remove_t {
	ap_index_t index;
} apl_action_remove_t;

typedef struct _apl_action_alloc_t {
	ap_t new_ap;
} apl_action_alloc_t;

typedef struct _apl_action_free_t {
	ap_t ap;
} apl_action_free_t;

typedef enum _apl_action_t {
	apl_action_preget,
	apl_action_postget,
	apl_action_preset,
	apl_action_postset,
	apl_action_preinsert,
	apl_action_postinsert,
	apl_action_preremove,
	apl_action_postremove,
	apl_action_alloc,
	apl_action_free,
	apl_action_predump,
	apl_action_postdump
} apl_action_t;

typedef struct _apl_action_info_t {
	union {
		apl_action_get_t get;
		apl_action_set_t set;
		apl_action_insert_t insert;
		apl_action_remove_t remove;
		apl_action_alloc_t alloc;
		apl_action_free_t free;
	} data;
	apl_action_t action;
} apl_action_info_t;

typedef wstatus (*APLHOOKROUTINE)(apl_t ap,apl_action_info_t action_info,void *param);

wstatus apl_insert(apl_t self,ap_t ap,ap_index_t index);
wstatus apl_remove(apl_t self,ap_t ap,ap_index_t index);
wstatus apl_changed(apl_t self,bool *changed_flag);
wstatus apl_get_count(apl_t self,ap_index_t *count);
wstatus apl_get(apl_t self,ap_index_t index,ap_t *ap);
wstatus apl_alloc(apl_t *apl);
wstatus apl_free(apl_t apl);
wstatus apl_attach_hook(apl_t self,APLHOOKROUTINE routine,void *param);
wstatus apl_dettach_hook(apl_t self,APLHOOKROUTINE routine);
wstatus apl_dump(apl_t apl);

#endif

