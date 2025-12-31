#include <stdlib.h>
#include <libgte.h>
#include <libetc.h>
#include <libgpu.h>
#include <stdbool.h>
#include <stdio.h>
#include "inline_n.h"
#include "display.h"
#include "joypad.h"
#include "globals.h"
#include "camera.h"
#include "libcd.h"
#include "utils.h"
#include "object.h"

extern char __heap_start, __sp;

POLY_F3 *poly;

MATRIX worldmat = {0};
MATRIX viewmat = {0};

Camera camera;

Object face;

TIM_IMAGE tim;

void HeapSize(int size) {
	InitHeap3((unsigned long *)(&__heap_start), (&__sp - size) - &__heap_start);
}

void LoadTexture(char *filename) {
	u_long *data;
	u_long length;
	//TIM_IMAGE tim; // is currently global
	data = (u_long *) FileRead(filename, &length);
	OpenTIM(data);
	ReadTIM(&tim);

	LoadImage(tim.prect, tim.paddr);
	DrawSync(0);
	if(tim.mode & 0x8) {
		LoadClut2(tim.caddr, tim.crect->x, tim.crect->y); // 16 bytes
	}

	free(data);
}

void Setup(void) {
	HeapSize(0x5000); // 20480
	ScreenInit();
	CdInit();
	JoyPadInit();
	ResetNextPrim(GetCurrentBuffer());

	setVector(&camera.position, 500, -1000, -1200);
	camera.lookat = (MATRIX){0};

	setVector(&face.position, 0, 0, 0);
	setVector(&face.rotation, 0, 0, 0);
	setVector(&face.scale, ONE, ONE, ONE);
	setVector(&face.vel, 0, 0, 0);
	setVector(&face.acc, 0, 1, 0);

	//LoadModel("\\MODEL.BIN;1");
	LoadPly("\\MILK.PLY;1", &face);
	LoadTexture("\\METAL.TIM;1");
}

void JoyPadCheckAll(void) {
	if(JoyPadCheck(PAD1_LEFT)) {
		camera.position.vx -= 50;
	}
	if(JoyPadCheck(PAD1_RIGHT)) {
		camera.position.vx += 50;
	}
	if(JoyPadCheck(PAD1_UP)) {
		camera.position.vy -= 50;
	}
	if(JoyPadCheck(PAD1_DOWN)) {
		camera.position.vy += 50;
	}
	if(JoyPadCheck(PAD1_CROSS)) {
		camera.position.vz += 50;
	}
	if(JoyPadCheck(PAD1_CIRCLE)) {
		camera.position.vz -= 50;
	}
}

void Update(void) {
	int nclip;
	long otz, p, flg;

	EmptyOT(GetCurrentBuffer());

	JoyPadUpdate();
	JoyPadCheckAll();
	
	VECTOR up = (VECTOR){0,-ONE,0};
	LookAt(&camera,&camera.position,&face.position,&up);

	// world matrix transform
	RotMatrix(&face.rotation, &worldmat);
	TransMatrix(&worldmat, &face.position);
	ScaleMatrix(&worldmat, &face.scale);

	// create the view matrix, combining the world matrix and the lookat matrix
	CompMatrixLV(&camera.lookat, &worldmat, &viewmat);

	SetRotMatrix(&viewmat);
	SetTransMatrix(&viewmat);

	for(int i=0, q=0; i<face.numfaces * 3; i+=3, q++) {
		poly = (POLY_F3*) GetNextPrim(); // flat triangle
		setPolyF3(poly); // init a flat triangle
		setRGB0(poly, 10*i, 255-(10*i), 128-(10*i));

		/*
			poly->u0 =  0; poly->v0 =  0;
			poly->u1 = 63; poly->v1 =  0;
			poly->u2 =  0; poly->v2 =  63;
		*/

		// poly->tpage = getTPage(tim.mode, 0, tim.prect->x, tim.prect->y);
		// poly->clut = getClut(tim.crect->x, tim.crect->y);

		nclip = RotAverageNclip3(
			&face.vertices[face.faces[i+0]], 
			&face.vertices[face.faces[i+1]], 
			&face.vertices[face.faces[i+2]],
			(long*)&poly->x0, 
			(long*)&poly->x1, 
			(long*)&poly->x2,
			&p, 
			&otz, 
			&flg
		);

		if(nclip <= 0) continue;

		if((otz > 0) && (otz < OT_LEN)) {
			addPrim(GetOTAt(GetCurrentBuffer(), otz), poly);
			IncrementNextPrim(sizeof(POLY_F3));
		}
	}
	face.rotation.vy += 20;

}

void Render(void) {
    DisplayFrame();
}

int main(void) {
	Setup();
	while (1) {
		Update();
		Render();
	}
	return 0;
}
