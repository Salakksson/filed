#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <curses.h>
#include <termios.h>
#include <limits.h>
#include <ctype.h>

#include "da.h"
#include "directory.h"

typedef struct
{
	char* path;
} config;

#define control(c) (c & ~0x60)

#define RESERVED_LINES 2

void _info(WINDOW* wind, const char* fmt, va_list args)
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
		printw("%s ", e.perms);
		printw("%*d ", cwd.longest_links, e.n_links);
		printw("%s ", e.owner);
		printw("%s ", e.group);
		if (e.sz_unit == 'B')
		{
			printw("%5d ", (int)e.sz_amount);
		}
		else
		{
			printw("%4.1f", e.sz_amount);
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

void cleanup()
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

	static bool first_init = true;
	if (first_init)
	{
		first_init = false;
		tcgetattr(STDIN_FILENO, &original_termios);
		atexit(cleanup);
	}

	struct termios t;
	tcgetattr(STDIN_FILENO, &t);
	t.c_lflag &= ~(ISIG);
	tcsetattr(STDIN_FILENO, TCSANOW, &t);

	return wind;
}

void exec_file(WINDOW* wind, directory* cwd, const char* path)
{
	if (is_dir(path))
	{
		change_dir(cwd, path);
		return;
	}

	pid_t pid = fork();
	if (pid == -1)
	{
		info(wind, "failed to fork: %s", strerror(errno));
		return;
	}
	if (pid == 0)
	{
		execlp("xdg-open", "xdg-open", path, (char*)NULL);
		info(wind, "failed to xdg-open '%s': %s", strerror(errno));
		return;
	}

}

void refresh_cwd(directory* cwd)
{
	clear();
	change_dir(cwd, ".");
}

int main(int argc, char** argv)
{
	config conf = {0};
	conf.path = ".";
	for (int i = 1; argv[i]; i++)
	{
		conf.path = argv[i];
	}

	WINDOW* wind = init_window();

	directory cwd = {0};
	change_dir(&cwd, conf.path);
	draw_screen(wind, cwd);
	char c;
	while ((c = getch()))
	{
		char* entry = cwd.entries.items[cwd.current + cwd.scroll].name;
		switch (c)
		{
		case 'p':
			if (cwd.current + cwd.scroll <= 0)
			{
				info(wind, "reached top of directory");
			}
			else if (cwd.current <= 0)
			{
				cwd.scroll--;
			}
			else cwd.current--;
			break;
		case 'n':
			if (cwd.current + cwd.scroll + 1 >= cwd.entries.len)
			{
				info(wind, "reached end of directory");
			}
			else if (cwd.current + 1 >= LINES - RESERVED_LINES)
			{
				cwd.scroll++;
			}
			else cwd.current++;
			break;
		case '\n':
			info(wind, "cding into %s", entry);
			exec_file(wind, &cwd, entry);
			break;
		case control('v'):
			cwd.scroll++;
			break;
		case control('f'):
			if (cwd.scroll > 0)
				cwd.scroll--;
			break;
		case 'd':
		{
			char input = confirm(wind, "delete '%s'? (y/N)", entry);
			if (toupper(input) != 'Y')
				break;
			if (remove(entry) == 0)
				info(wind, "deleted '%s' successfully", entry);
			else
				info(wind, "failed to delete '%s': %s", entry, strerror(errno));
			change_dir(&cwd, ".");
			 break;
		}
		case 'g':
			refresh_cwd(&cwd);
			break;
		case control('c'):
			goto leave;
		default:
			break;
		}
		draw_screen(wind, cwd);
	}
leave:
	change_dir(&cwd, "");
}
