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

#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "debug.h"
#include "nvpair.h"

wstatus
_nvp_alloc(uint16_t name_size,uint16_t value_size,nvpair_t *nvp)
{
	nvpair_t new_nvp;

	dbgprint(MOD_NVPAIR,__func__,"called with name_size=%u, value_size=%u, nvp=%p",
			name_size,value_size,nvp);

	if( !name_size ) {
		dbgprint(MOD_NVPAIR,__func__,"invalid argument name_size, size > 0 is required");
		goto return_fail;
	}

	if( !nvp ) {
		dbgprint(MOD_NVPAIR,__func__,"invalid argument nvp (nvp=0)");
		goto return_fail;
	}

	/* got everything we need to allocate the new nvp */

	new_nvp = (nvpair_t) malloc(sizeof(struct _nvpair_t));
	if( !new_nvp ) {
		dbgprint(MOD_NVPAIR,__func__,"malloc failed");
		goto return_fail;
	}

	/* initialize stucture */

	new_nvp->name_ptr = 0;
	new_nvp->name_size = 0;
	new_nvp->value_ptr = 0;
	new_nvp->value_size = 0;

	new_nvp->name_ptr = (char*)malloc(name_size);
	if( !new_nvp->name_ptr ) {
		dbgprint(MOD_NVPAIR,__func__,"malloc failed");
		goto return_fail_malloc;
	}
	dbgprint(MOD_NVPAIR,__func__,"allocated %u bytes for name of nvpair=%p",
			name_size,new_nvp);

	new_nvp->name_size = name_size;
	dbgprint(MOD_NVPAIR,__func__,"changed nvpair (%p) name size to %u",
			new_nvp,new_nvp->name_size);

	if( value_size )
	{
		new_nvp->value_ptr = malloc(value_size);
		if( !new_nvp->value_ptr ) {
			dbgprint(MOD_NVPAIR,__func__,"malloc failed");
			goto return_fail_malloc;
		}
		dbgprint(MOD_NVPAIR,__func__,"allocated %d bytes for value of nvpair=%p",
				value_size,new_nvp);

		new_nvp->value_size = value_size;
		dbgprint(MOD_NVPAIR,__func__,"changed nvpair (%p) value size to %u",
				new_nvp,new_nvp->value_size);
	}

	dbgprint(MOD_NVPAIR,__func__,"updating argument nvp to %p",nvp);
	*nvp = new_nvp;
	dbgprint(MOD_NVPAIR,__func__,"argument nvp updated successfully to %p",*nvp);

	DBGRET_SUCCESS(MOD_NVPAIR);

return_fail_malloc:
	if( new_nvp->name_ptr ) {
		free(new_nvp->name_ptr);
	}

	if( new_nvp->value_ptr ) {
		free(new_nvp->value_ptr);
	}

	free(new_nvp);

return_fail:
	DBGRET_FAILURE(MOD_NVPAIR);
}

/*
   _nvp_fill

   Helper function to fill a name-value pair data structure.
*/
wstatus
_nvp_fill(const char *name_ptr,const uint16_t name_size,const void *value_ptr,const uint16_t value_size,nvpair_t nvp)
{
	char *decoded_ptr = 0;

	dbgprint(MOD_NVPAIR,__func__,"called with nvp=%p, name_ptr=%p, name_size=%u, value_ptr=%p, value_size=%u",
			nvp,name_ptr,name_size,value_ptr,value_size);

	if( !nvp ) {
		dbgprint(MOD_NVPAIR,__func__,"invalid argument nvp (nvp=0)");
		goto return_fail;
	}

	if( !name_ptr || !name_size ) {
		dbgprint(MOD_NVPAIR,__func__,"invalid argument name_ptr or name_size=0");
		goto return_fail;
	}

	/* compare name_size and value_size with the values in nvp */
	
	if( nvp->name_size != name_size ) {
		dbgprint(MOD_NVPAIR,__func__,"inconsistent values of name_size (nvp->name_size=%u, name_size=%u)",
				nvp->name_size,name_size);
		goto return_fail;
	}

	if( nvp->value_size != value_size ) {
		dbgprint(MOD_NVPAIR,__func__,"inconsistent values of value_size (nvp->value_size=%u, value_size=%u)",
				nvp->value_size,value_size);
		goto return_fail;
	}

	dbgprint(MOD_NVPAIR,__func__,"copying %u bytes of the name to nvp structure (%p)",name_size,nvp);
	memcpy(nvp->name_ptr,name_ptr,name_size);
	dbgprint(MOD_NVPAIR,__func__,"copied %d bytes successfully of the name to the nvp structure (%p)",name_size,nvp);

	if( !nvp->value_size ) {
		DBGRET_SUCCESS(MOD_NVPAIR);
	}

	dbgprint(MOD_NVPAIR,__func__,"copying %u bytes of the value to nvp structure (%p)",value_size,nvp);
	memcpy(nvp->value_ptr,value_ptr,value_size);
	dbgprint(MOD_NVPAIR,__func__,"copied %d bytes successfully of the value to the nvp structure (%p)",value_size,nvp);

	DBGRET_SUCCESS(MOD_NVPAIR);

return_fail:
	if( decoded_ptr )
		free(decoded_ptr);

	DBGRET_FAILURE(MOD_NVPAIR);
}

