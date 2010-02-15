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

#ifndef _PLANT_H
#define _PLANT_H

#include "geometry.h"

typedef unsigned int plantid_t;

typedef enum _plant_visibility_t {
	PLANT_VISIBLE,
	PLANT_HIDDEN
} plant_visibility_t;

typedef struct _plant_t {
	char description[128];
	v3d_t translate;
	v3d_t scale;
	plant_visibility_t visibility;
	plantid_t plantid;
} plant_t;

#endif

