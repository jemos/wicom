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

#ifndef _APPOWER_H
#define _APPOWER_H

#include "wstatus.h"

typedef enum _apwr_param_t {
	apwr_param_ofdm,
	apwr_param_cck,
	apwr_param_client
} apwr_param_t;
#define APWR_PARAM_COUNT 3

typedef struct _apwr_t {
	double ofdm;
	double cck;
	double client;
	bool param_set[APWR_PARAM_COUNT];
	bool change_flag[APWR_PARAM_COUNT];
} *apwr_t;

typedef enum _apwr_action_t {
	apwr_action_preget,
	apwr_action_postget,
	apwr_action_preset,
	apwr_action_postset,
	apwr_action_prechange,
	apwr_action_postchange
} apwr_action_t;


typedef struct _apwr_get_info_t {
	apwr_param_t param;
	double actual_value;
	bool param_set;
} apwr_get_info_t;

typedef struct _apwr_set_info_t {
	apwr_param_t param;
	double value_from;
	double value_to;
	bool param_set;
	bool change_flag;
} apwr_set_info_t;

typedef struct _apwr_change_info_t {
	apwr_param_t param;
	double value_from;
	double value_to;
	bool param_set;
} apwr_change_info_t;

typedef struct _apwr_action_info_t {
	union {
		apwr_get_info_t get;
		apwr_set_info_t set;
		apwr_change_info_t change;
	} data;
	apwr_action_t action;
} apwr_action_info_t;

wstatus apwr_get(apwr_t self,apwr_param_t param,void *value_ptr,size_t value_size);
wstatus apwr_set(apwr_t self,apwr_param_t param,void *value_ptr,size_t value_size);
wstatus apwr_attach_hook(apwr_t self,APWRHOOKROUTINE routine,void *param);
wstatus apwr_dettach_hook(apwr_t self,APWRHOOKROUTINE routine);
wstatus apwr_was_changed(apwr_t self,bool *changed_flag);
wstatus apwr_isset(apwr_t self,bool *set_flag);

#endif

