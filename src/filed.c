#include "filed.h"

/* #include <stdlib.h> */
#include <unistd.h>
/* #include <sys/stat.h> */
#include <ctype.h>

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
