#ifndef WINDOW_H_
#define WINDOW_H_

#include <curses.h>

#include "directory.h"

#define RESERVED_LINES 2

void info(WINDOW* wind, const char* fmt, ...);

char confirm(WINDOW* wind, const char* fmt, ...);

void draw_screen(WINDOW* wind, directory cwd);

void close_window();

WINDOW* init_window();



#endif
