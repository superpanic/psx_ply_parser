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
#define SCREEN_Z (320)

#define OT_LENGTH 256
#define NUM_VERTICES 8
#define NUM_FACES 12

SVECTOR vertices[] = {
    {-128,-128,-128},
    { 128,-128,-128},
    { 128,-128, 128},
    {-128,-128, 128},
    {-128, 128,-128},
    { 128, 128,-128},
    { 128, 128, 128},
    {-128, 128, 128}
};

short faces[] = {
    0,3,2, // top
    0,2,1, // top
    4,0,1, // front
    4,1,5, // front
    7,4,5, // bottom
    7,5,6, // bottom
    5,1,2, // right
    5,2,6, // right
    2,3,7, // back
    2,7,6, // back
    0,4,7, // left
    0,7,3 // left
};

typedef struct {
    DRAWENV draw[2];
    DISPENV disp[2];  
} DoubleBuff;

DoubleBuff screen;
u_short currbuff;

u_long ot[2][OT_LENGTH];

char primbuff[2][2048];
char *nextprim;

POLY_G3 *poly;

SVECTOR rotation = {0,0,0}; // short because rotation will be a smaller value
VECTOR translation = {0,0,900};
VECTOR scale = {ONE,ONE,ONE};

MATRIX world = {0};

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
    SetGeomScreen(SCREEN_Z);

    SetDispMask(1);
}

void DisplayFrame(void) {
    DrawSync(0);
    VSync(0);

    PutDispEnv(&screen.disp[currbuff]);
    PutDrawEnv(&screen.draw[currbuff]);

    DrawOTag(ot[currbuff] + OT_LENGTH - 1);

    // swap buffer
    currbuff = !currbuff;
    nextprim = primbuff[currbuff];
}

void Setup(void) {
    ScreenInit();
    nextprim = primbuff[currbuff];
}

void Update(void) {
    int nclip;
    long otz, p, flg;

    ClearOTagR(ot[currbuff], OT_LENGTH);

    RotMatrix(&rotation, &world);
    TransMatrix(&world, &translation);
    ScaleMatrix(&world, &scale);

    SetRotMatrix(&world);
    SetTransMatrix(&world);

    bool flip = true;

    for(int i=0; i<NUM_FACES * 3; i+=3) {
        poly = (POLY_G3*) nextprim;
        setPolyG3(poly);
        
        setRGB0(poly, 255,255,0);
        setRGB1(poly, 255*flip,255*!flip,255);
        setRGB2(poly, 255*!flip,255*flip,255);
        flip=!flip;
        
        // otz = 0;
        // otz += RotTransPers(&vertices[faces[i+0]], (long*)&poly->x0, &p, &flg);
        // otz += RotTransPers(&vertices[faces[i+1]], (long*)&poly->x1, &p, &flg);
        // otz += RotTransPers(&vertices[faces[i+2]], (long*)&poly->x2, &p, &flg);
        // otz /= 3;

        nclip = RotAverageNclip3(&vertices[faces[i+0]], &vertices[faces[i+1]], &vertices[faces[i+2]], (long*)&poly->x0, (long*)&poly->x1, (long*)&poly->x2, &p, &otz, &flg);
        if(nclip <= 0) continue;

        if((otz > 0) && (otz < OT_LENGTH)) {
            addPrim(ot[currbuff][otz], poly);
            nextprim += sizeof(POLY_G3);
        }
    }

    rotation.vx+=8;
    rotation.vy+=4;
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
