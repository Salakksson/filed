#ifndef WINDOW_H_
#define WINDOW_H_

#include <curses.h>

#include "directory.h"

#define RESERVED_LINES 2

#define control(c) (c & ~0x60)

void info(WINDOW* wind, const char* fmt, ...);

char confirm(WINDOW* wind, const char* fmt, ...);

__attribute__((format(printf, 2, 3)))
__attribute__((malloc))
char* nreadline(WINDOW* wind, const char* fmt, ...);

void draw_screen(WINDOW* wind, directory cwd);

void close_window(void);

WINDOW* init_window(void);



#endif
