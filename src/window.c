#include "window.h"

#include <unistd.h>
#include <termios.h>
#include <locale.h>

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

typedef DA(char) sv;

char* nreadline(WINDOW* wind, const char* prompt)
{
	int y, x;
	getyx(wind, y, x);

	sv before;
	sv after;
	da_construct(before, 15);
	da_construct(after, 15);
	da_append(before, '\0');
	da_append(after, '\0');

	while (true)
	{
		attron(COLOR_PAIR(ECOLOR_MSG));
		mvprintw(LINES - 1, 0, "%s » ", prompt);
		attroff(COLOR_PAIR(ECOLOR_MSG));

		printw("%s", before.items);

		int input_y, input_x;
		getyx(wind, input_y, input_x);

		printw("%s", after.items);

		clrtoeol();
		move(input_y, input_x);

		int c = getch();
		switch (c)
		{
		case KEY_RESIZE: break;
		case KEY_BACKSPACE:
			if (before.len <= 1) break;
			before.len--;
			before.items[before.len - 1] = '\0';
			break;
		case control('a'):
		{
			before.len--;
			for (int i = 0; i < after.len; i++)
			{
				da_append(before, after.items[i]);
			}
			sv temp = before;
			before = after;
			after = temp;
			before.len = 1;
			before.items[0] = '\0';
			break;
		}
		case control('e'):
		{
			before.len--;
			for (int i = 0; i < after.len; i++)
			{
				da_append(before, after.items[i]);
}
			after.len = 1;
			after.items[0] = '\0';
			break;
		}
		case control('f'):
		{
			if (after.len <= 1) break;
			char next = after.items[0];
			before.items[before.len - 1] = next;
			da_append(before, '\0');
			for (int i = 1; i < after.len; i++)
			{
				after.items[i - 1] = after.items[i];
			}
			after.len--;
			break;
		}
		case control('b'):
		{
			if (before.len <= 1) break;
			char prev = before.items[before.len - 2];
			for (int i = after.len - 1; i > 0; i--)
			{
				after.items[i] = after.items[i - 1];
			}
			after.items[0] = prev;
			da_append(after, '\0');
			before.items[before.len - 2] = '\0';
			before.len--;
			break;
		}
		case control('d'):
			if (after.len <= 1) break;
			for (int i = 1; i < after.len; i++)
			{
				after.items[i - 1] = after.items[i];
			}
			after.len--;
			break;
		case control('k'):
			after.len = 1;
			after.items[0] = '\0';
			break;
		case control('g'):
		case control('c'):
			free(before.items);
			free(after.items);
			move(y, x);
			return NULL;
		case '\n':
			before.len--;
			for (int i = 0; i < after.len; i++)
			{
				da_append(before, after.items[i]);
			}
			free(after.items);
			move(y, x);
			return before.items;
		default:
			before.items[before.len - 1] = c;
			da_append(before, '\0');
			break;
		}
	}
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
	setlocale(LC_ALL, "");

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
