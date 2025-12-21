#ifndef CAMERA_H
#define CAMERA_H

#include "globals.h"
#include <libgte.h>

typedef struct Camera {
	VECTOR position;
	SVECTOR rotation;
	MATRIX lookat;
} Camera;

void LookAt(Camera *camera, VECTOR *eye, VECTOR *target, VECTOR *up);

#endif