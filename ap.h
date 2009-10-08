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

   Access point module has a structure associated, it contains some
   parameters that have c-classes associated also. this will allow 
   a dynamic control of these parameters. for example, the c-class 
   of the power (apwr_t) might allow in the future to change
   directly the OFDM signal power of the access point directly when
   a apwr_set() is called (by using SNMP f.ex).
*/

#ifndef _AP_H
#define _AP_H

#include "wstatus.h"
#include "mapcoord.h"
#include "appower.h"
#include "apip.h"
#include "apserial.h"

typedef const char* ap_name_t;

typedef struct _ap_t {
	ap_name_t name;
	map_coord_t coord;
	apwr_t power;
	apip_t ip;
	aps_t serial;
	bool param_set[AP_PARAM_COUNT];
	bool change_flag[AP_PARAM_COUNT];
} *ap_t;

/* params aquired from the AP itself should have a c-class
   type, otherwise its common type. */
typedef enum _ap_params_t {
	AP_COORD,	/* map_coord_t */
	AP_POWER,	/* apwr_t */
	AP_IP,		/* apip_t */
	AP_NAME,	/* ap_name_t */
	AP_SERIAL	/* aps_t */
} ap_params_t;
#define AP_PARAM_COUNT 5

typedef enum _ap_action_t {
	ap_action_alloc,
	ap_action_free,
	ap_action_preset,
	ap_action_preget,
	ap_action_postset,
	ap_action_postget,
	ap_action_predump,
	ap_action_postdump
} ap_action_t;

typedef struct _ap_action_info_alloc_t {
	ap_t *new_ap;
} ap_action_info_alloc_t;

typedef struct _ap_action_free_alloc_t {
	ap_t *ap;
} ap_action_info_free_t;

typedef struct _ap_action_get_t {
	ap_t *ap;
	ap_params_t param;
	void *value_ptr;
	size_t value_size;
} ap_action_get_t;

typedef struct _ap_action_set_t {
	ap_t *ap;
	ap_params_t param;
	void *value_ptr;
	size_t value_size;
} ap_action_set_t;

typedef struct _ap_action_info_t {
	union {
		ap_action_info_alloc_t alloc;
		ap_action_info_free_t free;
		ap_action_info_preset_t set;
		ap_action_info_postset_t set;
		ap_action_info_preget_t get;
		ap_action_info_postget_t get;
	} data;
	ap_action_t action;
} ap_action_info_t;

typedef wstatus (*APHOOKROUTINE)(ap_t ap,ap_action_info_t action_info,void *param);

wstatus ap_set(ap_t ap,ap_params_t param,void *value_ptr,size_t value_size);
wstatus ap_get(ap_t ap,ap_params_t param,void *value_ptr,size_t value_size);
wstatus ap_param_isset(ap_t ap,ap_params_t param,bool *set_flag);
wstatus ap_param_changed(ap_t ap,ap_params_t param,bool *change_flag);
wstatus ap_dump(ap_t self);
wstatus ap_alloc(ap_t *ap);
wstatus ap_free();
wstatus ap_attach_hook(ap_t self,APHOOKROUTINE routine,void *param);
wstatus ap_dettach_hook(ap_t self,APHOOKROUTINE routine);

#endif // _AP_H
