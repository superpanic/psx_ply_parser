#ifndef UTILS_h
#define UTILS_H

#include <sys/types.h>
#include "object.h"

char *FileRead(char *filename, u_long *length);
void LoadPly(char *filename, Object *obj);

char GetChar(u_char *bytes, u_long *b);

short GetShortLE(u_char *bytes, u_long *b);
short GetShortBE(u_char *bytes, u_long *b);

long GetLongLE(u_char *bytes, u_long *b);
long GetLongBE(u_char *bytes, u_long *b);

#endif