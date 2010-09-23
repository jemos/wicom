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
#define V_NVSEPCHAR(x) (x == '=')
#define V_VALUECHAR(x) (isalnum(x) || (x == '.') || (x == ':') || (x == '_') || (x == '-')) 
#define V_QVALUECHAR(x) (V_VALUECHAR(x) || (x == ' '))
#define V_EVALUECHAR(x) (isdigit(x) || (x == 'A') || (x == 'B') || (x == 'C') || (x == 'D') || (x == 'E') || (x == 'F'))
#define V_QUOTECHAR(x) (x == '"')

/* nvpair_t: this data structure must have well defined sizes */
typedef struct _nvpair_t
{
	char *name_ptr;
	uint16_t name_size;
	void *value_ptr;
	uint16_t value_size;
} *nvpair_t;

typedef enum _nvpair_info_flag_list
{
	NVPAIR_IFLAG_UNINITIALIZED = 1024,
	NVPAIR_IFLAG_NOT_FOUND = 1,
	NVPAIR_IFLAG_FOUND = 2,
	
	NVPAIR_IFLAG_NAME_OVERSIZE = 4,
	NVPAIR_IFLAG_NAME_VALID = 64,
	NVPAIR_IFLAG_NAME_NOT_SET = 128,

	NVPAIR_IFLAG_VALUE_VALID = 256,
	NVPAIR_IFLAG_VALUE_OVERSIZE = 8,
	NVPAIR_IFLAG_VALUE_NOT_SET = 16,		/* name1 */
	NVPAIR_IFLAG_VALUE_EMPTY = 32,		/* name1="" or name1=# */
} nvpair_info_flag_list;

typedef enum _value_format_list {
	VALUE_FORMAT_UNINITIALIZED = 128,
	VALUE_FORMAT_UNQUOTED = 0,
	VALUE_FORMAT_QUOTED = 1,
	VALUE_FORMAT_ENCODED = 2
} value_format_list;

typedef struct _nvpair_info_t
{
	const char *name_ptr;
	unsigned int name_size;
	const char *value_ptr;
	unsigned int value_size;
	unsigned int encoded_size;
	unsigned int decoded_size;
	value_format_list value_format;
	nvpair_info_flag_list flags;
} *nvpair_info_t;


wstatus _nvp_alloc(uint16_t name_size,uint16_t value_size,nvpair_t *nvp);
wstatus _nvp_fill(const char *name_ptr,const uint16_t name_size,const void *value_ptr,const uint16_t value_size,nvpair_t nvp);
wstatus _nvp_free(nvpair_t nvp);
wstatus _nvp_value_format(const char *value_ptr,const unsigned int value_size,value_format_list *value_format);
wstatus _nvp_value_encode(const char *value_ptr,const unsigned int value_size,char **value_encoded);
wstatus _nvp_value_encoded_size(const char *value_ptr,const unsigned int value_size,unsigned int *encoded_size);
wstatus _nvp_value_decode(const char *value_ptr,const uint16_t value_size,char *decoded_ptr,unsigned int decoded_size);
wstatus _nvp_value_decoded_size(const char *value_ptr,const uint16_t value_size,unsigned int *decoded_size);
wstatus _nvp_dup(const nvpair_t nvp,nvpair_t *new_nvp);

/* debugging functions */
void _nvp_print_value(const char *value_ptr, unsigned int value_size);

#endif

