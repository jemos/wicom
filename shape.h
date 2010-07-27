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

#ifndef _SHAPE_H
#define _SHAPE_H

#include "layer.h"
#include "plant.h"
#include "geometry.h"

typedef unsigned int shapeid_t;

#define MAX_SHAPE_TAG_SIZE 128

typedef struct _shape_point_t {
	v3d_t p1;
	v1d_t size;
	v3d_t color;
} shape_point_t;

typedef struct _shape_line_t {
	v3d_t p1,p2;
	v3d_t color;
} shape_line_t;

typedef struct _shape_polyline_t {
	v3d_t *point_list;
	unsigned int point_count;
	v3d_t color;
} shape_polyline_t;

typedef struct _shape_polygon_t {
	v3d_t *point_list;
	unsigned int point_count;
	v3d_t color;
} shape_polygon_t;

typedef enum _textfont_t {
	TEXT_FONT_SMALL,
	TEXT_FONT_NORMAL
} textfont_t;

typedef struct _shape_text_t {
	char *content;
	v3d_t position;
	v3d_t color;
	textfont_t font;
} shape_text_t;

typedef enum _shapetype_t {
	SHAPE_POINT,
	SHAPE_LINE,
	SHAPE_POLYLINE,
	SHAPE_POLYGON,
	SHAPE_TEXT
} shapetype_t;

typedef enum _shapevisibility_t {
	SHAPE_VISIBLE,
	SHAPE_HIDDEN
} shapevisibility_t;

typedef struct _shape_t {
	layer_t layer;
	plantid_t plantid;
	char shapetag[MAX_SHAPE_TAG_SIZE];
	shapetype_t type;
	shapevisibility_t visibility;
	union {
		shape_point_t point;
		shape_line_t line;
		shape_polyline_t polyline;
		shape_polygon_t polygon;
		shape_text_t text;
	} data;
	shapeid_t shapeid;
} shape_t;

#endif

