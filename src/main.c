#include "filed.h"
#include <sys/stat.h>

void refresh_cwd(directory* cwd)
{
	clear();
	change_dir(cwd, ".");
}

int main(int argc, char** argv)
{
	char* start_path = ".";
	for (int i = 1; argv[i]; i++)
	{
		start_path = argv[i];
	}

	WINDOW* wind = init_window();

	directory cwd = {0};
	change_dir(&cwd, start_path);
	draw_screen(wind, cwd);
	int c;
	while ((c = getch()))
	{
		move(LINES - 1, 0);
		clrtoeol();
		entry* e = &cwd.entries.items[cwd.current + cwd.scroll];
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
			exec_file(wind, &cwd, e->name);
			break;
		case 'd':
			delete_entries(wind, &cwd, e);
			change_dir(&cwd, ".");
			break;
		case 's':
			cwd.soft = !cwd.soft;
			refresh_cwd(&cwd);
			break;
		case 'r':
		{
			char* new_name = nreadline(wind, "rename '%s' to", e->name);
			int success = rename(e->name, new_name);
			if (success == 0)
				info(wind, "successfully renamed");
			else
				info(wind, "failed to rename '%s' to '%s': %s",
				     e->name, new_name, strerror(errno));
			refresh_cwd(&cwd);
			break;
		}
		case 'g':
		case KEY_RESIZE:
			refresh_cwd(&cwd);
			break;
		case 'm':
			e->marked = !e->marked;
			break;
		case 'o':
		{
			char* path = nreadline(wind, "open");
			if (!path) break;
			char* expanded = expand_home(path);
			free(path);
			if (!is_dir(expanded))
			{
				info(wind, "'%s' is not a valid path", expanded);
				free(expanded);
				break;
			}
			change_dir(&cwd, expanded);
			free(expanded);
			break;
		}
		case '+':
		{
			char* path = nreadline(wind, "create path");
			if (!path) break;
			if (mkdir(path, 0755) != 0)
				info(wind, "failed to create '%s': %s",
				     path, strerror(errno));
			else
				info(wind, "created directory '%s'", path);
			free(path);
			refresh_cwd(&cwd);
			break;
		}
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