/*
   _nvp_free

   Helper function to free the name-value pair data structure from memory.
*/
wstatus
_nvp_free(nvpair_t nvp)
{
	dbgprint(MOD_NVPAIR,__func__,"called with nvp=%p");

	if( !nvp ) {
		dbgprint(MOD_NVPAIR,__func__,"invalid argument nvp (nvp=0)");
		goto return_fail;
	}

	if( nvp->name_ptr ) {
		dbgprint(MOD_NVPAIR,__func__,"(nvp=%p) freeing name_ptr=%p",nvp,nvp->name_ptr);
		free(nvp->name_ptr);
	}

	if( nvp->value_ptr ) {
		dbgprint(MOD_NVPAIR,__func__,"(nvp=%p) freeing value_ptr=%p",nvp,nvp->value_ptr);
		free(nvp->value_ptr);
	}

	DBGRET_SUCCESS(MOD_NVPAIR);

return_fail:
	DBGRET_FAILURE(MOD_NVPAIR);
}

/*
   _nvp_value_decoded_size

   Helper function to get decoded value size.
*/
wstatus
_nvp_value_decoded_size(const char *value_ptr,const uint16_t value_size,unsigned int *decoded_size)
{
	dbgprint(MOD_NVPAIR,__func__,"called with value_ptr=%p, value_size=%u, decoded_size=%p",
			value_ptr,value_size,decoded_size);

	if( !value_size ) {
		dbgprint(MOD_NVPAIR,__func__,"value is empty (value_size=0)");
		DBGRET_FAILURE(MOD_NVPAIR);
	}

	if( !decoded_size ) {
		dbgprint(MOD_NVPAIR,__func__,"invalid decoded_size argument (decoded_size=0)");
		DBGRET_FAILURE(MOD_NVPAIR);
	}

	if( !(value_size & 1) ) {
		dbgprint(MOD_NVPAIR,__func__,"invalid value_size (odd number expected)");
		DBGRET_FAILURE(MOD_NVPAIR);
	}

	*decoded_size = (value_size - 1)/2;
	dbgprint(MOD_NVPAIR,__func__,"updated decoded_size value to %u",*decoded_size);

	DBGRET_SUCCESS(MOD_NVPAIR);
}

/*
   _nvp_value_decode

   Helper function to decode a value to its original format.
   The decoded_size argument tells how big is the decoded_ptr buffer.
*/
wstatus
_nvp_value_decode(const char *value_ptr,const uint16_t value_size,char *decoded_ptr,unsigned int decoded_size)
{
	unsigned int i,j,k;
	char hex_str[3];

	dbgprint(MOD_NVPAIR,__func__,"called with value_ptr=%p, value_size=%u, decoded_ptr=%p, decoded_size=%u",
			value_ptr,value_size,decoded_ptr,decoded_size);

	if( !(value_size & 1) ) {
		dbgprint(MOD_NVPAIR,__func__,"invalid value_size (odd number expected)");
		DBGRET_FAILURE(MOD_NVPAIR);
	}

	/* i starts at 1 to skip the ENCPREFIXCHAR */
	for( i = 1, j = 0 ; i < value_size ; i+= 2 )
	{
		hex_str[0] = value_ptr[i];
		hex_str[1] = value_ptr[i+1];
		hex_str[2] = '\0';

		if( sscanf(hex_str,"%X",&k) != 1 ) {
			dbgprint(MOD_NVPAIR,__func__,"couldn't convert %s to binary",hex_str);
			DBGRET_FAILURE(MOD_NVPAIR);
		}

		if( j >= decoded_size ) {
			dbgprint(MOD_NVPAIR,__func__,"decoded buffer is too small for value");
			DBGRET_FAILURE(MOD_NVPAIR);
		}

		decoded_ptr[j++] = (char) (k & 0xFF);
	}

	DBGRET_SUCCESS(MOD_NVPAIR);
}

