#ifndef WINDOW_H_
#define WINDOW_H_

#include <curses.h>

#include "directory.h"

#define RESERVED_LINES 2

#define control(c) (c & ~0x60)

void info(WINDOW* wind, const char* fmt, ...);

char confirm(WINDOW* wind, const char* fmt, ...);

char* nreadline(WINDOW* wind, const char* prompt);

void draw_screen(WINDOW* wind, directory cwd);

void close_window();

WINDOW* init_window();



#endif
