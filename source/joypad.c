#include "joypad.h"
#include "libetc.h"
#include <sys/types.h>

static u_long padstate;

int JoyPadCheck(int p) {
	return padstate & p;
};

void JoyPadInit(void) {
	PadInit(0);
};

void JoyPadReset(void) {
	padstate = 0;
};

void JoyPadUpdate(void) {
	u_long pad = PadRead(0);
	padstate = pad;
};