/*
   _nvp_print_value

   Helper function to print the value of a nvpair. Values can contain special chars or even unprintable
   characters. This function supports different printing formats.
*/
void _nvp_print_value(const char *value_ptr, unsigned int value_size)
{
	unsigned int i;

	for( i = 0 ; i < value_size ; i++ )
	{
		if( isprint(value_ptr[i]) )
			putchar(value_ptr[i]);
		else
			putchar('?');
	}

	return;
}

/*
   _nvp_value_format

   Helper function that decides the format to use in the value of the name-value pair.
*/
wstatus
_nvp_value_format(const char *value_ptr, const unsigned int value_size,nvpair_fflag_list *fflags)
{
	unsigned int i;
	nvpair_fflag_list l_value_format = NVPAIR_FFLAG_UNQUOTED;
	nvpair_fflag_list format_transform[][3] = {
		{ NVPAIR_FFLAG_UNQUOTED , NVPAIR_FFLAG_QUOTED , NVPAIR_FFLAG_ENCODED },
		{ NVPAIR_FFLAG_QUOTED , NVPAIR_FFLAG_QUOTED , NVPAIR_FFLAG_ENCODED },
		{ NVPAIR_FFLAG_ENCODED , NVPAIR_FFLAG_ENCODED , NVPAIR_FFLAG_ENCODED }
	}; /* format_transform[FORM][TO] */

	dbgprint(MOD_NVPAIR,__func__,"called with value_ptr=%p, value_size=%u",value_ptr,value_size);

	if( !value_ptr ) {
		dbgprint(MOD_NVPAIR,__func__,"invalid value_ptr argument (value_ptr=0)");
		goto return_fail;
	}

	if( !value_size ) {
		dbgprint(MOD_NVPAIR,__func__,"value is empty (value_size=0), any format will do");
		goto return_fail;
	}

	if( !fflags ) {
		dbgprint(MOD_NVPAIR,__func__,"invalid fflags argument (fflags=0)");
		goto return_fail;
	}

	for( i = 0 ; i < value_size ; i++ )
	{
		if( V_VALUECHAR(value_ptr[i]) ) {
			l_value_format = format_transform[l_value_format][NVPAIR_FFLAG_UNQUOTED];
			continue;
		}

		if( V_QVALUECHAR(value_ptr[i]) ) {
			l_value_format = format_transform[l_value_format][NVPAIR_FFLAG_QUOTED];
			continue;
		}

		l_value_format = NVPAIR_FFLAG_ENCODED;
		break;
	}

	dbgprint(MOD_NVPAIR,__func__,"finished processing value bytes, result is format %d",l_value_format);

	*fflags = l_value_format;
	dbgprint(MOD_NVPAIR,__func__,"updated fflags value to %d",*fflags);

	DBGRET_SUCCESS(MOD_NVPAIR);

return_fail:
	DBGRET_FAILURE(MOD_NVPAIR);
}

