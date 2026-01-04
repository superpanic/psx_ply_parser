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

POLY_FT3 *poly;

MATRIX worldmat = {0};
MATRIX viewmat = {0};

Camera camera;

Object obj;

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
	printf("picture x:%i y:%i\n", tim.prect->x, tim.prect->y);
	printf("picture w:%i h:%i\n", tim.prect->w, tim.prect->h);
	printf("clut x:%i y:%i\n", tim.crect->x, tim.crect->y);
	printf("clut w:%i h:%i\n", tim.crect->w, tim.crect->h);

	DrawSync(0);
	if(tim.mode & 0x8) {
		printf("TIM texture file format is 4-bit\n");
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

//	setVector(&camera.position, 500, -1000, -1200);
	setVector(&camera.position, 0, -1000, -1200);
	camera.lookat = (MATRIX){0};

	setVector(&obj.position, 0, 0, 0);
	setVector(&obj.rotation, 0, 0, 0);
	setVector(&obj.scale, ONE, ONE, ONE);
	setVector(&obj.vel, 0, 0, 0);
	setVector(&obj.acc, 0, 1, 0);

	LoadPly("\\MILK.PLY;1", &obj);
	LoadTexture("\\MILK.TIM;1");
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
	LookAt(&camera,&camera.position,&obj.position,&up);

	// world matrix transform
	RotMatrix(&obj.rotation, &worldmat);
	TransMatrix(&worldmat, &obj.position);
	ScaleMatrix(&worldmat, &obj.scale);

	// create the view matrix, combining the world matrix and the lookat matrix
	CompMatrixLV(&camera.lookat, &worldmat, &viewmat);

	SetRotMatrix(&viewmat);
	SetTransMatrix(&viewmat);

	for(int i=0, q=0; i<obj.numfaces * 3; i+=3, q++) {
		poly = (POLY_FT3*) GetNextPrim(); // flat triangle
		setPolyFT3(poly); // init a flat triangle
		setRGB0(poly, 255, 255, 255);

		poly->tpage = getTPage(tim.mode, 0, tim.prect->x, tim.prect->y);
		poly->clut = getClut(tim.crect->x, tim.crect->y);

		poly->u0 = (u_char)obj.uvs[obj.faces[i+0]].vx; poly->v0 = (u_char)obj.uvs[obj.faces[i+0]].vy;
		poly->u1 = (u_char)obj.uvs[obj.faces[i+1]].vx; poly->v1 = (u_char)obj.uvs[obj.faces[i+1]].vy;
		poly->u2 = (u_char)obj.uvs[obj.faces[i+2]].vx; poly->v2 = (u_char)obj.uvs[obj.faces[i+2]].vy;
		
		nclip = RotAverageNclip3(
			&obj.vertices[obj.faces[i+0]], 
			&obj.vertices[obj.faces[i+1]], 
			&obj.vertices[obj.faces[i+2]],
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
			IncrementNextPrim(sizeof(POLY_FT3));
		}
	}
	obj.rotation.vy += 20;

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
