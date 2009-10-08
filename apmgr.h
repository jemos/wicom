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

typedef struct _apmgr_init_t {
	char junk;
} apmgr_init_t;

wstatus apmgr_initialize(apmgr_init_t apmgr);
wstatus apmgr_uninitialize();
wstatus apmgr_insert(apl_t ap_list,ap_t ap);
wstatus apmgr_replace_by_name(apl_t ap_list,char *name,ap_t new_ap);
wstatus apmgr_replace_by_index(apl_t ap_list,ap_index_t index,ap_t new_ap);
wstatus apmgr_remove_by_name(apl_t ap_list,char *name);
wstatus apmgr_remove_by_index(apl_t ap_list,ap_index_t index);
wstatus apmgr_lookup_by_ip(apl_t apl,apip_t ip,ap_t *ap);
wstatus apmgr_set(apmgr_param_t param,void *value_ptr,size_t value_size);
wstatus apmgr_get(apmgr_param_t param,void *value_ptr,size_t value_size);

#endif

