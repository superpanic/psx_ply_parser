#include "globals.h"
#include "libgpu.h"

static u_long ot[2][OT_LEN];
static char   primbuff[2][PRIMBUFF_LEN];
static char   *nextprim;

void EmptyOT(u_short currbuff) {
	ClearOTagR(ot[currbuff], OT_LEN);
}

void SetOTAt(u_short currbuff, u_int index, u_long value) {
	ot[currbuff][index] = value;
}

u_long *GetOTAt(u_short currbuff, u_int index) {
	return &ot[currbuff][index];
}

void IncrementNextPrim(u_int size) {
	nextprim += size;
}

void ResetNextPrim(u_short currbuff) {
	nextprim = primbuff[currbuff];
}

void SetNextPrim(char *value) {
	nextprim = value;
}

char *GetNextPrim(void) {
	return nextprim;
}