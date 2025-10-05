#include <stdlib.h>
#include <libgte.h>
#include <libetc.h>
#include <libgpu.h>
#include <stdbool.h>

#define VIDEO_MODE (0)
#define SCREEN_RES_X (320)
#define SCREEN_RES_Y (240)
#define SCREEN_CENTER_X (SCREEN_RES_X>>1)
#define SCREEN_CENTER_Y (SCREEN_RES_Y>>1)

#define OT_LENGTH 16

typedef struct {
    DRAWENV draw[2];
    DISPENV disp[2];  
} DoubleBuff;

DoubleBuff screen;
u_short currbuff;

u_long ot[2][OT_LENGTH];

char primbuff[2][2048];
char *nextprim;

POLY_F3 *f_triangle;
TILE    *tile;
POLY_G4 *g_quad;

void ScreenInit(void) {
    // reset GPU
    ResetGraph(0);

    // set the display area of the first buffer
    SetDefDispEnv(&screen.disp[0], 0, 0, SCREEN_RES_X, SCREEN_RES_Y);
    SetDefDrawEnv(&screen.draw[0], 0, 240, SCREEN_RES_X, SCREEN_RES_Y);

    // set the display area of the second buffer
    SetDefDispEnv(&screen.disp[1], 0, 240, SCREEN_RES_X, SCREEN_RES_Y);
    SetDefDrawEnv(&screen.draw[1], 0, 0, SCREEN_RES_X, SCREEN_RES_Y);

    // set the back drawing buffer
    screen.draw[0].isbg = true;
    screen.draw[1].isbg = true;

    // set the background clear color
    setRGB0(&screen.draw[0], 63, 0, 127);
    setRGB0(&screen.draw[1], 63, 0, 127);

    // set the current initial buffer
    currbuff = 0;
    PutDispEnv(&screen.disp[currbuff]);
    PutDrawEnv(&screen.draw[currbuff]);

    InitGeom();
    SetGeomOffset(SCREEN_CENTER_X, SCREEN_CENTER_Y);
    SetGeomScreen(SCREEN_CENTER_X);

    SetDispMask(1);
}

void DisplayFrame(void) {
    DrawSync(0);
    VSync(0);

    PutDispEnv(&screen.disp[currbuff]);
    PutDrawEnv(&screen.draw[currbuff]);

    // swap buffer
    currbuff = !currbuff;
}

void Setup(void) {
    ScreenInit();
}

void Update(void) {
    ClearOTagR(ot[currbuff], OT_LENGTH);

    tile = (TILE*) nextprim; // cast next primitive to TILE
    setTile(tile);
    setXY0(tile,82,32);
    setWH(tile,64,64);
    setRGB0(tile, 0, 255, 0);
    addPrim(ot[currbuff], tile); // "sorting" the tile into the OT
    nextprim += sizeof(TILE);

    
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
