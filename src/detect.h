#ifndef DETECT_H
#define DETECT_H

#include "libc.h"
#include "buffer.h"

char *detect_interpreter(Buffer *b);
bool detect_indent(Buffer *b);

#endif
