#include "display.h"
#include "globals.h"

static DoubleBuff screen;
static u_short currbuff;

static void SetTheBackgroundClearColor(char r, char g, char b);

void ScreenInit(void) {
	// reset GPU
	ResetGraph(0);

	// buff 0
	SetDefDispEnv(&screen.disp[0], 0, 0, SCREEN_RES_X, SCREEN_RES_Y);
	SetDefDrawEnv(&screen.draw[0], 0, 240, SCREEN_RES_X, SCREEN_RES_Y);
	// buff 1
	SetDefDispEnv(&screen.disp[1], 0, 240, SCREEN_RES_X, SCREEN_RES_Y);
	SetDefDrawEnv(&screen.draw[1], 0, 0, SCREEN_RES_X, SCREEN_RES_Y);

	// set the back drawing buffer
	screen.draw[0].isbg = true;
	screen.draw[1].isbg = true;

	SetTheBackgroundClearColor(255, 255, 0);
	
	currbuff = 0;
	PutDispEnv(&screen.disp[currbuff]);
	PutDrawEnv(&screen.draw[currbuff]);

	InitGeom();
	SetGeomOffset(SCREEN_CENTER_X, SCREEN_CENTER_Y);
    	SetGeomScreen(SCREEN_Z);

	SetDispMask(1);
}

void DisplayFrame(void) {
	DrawSync(0);
	VSync(0);
	PutDispEnv(&screen.disp[currbuff]);
    	PutDrawEnv(&screen.draw[currbuff]);
	DrawOTag(GetOTAt(GetCurrentBuffer(),OT_LEN-1));
	currbuff = !currbuff;  
	ResetNextPrim(GetCurrentBuffer());
}

static void SetTheBackgroundClearColor(char r, char g, char b) {
	setRGB0(&screen.draw[0], r, g, b);
	setRGB0(&screen.draw[1], r, g, b);
}


u_short GetCurrentBuffer(void) {
	return currbuff;
}

// TODO:
// implement getters and setters for screen and for currbuff