/*
	_nvp_value_encoded_size

	Helper function to obtain the length in characters of the encoded value.
	This length includes the null char and is 1 based (starts at one).
*/
wstatus
_nvp_value_encoded_size(const char *value_ptr,const unsigned int value_size,unsigned int *encoded_size)
{
	dbgprint(MOD_NVPAIR,__func__,"called with value_ptr=%p, value_size=%u, encoded_size=%p",
			value_ptr,value_size,encoded_size);

	if( !value_ptr ) {
		dbgprint(MOD_NVPAIR,__func__,"invalid value_ptr argument (value_ptr=0)");
		goto return_fail;
	}

	if( !value_size ) {
		dbgprint(MOD_NVPAIR,__func__,"this value is empty (value_size=0)");
		goto return_fail;
	}

	if( !encoded_size ) {
		dbgprint(MOD_NVPAIR,__func__,"invalid encoded_size argument (encoded_size=0)");
		goto return_fail;
	}

	*encoded_size = (value_size * 2) + 1;
	dbgprint(MOD_NVPAIR,__func__,"updated encoded_size value to %u",*encoded_size);

	DBGRET_SUCCESS(MOD_NVPAIR);

return_fail:
	DBGRET_FAILURE(MOD_NVPAIR);
}

/*
   _nvp_value_encode

   Helper function to encode the value data into a text string, here we've various options,
   one could use base64, uenc, etc. I'm going to use HEX encoding for now its the easiest to
   implement and to determine the final size, basically is the double.
*/
wstatus
_nvp_value_encode(const char *value_ptr,const unsigned int value_size,char **value_encoded)
{
	char *aux_ptr;
	unsigned int i;
	char c;
	char conv_table[] = {"0123456789ABCDEF"};

	dbgprint(MOD_NVPAIR,__func__,"called with value_ptr=%p, value_size=%u, value_encoded=%p",
			value_ptr,value_size,value_encoded);

	aux_ptr = (char*)malloc(sizeof(char)*value_size*2 + 2*sizeof(char));
	if( !aux_ptr ) {
		dbgprint(MOD_NVPAIR,__func__,"malloc failed (size=%d)",sizeof(char)*value_size*2 + 2*sizeof(char));
		goto return_fail;
	}
	dbgprint(MOD_NVPAIR,__func__,"allocated buffer successfully (ptr=%p)",aux_ptr);

	*value_encoded = aux_ptr;

	dbgprint(MOD_NVPAIR,__func__,"inserting encoded prefix");

	*aux_ptr = NVP_ENCODED_PREFIX;
	aux_ptr++;

	dbgprint(MOD_NVPAIR,__func__,"starting the conversion of %d bytes",value_size);
	for( i = 0 ; i < value_size ; i++ )
	{
		c = value_ptr[i];

		*aux_ptr = conv_table[ (c & 0xF0) >> 4 ];
		aux_ptr++;
		*aux_ptr = conv_table[ (c & 0x0F) ];
		aux_ptr++;
	}
	dbgprint(MOD_NVPAIR,__func__,"finished conversion of %d bytes, aux_ptr has offset +%d bytes",
			value_size,aux_ptr - *value_encoded);

	/* cat the null char */
	*aux_ptr = '\0';

	DBGRET_SUCCESS(MOD_NVPAIR);

return_fail:
	DBGRET_FAILURE(MOD_NVPAIR);
}

/*
   _nvp_dup

   Helper function to duplicate a nvpair data structure. To free the allocated
   nvpair data structure use the _nvp_free helper function.
*/
wstatus _nvp_dup(const nvpair_t nvp,nvpair_t *new_nvp)
{
	wstatus ws;
	nvpair_t aux_nvp = 0;

	dbgprint(MOD_NVPAIR,__func__,"called with nvp=%p, new_nvp=%p",nvp,new_nvp);

	/* validate arguments */

	if( !nvp ) {
		dbgprint(MOD_NVPAIR,__func__,"invalid nvp argument (nvp=0)");
		DBGRET_FAILURE(MOD_NVPAIR);
	}

	if( !new_nvp ) {
		dbgprint(MOD_NVPAIR,__func__,"invalid new_nvp argument (new_nvp=0)");
		DBGRET_FAILURE(MOD_NVPAIR);
	}

	ws = _nvp_alloc(nvp->name_size,nvp->value_size,&aux_nvp);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_NVPAIR,__func__,"failed to allocate new nvpair data structure "
				"(helper function failed with ws=%s)",wstatus_str(ws));
		DBGRET_FAILURE(MOD_NVPAIR);
	}
	dbgprint(MOD_NVPAIR,__func__,"allocated new nvpair data structure (ptr=%p)",aux_nvp);

	ws = _nvp_fill(nvp->name_ptr,nvp->name_size,nvp->value_ptr,nvp->value_size,aux_nvp);
	if( ws != WSTATUS_SUCCESS ) {
		dbgprint(MOD_NVPAIR,__func__,"failed to fill the new nvpair data structure "
				"(helper function failed with ws=%s)",wstatus_str(ws));

		goto return_fail;
	}
	dbgprint(MOD_NVPAIR,__func__,"new nvpair data structure was filled successfully");

	*new_nvp = aux_nvp;
	dbgprint(MOD_NVPAIR,__func__,"updated new_nvp value to %p",*new_nvp);

	DBGRET_SUCCESS(MOD_NVPAIR);

