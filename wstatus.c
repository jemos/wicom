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

#include "wstatus.h"

/*
   wstatus_str

   Function that returns a string that corresponds to the wstatus ws.
*/
const char *wstatus_str(wstatus ws)
{
	switch(ws)
	{
		case WSTATUS_FAILURE:
			return "WSTATUS_FAILURE";
		case WSTATUS_SUCCESS:
			return "WSTATUS_SUCCESS";
		case WSTATUS_SEMIFAIL:
			return "WSTATUS_SEMIFAIL";
		case WSTATUS_UNIMPLEMENTED:
			return "WSTATUS_UNIMPLEMENTED";
		case WSTATUS_UNSUPPORTED:
			return "WSTATUS_UNSUPPORTED";
		case WSTATUS_INVALID_ARGUMENT:
			return "WSTATUS_INVALID_ARGUMENT";
		case WSTATUS_MOD_UNINITIALIZED:
			return "WSTATUS_MOD_UNINITIALIZED";
		default:
			return "(UNKNOW WSTATUS)";
	}
}

