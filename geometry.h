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
#define V1_SET(v,x1) v=x1;
#define V2_SET(v,x1,x2) v.x=x1; v.y=x2;
#define V3_SET(v,x1,x2,x3) v.x=x1; v.y=x2; v.z=x3;
#define V4_SET(v,x1,x2,x3,x4) v.x=x1; v.y=x2; v.z=x3; v.t=x4;

#define V2DPRINTF "(%0.2g,%0.2g)"
#define V3DPRINTF "(%0.2g,%0.2g,%0.2g)"
#define V4DPRINTF "(%0.2g,%0.2g,%0.2g,%0.2g)"

#endif

