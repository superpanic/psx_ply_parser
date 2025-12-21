#include "camera.h"
#include "libgte.h"

void VectorCross(VECTOR *a, VECTOR *b, VECTOR *out) {
	OuterProduct12(a, b, out);
}

void LookAt(Camera *camera, VECTOR *eye, VECTOR *target, VECTOR *up) {
	VECTOR xright;
	VECTOR yup;
	VECTOR zforward;
	VECTOR x,y,z;
	VECTOR pos;
	VECTOR t;

	// compute the z-forward vector and normalize it
	zforward.vx = target->vx - eye->vx;
	zforward.vy = target->vy - eye->vy;
	zforward.vz = target->vz - eye->vz;
	VectorNormal(&zforward, &z);

	// compute the right vector and normalize it	
	VectorCross(&z, up, &xright);
	VectorNormal(&xright, &x);

	// compute the yup vector and normalize it
	VectorCross(&z, &x, &yup);
	VectorNormal(&yup, &y);

	// rotation
	camera->lookat.m[0][0] = x.vx; camera->lookat.m[0][1] = x.vy; camera->lookat.m[0][2] = x.vz;
	camera->lookat.m[1][0] = y.vx; camera->lookat.m[1][1] = y.vy; camera->lookat.m[1][2] = y.vz;
	camera->lookat.m[2][0] = z.vx; camera->lookat.m[2][1] = z.vy; camera->lookat.m[2][2] = z.vz;

	// position
	pos.vx = -eye->vx;
	pos.vy = -eye->vy;
	pos.vz = -eye->vz;

	ApplyMatrixLV(&camera->lookat, &pos, &t);
	TransMatrix(&camera->lookat, &t);
}