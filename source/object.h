#ifndef OBJECT_H
#define OBJECT_H

#include <sys/types.h>
#include "globals.h"

typedef struct Object {
	SVECTOR rotation;
	VECTOR position;
	VECTOR scale;

	VECTOR vel;
	VECTOR acc;

	short numverts;
	SVECTOR *vertices;

	short numfaces; // faces
	short *faces; // face indices = numfaces * 4

	short numcolors;
	CVECTOR *colors;
} Object;

#endif