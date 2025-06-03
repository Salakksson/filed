#include "window.h"

#include <unistd.h>
#include <termios.h>

static void _info(WINDOW* wind, const char* fmt, va_list args)
{
	int y, x;
	getyx(wind, y, x);

	char buf[256];
	vsnprintf(buf, sizeof(buf), fmt, args);

	attron(COLOR_PAIR(ECOLOR_MSG));
	mvprintw(LINES - 1, 0, "%s", buf);
	attroff(COLOR_PAIR(ECOLOR_MSG));
	clrtoeol();
	refresh();
	move(y, x);
}

void info(WINDOW* wind, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	_info(wind, fmt, args);
	va_end(args);
}

char confirm(WINDOW* wind, const char* fmt, ...)
{
	int y, x;
	getyx(wind, y, x);

	va_list args;
	va_start(args, fmt);
	_info(wind, fmt, args);
	va_end(args);

	char c = getch();
	move(LINES -1, 0);
	clrtoeol();
	move(y, x);
	return c;
}

void draw_screen(WINDOW* wind, directory cwd)
{
	attron(COLOR_PAIR(ECOLOR_HEAD));
	mvprintw(0, 0, "%s:", cwd.path);
	attroff(COLOR_PAIR(ECOLOR_HEAD));

	clrtoeol();
	printw("\n");

	for (int i = 0; i < LINES - RESERVED_LINES; i++)
	{
		clrtoeol();
		if (i + cwd.scroll >= cwd.entries.len)
		{
			printw("\n");
			continue;
		}
		entry e = cwd.entries.items[i + cwd.scroll];

		attron(COLOR_PAIR(ECOLOR_MARKED));
		if (e.marked)
			printw("- ");
		else
			printw("  ");
		attroff(COLOR_PAIR(ECOLOR_MARKED));

		printw("%s ", e.perms);
		printw("%*d ", cwd.longest_links, e.n_links);
		printw("%s ", e.owner);
		printw("%s ", e.group);
		if (e.sz_unit == 'B')
		{
			printw("%4d ", (int)e.sz_amount);
		}
		else
		{
			if (e.sz_amount >= 10)
				printw("%3d", (int)e.sz_amount);
			else
				printw("%3.1f", e.sz_amount);
			printw("%c ", e.sz_unit);
		}
		printw("%s ", e.date);

		if (i == cwd.current)
			getyx(wind, cwd.y, cwd.x);

		attron(COLOR_PAIR(e.color));
		printw("%s", e.name);
		attroff(COLOR_PAIR(e.color));
		if (e.link)
			printw(" -> %s", e.link);

		printw("\n");
	}
	refresh();
	move(cwd.y, cwd.x);
}

static struct termios original_termios;

void close_window()
{
	tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
	endwin();
}

WINDOW* init_window()
{
	WINDOW* wind = initscr();
	start_color();
	use_default_colors();
	raw();
	cbreak();
	keypad(stdscr, TRUE);
	noecho();

	init_pair(ECOLOR_FILE, COLOR_WHITE, -1);
	init_pair(ECOLOR_DIR, COLOR_BLUE, -1);
	init_pair(ECOLOR_LNK, COLOR_CYAN, -1);
	init_pair(ECOLOR_EXE, COLOR_GREEN, -1);

	init_pair(ECOLOR_MSG, COLOR_YELLOW, -1);
	init_pair(ECOLOR_HEAD, COLOR_YELLOW, -1);
	init_pair(ECOLOR_MARKED, COLOR_YELLOW, -1);

	static bool first_init = true;
	if (first_init)
	{
		first_init = false;
		tcgetattr(STDIN_FILENO, &original_termios);
		atexit(close_window);
	}

	struct termios t;
	tcgetattr(STDIN_FILENO, &t);
	t.c_lflag &= ~(ISIG);
	tcsetattr(STDIN_FILENO, TCSANOW, &t);

	return wind;
}