return_fail:

	if( aux_nvp )
   	{
		_nvp_free(aux_nvp);
		if( ws != WSTATUS_SUCCESS ) {
			dbgprint(MOD_NVPAIR,__func__,"failed to free the allocated nvpair data structure (ptr=%p)",aux_nvp);
		}
		aux_nvp = 0;
	}

	DBGRET_FAILURE(MOD_NVPAIR);
}

/*
   _nvp_name_validate

   Public function to validate array of characters defined by name_ptr, name_size as the name
   of a name-value pair. This function actually does a bit more than to validate, it returns
   a set of flags as the result of the name validation which includes whenever the name does
   actually contain any data, if is oversized, if it contains invalid characters or not.
*/
wstatus
_nvp_name_validate(const char *name_ptr,const unsigned int name_size,nvpair_nflag_list *nflags)
{
	unsigned int i;

	dbgprint(MOD_NVPAIR,__func__,"called with name_ptr=%p (\"%s\"), name_size=%u, nflags=%p",
			name_ptr,array2z(name_ptr,name_size),nflags);

	if( !nflags ) {
		dbgprint(MOD_NVPAIR,__func__,"invalid nflags argument (nvflags=0)");
		DBGRET_FAILURE(MOD_NVPAIR);
	}

	if( !name_size )
	{
		if( name_ptr ) {
			dbgprint(MOD_NVPAIR,__func__,"there shouldn't be a name pointer when name size is null");
			DBGRET_FAILURE(MOD_NVPAIR);
		}

		dbgprint(MOD_NVPAIR,__func__,"this value is empty, updating nflags");
		*nflags = NVPAIR_NFLAG_VALID | NVPAIR_NFLAG_NOT_SET;
		dbgprint(MOD_NVPAIR,__func__,"updated nflags value to %d (NVPAIR_NFLAG_VALID | NVPAIR_NFLAG_NOT_SET)",*nflags);

		DBGRET_SUCCESS(MOD_NVPAIR);
	}

	if( !name_ptr ) {
		dbgprint(MOD_NVPAIR,__func__,"invalid name_ptr argument (name_ptr=0)");
		DBGRET_FAILURE(MOD_NVPAIR);
	}

	/* validate name characters */

	for( i = 0 ; i < name_size ; i++ )
	{
		if( V_NAMECHAR(name_ptr[i]) )
			continue;

		/* found invalid character */

		dbgprint(MOD_NVPAIR,__func__,"invalid character (name[%u] = 0x%02X, '%c') found in name of nvpair",
				i,name_ptr[i],b2c(name_ptr[i]));

		goto return_invalid;
	}

	*nflags = NVPAIR_NFLAG_VALID;
	dbgprint(MOD_NVPAIR,__func__,"updated nflags value to %d (NVPAIR_NFLAG_VALID)",*nflags);
	DBGRET_SUCCESS(MOD_NVPAIR);

return_invalid:
	*nflags = 0;
	dbgprint(MOD_NVPAIR,__func__,"updated nflags value to %d",*nflags);
	DBGRET_SUCCESS(MOD_NVPAIR);
}

