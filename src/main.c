#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <termios.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <ctype.h>

#include "da.h"

#define KILOBYTE (off_t)1024
#define MEGABYTE (KILOBYTE*KILOBYTE)
#define GIGABYTE (MEGABYTE*KILOBYTE)
#define TERABYTE (GIGABYTE*KILOBYTE)

typedef struct
{
	char* path;
} config;

typedef struct
{
	char* perms;
	int n_links;
	char* owner;
	char* group;
	float sz_amount;
	char sz_unit;
	char* date;
	char* name;
	char* link;
} entry;

typedef struct
{
	char* path;
	DA(entry) entries;
	int longest_links;
	int longest_owner;
	int longest_group;
	int longest_date;
	int y, x;
	int current;
	int scroll;
} directory;

static inline char control(char c)
{
	return c & ~0x60;
}

void _info(WINDOW* wind, const char* fmt, va_list args)
{
	int y, x;
	getyx(wind, y, x);

	char buf[256];
	vsnprintf(buf, sizeof(buf), fmt, args);

	attron(COLOR_PAIR(2));
	mvprintw(LINES - 1, 0, "%s", buf);
	attroff(COLOR_PAIR(2));
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

int intlen(int n)
{
	int digits = 0;
	if (n < 0)
	{
		n = -n;
		digits++;
	}
	while (n)
	{
		n /= 10;
		digits++;
	}
	return digits;
}

void change_dir(directory* cwd, const char* path)
{
	path = strdup(path);
	if (cwd->path)
	{
		for (int i = 0; i < cwd->entries.len; i++)
		{
			entry e = cwd->entries.items[i];
			free(e.owner);
			free(e.group);
			free(e.name);
			free(e.perms);
			free(e.date);
		}
		free(cwd->entries.items);
		free(cwd->path);
	}
	cwd->path = realpath(path, 0);
	chdir(cwd->path);
	da_construct(cwd->entries, 10);

	cwd->current = 1;

	cwd->longest_group = 1;
	cwd->longest_owner = 1;
	cwd->longest_links = 1;
	cwd->longest_date = 1;

	DIR* dir = opendir(cwd->path);
	struct dirent* dir_entry;
	while ((dir_entry = readdir(dir)))
	{
		entry e = {0};
		struct stat st = {0};
		if (lstat(dir_entry->d_name, &st) == -1)
			fatal("failed to stat file");
		e.n_links = st.st_nlink;
		if (st.st_size < KILOBYTE)
		{
			e.sz_unit = 'B';
			e.sz_amount = st.st_size;
		}
		else if (st.st_size < MEGABYTE)
		{
			e.sz_unit = 'K';
			e.sz_amount = st.st_size / KILOBYTE;
		}
		else if (st.st_size < GIGABYTE)
		{
			e.sz_unit = 'M';
			e.sz_amount = st.st_size / MEGABYTE;
		}
		else if (st.st_size < TERABYTE)
		{
			e.sz_unit = 'G';
			e.sz_amount = st.st_size / GIGABYTE;
		}
		else
		{
			e.sz_unit = 'T';
			e.sz_amount = st.st_size / TERABYTE;
		}
		struct passwd* owner = getpwuid(st.st_uid);
		e.owner = strdup(owner->pw_name);
		struct group* grp = getgrgid(st.st_gid);
		e.group = strdup(grp->gr_name);

		mode_t m = st.st_mode;
		e.perms = strdup("----------");
		if (S_ISDIR(m)) e.perms[0] = 'd';
		else if (S_ISLNK(m)) e.perms[0] = 'l';
		else if (S_ISCHR(m)) e.perms[0] = 'c';
		else if (S_ISBLK(m)) e.perms[0] = 'b';
		else if (S_ISSOCK(m)) e.perms[0] = 's';
		else if (S_ISFIFO(m)) e.perms[0] = 'p';

		if (m & S_IRUSR) e.perms[1] = 'r';
		if (m & S_IWUSR) e.perms[2] = 'w';
		if (m & S_IXUSR) e.perms[3] = 'x';
		if (m & S_IRGRP) e.perms[4] = 'r';
		if (m & S_IWGRP) e.perms[5] = 'w';
		if (m & S_IXGRP) e.perms[6] = 'x';
		if (m & S_IROTH) e.perms[7] = 'r';
		if (m & S_IWOTH) e.perms[8] = 'w';
		if (m & S_IXOTH) e.perms[9] = 'x';

		e.date = malloc(20);
		struct tm* mod_time = localtime(&st.st_mtime);
		strftime(e.date, 20, "%b %d %H:%M", mod_time);

		e.name = strdup(dir_entry->d_name);

		if (S_ISLNK(m))
		{
			char buf[PATH_MAX + 1];
			ssize_t len = readlink(e.name, buf, PATH_MAX);
			if (len == -1)
				fatal("failed to readlink");
			buf[len] = 0;
			e.link = strdup(buf);
		}
		else e.link = 0;

		// TODO: check for errors in stat and getpwuid

		if (intlen(e.n_links) > cwd->longest_links)
			cwd->longest_links = intlen(e.n_links);
		if (strlen(e.owner) > cwd->longest_owner)
			cwd->longest_group = strlen(e.owner);
		if (strlen(e.group) > cwd->longest_group)
			cwd->longest_group = strlen(e.group);
		if (strlen(e.date) > cwd->longest_date)
			cwd->longest_date = strlen(e.date);

		da_append(cwd->entries, e);
	}
	da_sort_entry(cwd->entries);
}

void draw_screen(WINDOW* wind, directory cwd)
{
	mvprintw(0, 0, "%s", cwd.path);
	clrtoeol();
	printw("\n");

	for (int i = 0; i < LINES - 2; i++)
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

		attron(COLOR_PAIR(1));
		printw("%s", e.name);
		attroff(COLOR_PAIR(1));
		if (e.link)
		{
			attron(COLOR_PAIR(2));
			printw(" -> %s", e.link);
			attroff(COLOR_PAIR(2));
		}
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

	init_pair(1, COLOR_CYAN, -1);
	init_pair(2, COLOR_MAGENTA, -1);

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

bool is_dir(const char* path)
{
	// TODO check for errors when stating
	struct stat st;
	stat(path, &st);

	return S_ISDIR(st.st_mode);
}

void exec_file(WINDOW* wind, directory* cwd, const char* path)
{
	if (is_dir(path))
		return change_dir(cwd, path);

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
		if (c == 'p')
		{
			if (cwd.current + cwd.scroll <= 0)
			{
				info(wind, "todo: wrapping to bottom");
			}
			else if (cwd.current <= 0)
			{
				cwd.scroll--;
			}
			else cwd.current--;
		}
		if (c == 'n')
		{
			if (cwd.current + cwd.scroll + 1 >= cwd.entries.len)
			{
				info(wind, "todo: wrapping to top");
			}
			else if (cwd.current + 1 >= LINES - 2)
			{
				cwd.scroll++;
			}
			else cwd.current++;
		}
		if (c == '\n')
		{
			char* entry = cwd.entries.items[cwd.current + cwd.scroll].name;
			info(wind, "cding into %s", entry);
			exec_file(wind, &cwd, entry);
		}
		if (c == control('v'))
		{
			cwd.scroll++;
		}
		if (c == control('f'))
		{
			if (cwd.scroll > 0)
				cwd.scroll--;
		}
		if (c == 'd')
		{
			char* entry = cwd.entries.items[cwd.current + cwd.scroll].name;
			char input = confirm(wind, "delete '%s'? (y/N)", entry);
			if (toupper(input) == 'Y')
			{
				if (remove(entry) == 0)
					info(wind, "deleted '%s' successfully", entry);
				else
					info(wind, "failed to delete '%s'", entry);
			}
			refresh_cwd(&cwd);
		}
		if (c == 'g')
		{
			refresh_cwd(&cwd);
		}
		if (c == control('c')) exit(0);
		draw_screen(wind, cwd);
	}
}
