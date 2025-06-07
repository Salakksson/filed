#include "directory.h"
#include "window.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>

typedef struct
{
	char* path;
} config;

__attribute__((format(printf, 1, 2)))
__attribute__((malloc))
char* stralloc(const char* fmt, ...)
{
	va_list args, copy;
	va_start(args, fmt);
	va_copy(copy, args);

	size_t sz = vsnprintf(0, 0, fmt, args);
	char* buf = malloc(sz + 1);
	if (!buf) fatal("failed to alloc");
	vsnprintf(buf, sz + 1, fmt, copy);
	return buf;
}

__attribute__((malloc))
char* get_command(const char* path)
{
	if (access(path, X_OK) == 0)
	{
		return stralloc("exec '%s' > /dev/null 2>&1 &", path);
	}
	char* ext = strrchr(path, '.');
	if (!ext) return NULL;
	if (
		!strcmp(ext, ".mp4") ||
		!strcmp(ext, ".mp3") ||
		!strcmp(ext, ".wav") ||
		!strcmp(ext, ".jpg") ||
		!strcmp(ext, ".png") ||
		!strcmp(ext, ".webm")
	) return stralloc("mpv --loop '%s' > /dev/null 2>&1 &", path);
	if (
		!strcmp(ext, ".c")
	) return stralloc("emacsclient -c '%s' > /dev/null 2>&1 &", path);
	return NULL;
}

bool exec_file(WINDOW* wind, directory* cwd, const char* path)
{
	if (is_dir(path))
	{
		change_dir(cwd, path);
		return true;
	}

	char* command = get_command(path);
	if (!command)
	{
		char* app = nreadline(wind, "open '%s' with", path);
		if (!app) return true;
		command = stralloc("%s '%s' > /dev/null 2>&1 &", app, path);
		free(app);
	}
	int status = system(command);
	free(command);

	return (status != -1) && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

void refresh_cwd(directory* cwd)
{
	clear();
	change_dir(cwd, ".");
}

void delete_entries(WINDOW* wind, directory* cwd, entry* selected)
{
	bool marked = false;
	for (int i = 0; i < cwd->entries.len; i++)
	{
		if (!cwd->entries.items[i].marked) continue;
		marked = true;
		break;
	}

	char input;
	if (marked)
		input = confirm(wind, "delete marked files? (y/N)");
	else
		input = confirm(wind, "delete '%s'? (y/N)", selected->name);

	if (toupper(input) != 'Y')
		return;

	if (marked)
	{
		bool success = true;
		for (int i = 0; i < cwd->entries.len; i++)
		{
			entry e = cwd->entries.items[i];
			if (!e.marked) continue;

			if (!remove_recursive(e.name))
				success = false;
			if (!success)
			{
				info(wind, "failed to delete '%s': %s",
				     selected->name, strerror(errno));
				break;
			}

		}
		if (success)
			info(wind, "deleted '%s' successfully", selected->name);
	}
	else
	{
		bool success = remove_recursive(selected->name);
		if (success)
			info(wind, "deleted '%s' successfully", selected->name);
		else
			info(wind, "failed to delete '%s': %s",
			     selected->name, strerror(errno));
	}
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
