#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <curses.h>
#include <termios.h>
#include <limits.h>
#include <ctype.h>

#include "da.h"
#include "directory.h"
#include "window.h"

typedef struct
{
	char* path;
} config;

#define control(c) (c & ~0x60)

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
