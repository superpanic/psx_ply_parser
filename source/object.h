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

	DVECTOR *uvs;

	short numfaces; // faces
	short *faces; // face indices = numfaces x 4 (or x 3)

	short numcolors;
	CVECTOR *colors;
} Object;

#endif