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

POLY_FT4 *poly;

MATRIX worldmat = {0};
MATRIX viewmat = {0};

Camera camera;

Object object;

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

void LoadModel(char *filename) {
	char *bytes;
	u_long length;
	bytes = FileRead(filename, &length);
	printf("We read %lu bytes from MODEL.BIN\n", length);

	// start readng the MODEL.BIN file
	u_long byte_counter = 0; // byte counter;

	object.numverts = GetShortBE(bytes, &byte_counter);
	object.vertices = malloc3(object.numverts * sizeof(SVECTOR)); // SVECTOR contains 3 shorts vx,vy,vz and 1 padding short
	for(int i = 0; i < object.numverts; i++) {
		object.vertices[i].vx  = GetShortBE(bytes, &byte_counter);
		object.vertices[i].vy  = GetShortBE(bytes, &byte_counter);
		object.vertices[i].vz  = GetShortBE(bytes, &byte_counter);
		printf("VERTEX %d, X=%d, Y=%d, Z=%d\n", i, 
			object.vertices[i].vx,
			object.vertices[i].vy,
			object.vertices[i].vz
		);
	}

	// TODO: Read the face indices from the file
	object.numfaces = GetShortBE(bytes, &byte_counter);
	object.faces = malloc3(object.numfaces * sizeof(short) * 4); // each face has 4 indices
	for(int i = 0; i < object.numfaces; i++) {
		// multiply by 4, as each face has 4 indices
		object.faces[i*4+0] = GetShortBE(bytes, &byte_counter);
		object.faces[i*4+1] = GetShortBE(bytes, &byte_counter);
		object.faces[i*4+2] = GetShortBE(bytes, &byte_counter);
		object.faces[i*4+3] = GetShortBE(bytes, &byte_counter);
		printf("FACE %d, A=%d, B=%d, C=%d, D=%d\n", i, 
			object.faces[i*4+0], 
			object.faces[i*4+1], 
			object.faces[i*4+2], 
			object.faces[i*4+3]
		);
	}

	// TODO: Read the color values from the file
	object.numcolors = bytes[byte_counter++];
	printf("NUMBER OF COLORS: %d\n", object.numcolors);
	object.colors = malloc3(object.numcolors * sizeof(CVECTOR));
	for(int i=0; i<object.numcolors; i++) {
		object.colors[i].r =  GetChar(bytes, &byte_counter);
		object.colors[i].g =  GetChar(bytes, &byte_counter);
		object.colors[i].b =  GetChar(bytes, &byte_counter);
		object.colors[i].cd = GetChar(bytes, &byte_counter);
		printf("COLOR %d, R=%d G=%d B=%d CD=%d\n", i,
			object.colors[i].r,
			object.colors[i].g,
			object.colors[i].b,
			object.colors[i].cd
		);
	}
	free3(bytes);
}

void Setup(void) {
	HeapSize(0x5000); // 20480
	ScreenInit();
	CdInit();
	JoyPadInit();
	ResetNextPrim(GetCurrentBuffer());

	setVector(&camera.position, 500, -1000, -1200);
	camera.lookat = (MATRIX){0};

	setVector(&object.position, 0, 0, 0);
	setVector(&object.rotation, 0, 0, 0);
	setVector(&object.scale, ONE, ONE, ONE);
	setVector(&object.vel, 0, 0, 0);
	setVector(&object.acc, 0, 1, 0);

	LoadModel("\\MODEL.BIN;1");
	LoadTexture("\\METAL.TIM;1");

}

void Update(void) {
	int nclip;
	long otz, p, flg;

	EmptyOT(GetCurrentBuffer());

	JoyPadUpdate();

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

	VECTOR up = (VECTOR){0,-ONE,0};
	LookAt(&camera,&camera.position,&object.position,&up);

	// world matrix transform
	RotMatrix(&object.rotation, &worldmat);
	TransMatrix(&worldmat, &object.position);
	ScaleMatrix(&worldmat, &object.scale);

	// create the view matrix, combining the world matrix and the lookat matrix
	CompMatrixLV(&camera.lookat, &worldmat, &viewmat);

	SetRotMatrix(&viewmat);
	SetTransMatrix(&viewmat);

	for(int i=0, q=0; i<object.numfaces * 4; i+=4, q++) {
		poly = (POLY_FT4*) GetNextPrim();
		setPolyFT4(poly);
		setRGB0(poly, 128, 128, 128);

		poly->u0 =  0; poly->v0 =  0;
		poly->u1 = 63; poly->v1 =  0;
		poly->u2 =  0; poly->v2 =  63;
		poly->u3 = 63; poly->v3 =  63;

		poly->tpage = getTPage(tim.mode, 0, tim.prect->x, tim.prect->y);
		poly->clut = getClut(tim.crect->x, tim.crect->y);

		nclip = RotAverageNclip4(
			&object.vertices[object.faces[i+0]], 
			&object.vertices[object.faces[i+1]], 
			&object.vertices[object.faces[i+2]], 
			&object.vertices[object.faces[i+3]], 
			(long*)&poly->x0, 
			(long*)&poly->x1, 
			(long*)&poly->x2, 
			(long*)&poly->x3,
			&p, 
			&otz, 
			&flg
		);

		if(nclip <= 0) continue;

		if((otz > 0) && (otz < OT_LEN)) {
			addPrim(GetOTAt(GetCurrentBuffer(), otz), poly);
			IncrementNextPrim(sizeof(POLY_FT4));
		}
	}
	object.rotation.vy += 20;
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
