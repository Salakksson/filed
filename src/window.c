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

__attribute__((format(printf, 2, 3)))
__attribute__((malloc))
char* nreadline(WINDOW* wind, const char* fmt, ...)
{
	int y, x;
	getyx(wind, y, x);

	sv before;
	sv after;
	da_construct(before, 15);
	da_construct(after, 15);
	da_append(before, '\0');
	da_append(after, '\0');

	va_list args;
	va_start(args, fmt);

	while (true)
	{
		attron(COLOR_PAIR(ECOLOR_MSG));
		move(LINES - 1, 0);
		vw_printw(wind, fmt, args);
		printw(" » ");
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

#define LONGEST_MARK sizeof("-")
#define LONGEST_PERMS sizeof("drwxrwxrwx")
#define LONGEST_FILESIZE 4

void draw_screen(WINDOW* wind, directory cwd)
{
	attron(COLOR_PAIR(ECOLOR_HEAD));
	mvprintw(0, 0, "%s:", cwd.path);
	if (cwd.soft)
	{
		printw(" (");
		if (cwd.soft) printw("soft");
		printw(")");
	}
	attroff(COLOR_PAIR(ECOLOR_HEAD));

	clrtoeol();
	printw("\n");

	int len_all =
		LONGEST_MARK + 1 +
		LONGEST_PERMS + 1 +
		cwd.longest_links + 1 +
		cwd.longest_owner + 1 +
		cwd.longest_group + 1 +
		LONGEST_FILESIZE + 1 +
		cwd.longest_date + 1 +
		cwd.longest_name;

	// in soft mode remove entries in order of least significance
	int len_rm_links = len_all;
	int len_rm_group = len_rm_links - cwd.longest_links - 1;
	int len_rm_owner = len_rm_group - cwd.longest_group - 1;
	int len_rm_perms = len_rm_owner - cwd.longest_owner - 1;
	int len_rm_fsize = len_rm_perms - LONGEST_PERMS - 1;
	int len_rm_date = len_rm_fsize - LONGEST_FILESIZE - 1;

	int screen_space = COLS;

	bool draw_links = (len_rm_links <= screen_space) || !cwd.soft;
	bool draw_group = (len_rm_group <= screen_space) || !cwd.soft;
	bool draw_perms = (len_rm_perms <= screen_space) || !cwd.soft;
	bool draw_owner = (len_rm_owner <= screen_space) || !cwd.soft;
	bool draw_fsize = (len_rm_fsize <= screen_space) || !cwd.soft;
	bool draw_date = (len_rm_date <= screen_space) || !cwd.soft;

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

		if (draw_perms) printw("%s ", e.perms);
		if (draw_links) printw("%*d ", cwd.longest_links, e.n_links);
		if (draw_owner) printw("%s ", e.owner);
		if (draw_group) printw("%s ", e.group);
		if (draw_fsize)
		{
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
		}
		if (draw_date) printw("%s ", e.date);
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
static int original_stderr;
static int log_fd;

void close_window(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
	endwin();
	dup2(original_stderr, STDERR_FILENO);
	close(original_stderr);

	lseek(log_fd, 0, SEEK_SET);
	char buf[512];
	ssize_t n;
	while ((n = read(log_fd, buf, sizeof(buf))) > 0)
	{
		write(STDERR_FILENO, buf, n);
	}
	close(log_fd);
}

WINDOW* init_window(void)
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

		original_stderr = dup(STDERR_FILENO);
		char tempfile[] = "/tmp/filed.XXXXXX";
		log_fd = mkstemp(tempfile);
		if (log_fd == -1)
			fatal("failed to make tempfile: %s", strerror(errno));
		unlink(tempfile);
		dup2(log_fd, STDERR_FILENO);

		atexit(close_window);
	}

	struct termios t;
	tcgetattr(STDIN_FILENO, &t);
	t.c_lflag &= ~(ISIG);
	tcsetattr(STDIN_FILENO, TCSANOW, &t);

	return wind;
}

__attribute__((weak))
void __asan_on_error(void)
{
	close_window();
}
