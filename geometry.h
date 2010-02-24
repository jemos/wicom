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

#ifndef _GEOMETRY_H
#define _GEOMETRY_H

typedef double v1d_t;

typedef struct _v2d_t {
	double x,y;
} v2d_t;

typedef struct _v3d_t {
	double x,y,z;
} v3d_t;

typedef struct _v4d_t {
	double x,y,z,t;
} v4d_t;

typedef unsigned int v1u_t;

typedef struct _v2u_t {
	unsigned int x,y;
} v2u_t;

typedef struct _v3u_t {
	unsigned int x,y,z;
} v3u_t;

typedef struct _v4u_t {
	unsigned int x,y,z,t;
} v4u_t;

/* these macros are type independent and are simply helpers */
#define V1_SET(v,x) v=x;
#define V2_SET(v,x,y,z) v.x=x; v.y=y;
#define V3_SET(v,x,y,z) v.x=x; v.y=y; v.z=z;
#define V4_SET(v,x,y,z,t) v.x=x; v.y=y; v.z=z; v.t = t;

#endif

