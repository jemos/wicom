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

	Copyright (C) 2009 Jean-François Mousinho <jean.mousinho@ist.utl.pt>
	Centro de Informatica do IST - Universidade Tecnica de Lisboa 
*/
/*
	Description: this set of functions and data structures are related to
	the management of name-value pairs data structures. Notice that all
	this functions use the parsed and validated name-value pair. You've
	however, functions available here to validate the name or the value
	specifically.
*/

#ifndef _NVPAIR_H
#define _NVPAIR_H

#include "wstatus.h"
#include "debug.h"
#include "jmlist.h"
#include "wlock.h"

#define NVP_ENCODED_PREFIX '#'
#define V_NAMECHAR(x) isalnum(x)
#define V_ENCPREFIX(x) (x == NVP_ENCODED_PREFIX)
#define V_NVSEPCHAR(x) (x == '=')
#define V_VALUECHAR(x) (isalnum(x) || (x == '.') || (x == ':') || (x == '_') || (x == '-')) 
#define V_QVALUECHAR(x) (V_VALUECHAR(x) || (x == ' '))
#define V_EVALUECHAR(x) (isdigit(x) || (x == 'A') || (x == 'B') || (x == 'C') || (x == 'D') || (x == 'E') || (x == 'F'))
#define V_QUOTECHAR(x) (x == '"')
#define nvp_flag_test(x,f) ((x & f) == f) 

/* nvpair_t: this data structure must have well defined sizes */
typedef struct _nvpair_t
{
	char *name_ptr;
	uint16_t name_size;
	void *value_ptr;
	uint16_t value_size;
} *nvpair_t;

#define NVPAIR_FLAG_UNINITIALIZED 0x80000000

/* NAME FLAGS */
typedef enum _nvpair_nflag_list
{
	NVPAIR_NFLAG_VALID		= 0x10000001,
	NVPAIR_NFLAG_OVERSIZE	= 0x10000002,
	NVPAIR_NFLAG_NOT_SET	= 0x10000004
} nvpair_nflag_list;

/* VALUE FLAGS */
typedef enum _nvpair_vflag_list
{
	NVPAIR_VFLAG_VALID		= 0x20000001,
	NVPAIR_VFLAG_OVERSIZE	= 0x20000002,
	NVPAIR_VFLAG_NOT_SET	= 0x20000004, /* name1 or name1="" or name1= */
} nvpair_vflag_list;

/* VALUE FORMAT FLAGS */
typedef enum _nvpair_fflag_list
{
	NVPAIR_FFLAG_UNQUOTED	= 0x30000001,
	NVPAIR_FFLAG_QUOTED		= 0x30000002,
	NVPAIR_FFLAG_ENCODED	= 0x30000004
} nvpair_fflag_list;

/* NVPAIR LOOKUP FLAGS */
typedef enum _nvpair_lflag_list
{
	NVPAIR_LFLAG_NOT_FOUND	= 0x40000001,
	NVPAIR_LFLAG_FOUND		= 0x40000002
} nvpair_lflag_list;

typedef int nvpair_iflag_list;

typedef struct _nvpair_info_t
{
	const char *name_ptr;
	unsigned int name_size;
	const char *value_ptr;
	unsigned int value_size;
	unsigned int encoded_size;
	unsigned int decoded_size;
	nvpair_iflag_list flags;
} *nvpair_info_t;

wstatus _nvp_alloc(uint16_t name_size,uint16_t value_size,nvpair_t *nvp);
wstatus _nvp_fill(const char *name_ptr,const uint16_t name_size,const void *value_ptr,const uint16_t value_size,nvpair_t nvp);
wstatus _nvp_free(nvpair_t nvp);
wstatus _nvp_validate_name(const char *name_ptr,const unsigned int name_size,nvpair_nflag_list *nflags);
wstatus _nvp_validate_value(const char *value_ptr,const unsigned int value_size,nvpair_vflag_list *vflags);
wstatus _nvp_value_format(const char *value_ptr,const unsigned int value_size,nvpair_fflag_list *fflags);
wstatus _nvp_value_encode(const char *value_ptr,const unsigned int value_size,char **value_encoded);
wstatus _nvp_value_encoded_size(const char *value_ptr,const unsigned int value_size,unsigned int *encoded_size);
wstatus _nvp_value_decode(const char *value_ptr,const uint16_t value_size,char *decoded_ptr,unsigned int decoded_size);
wstatus _nvp_value_decoded_size(const char *value_ptr,const uint16_t value_size,unsigned int *decoded_size);
wstatus _nvp_dup(const nvpair_t nvp,nvpair_t *new_nvp);

/* debugging functions */
void _nvp_print_value(const char *value_ptr, unsigned int value_size);
const char *nvp_iflags_str(nvpair_iflag_list iflags);
const char *nvp_nflags_str(nvpair_nflag_list nflags);
const char *nvp_vflags_str(nvpair_vflag_list vflags);
const char *nvp_fflags_str(nvpair_fflag_list fflags);
const char *nvp_lflags_str(nvpair_lflag_list lflags);

#endif