/*
   _nvp_value_validate

   Public function to validate an array of characters defined by value_ptr, value_size as the
   value of a name-value pair. This function actually does a bit more than to validate, it
   returns a set of flags as the result of the value validation which includes whenever the
   value is valid or not andif there's actually any value or not (it is not a mandatory field).
*/
wstatus
_nvp_value_validate(const char *value_ptr,const unsigned int value_size,nvpair_vflag_list *vflags)
{
	dbgprint(MOD_NVPAIR,__func__,"called with value_ptr=%p, value_size=%u, vflags=%p",
			value_ptr,value_size,vflags);

	if( !vflags ) {
		dbgprint(MOD_NVPAIR,__func__,"invalid vflags argument (vflags=0)");
		DBGRET_FAILURE(MOD_NVPAIR);
	}

	dbgprint(MOD_NVPAIR,__func__,"clearing vflags");
	*vflags = NVPAIR_FLAG_UNINITIALIZED;

	if( !value_size )
   	{
		if( value_ptr ) {
			dbgprint(MOD_NVPAIR,__func__,"there shouldn't be a value pointer when value size is null");
			DBGRET_FAILURE(MOD_NVPAIR);
		}

		dbgprint(MOD_NVPAIR,__func__,"this value is empty, updating vflags");
		*vflags = NVPAIR_VFLAG_VALID | NVPAIR_VFLAG_NOT_SET;
		dbgprint(MOD_NVPAIR,__func__,"updated vflags to %d (NVPAIR_VFLAG_VALID | NVPAIR_VFLAG_NOT_SET)",*vflags);

		DBGRET_SUCCESS(MOD_NVPAIR);
	}

	if( !value_ptr ) {
		dbgprint(MOD_NVPAIR,__func__,"invalid value_ptr argument (value_ptr=0)");
		DBGRET_FAILURE(MOD_NVPAIR);
	}

	/* we've a value, it can contain any characters because of the encoding */

	*vflags = NVPAIR_VFLAG_VALID;
	dbgprint(MOD_NVPAIR,__func__,"updated vflags to %d (NVPAIR_VFLAG_VALID | NVPAIR_VFLAG_NOT_SET)",*vflags);
	DBGRET_SUCCESS(MOD_NVPAIR);
}

/*
   nvp_iflags_str

   Debugging function to translate binary flags into a string.
*/
const char *nvp_iflags_str(nvpair_iflag_list iflags)
{
	const char szUnknown[] = {"UNKNOWN"};
	static char buffer[512];

	/* initialize buffer */
	strcpy(buffer,szUnknown);

	if( nvp_flag_test(iflags,NVPAIR_NFLAG_VALID) )
		strcat(buffer,",NVPAIR_NFLAG_VALID");

	if( nvp_flag_test(iflags,NVPAIR_NFLAG_OVERSIZE) )
		strcat(buffer,",NVPAIR_NFLAG_OVERSIZE");

	if( nvp_flag_test(iflags,NVPAIR_NFLAG_NOT_SET) )
		strcat(buffer,",NVPAIR_NFLAG_NOT_SET");

	if( nvp_flag_test(iflags,NVPAIR_VFLAG_VALID) )
		strcat(buffer,",NVPAIR_VFLAG_VALID");

	if( nvp_flag_test(iflags,NVPAIR_VFLAG_OVERSIZE) )
		strcat(buffer,",NVPAIR_VFLAG_OVERSIZE");

	if( nvp_flag_test(iflags,NVPAIR_VFLAG_NOT_SET) )
		strcat(buffer,",NVPAIR_VFLAG_NOT_SET");

	if( nvp_flag_test(iflags,NVPAIR_FFLAG_UNQUOTED) )
		strcat(buffer,",NVPAIR_FFLAG_UNQUOTED");

	if( nvp_flag_test(iflags,NVPAIR_FFLAG_QUOTED) )
		strcat(buffer,",NVPAIR_FFLAG_QUOTED");

	if( nvp_flag_test(iflags,NVPAIR_FFLAG_ENCODED) )
		strcat(buffer,",NVPAIR_FFLAG_ENCODED");

	if( nvp_flag_test(iflags,NVPAIR_LFLAG_NOT_FOUND) )
		strcat(buffer,",NVPAIR_LFLAG_NOT_FOUND");

	if( nvp_flag_test(iflags,NVPAIR_LFLAG_FOUND) )
		strcat(buffer,",NVPAIR_LFLAG_FOUND");

	if( buffer[sizeof(szUnknown) - 1] )
		return buffer + 8 + 1;
	else
		return buffer;
}

const char *nvp_fflags_str(nvpair_fflag_list fflags)
{
	return nvp_iflags_str((nvpair_iflag_list)fflags);
}

