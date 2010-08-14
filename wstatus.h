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

#ifndef _WSTATUS_H
#define _WSTATUS_H

typedef enum _wstatus {
	WSTATUS_SUCCESS,
	WSTATUS_FAILURE,
	WSTATUS_SEMIFAIL,
	WSTATUS_UNIMPLEMENTED,
	WSTATUS_UNSUPPORTED,
	WSTATUS_INVALID_ARGUMENT,
	WSTATUS_MOD_UNINITIALIZED
} wstatus;

const char *wstatus_str(wstatus ws);

#endif

