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
   Module Description

   This module provides an abstraction layer between wicom and a
   graphics interface. wicom requires a window to display maps and
   basic shapes, that type of GUI interface might be created
   differently depending on the OS so I decided to create this
   module which abstracts that code.

   There might be various wview's available but as first version
   I'll only add viewgl which relates to OpenGL wview.

   The wview to use is decided in compilation using preprocessor
   code which changes a function list structure. This function list
   provides the basic functions to use the wview (create shapes,
   initialize and cleanup).

   -- Jean Mousinho (Feb. 2010)
*/

#ifndef _WVIEW_H
#define _WVIEW_H

#endif

