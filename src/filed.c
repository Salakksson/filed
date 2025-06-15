#include "filed.h"

#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <libgen.h>

#define READ_SIZE 256

selected_entries get_selected(directory* dir)
{
	selected_entries se = {0};
	da_construct(se.entries, 5);

	for (int i = 0; i < dir->entries.len; i++)
	{
		entry e = dir->entries.items[i];
		if (!e.marked) continue;
		se.marked = true;
		da_append(se.entries, e.name);
	}
	if (!se.marked)
	{
		entry e = dir->entries.items[dir->current + dir->scroll];
		da_append(se.entries, e.name);
	}
	return se;
}

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

static bool file_exists(const char* file)
{
	struct stat st;
	return (stat(file, &st) == 0);
}

static bool copy_file_dir(const char* src, const char* dst)
{
	if (!is_dir(dst)) return false;

	char* src_cpy = strdup(src);
	char* name = basename(src_cpy);
	char* dst_full = stralloc("%s/%s", dst, name);

	bool ret = copy_file(src, dst_full);
	free(dst_full);
	free(src_cpy);
	return ret;
}

bool copy_file(const char* src, const char* dst)
{
	if (file_exists(dst)) return false;
	if (is_dir(dst)) return copy_file_dir(src, dst);

	FILE* fp_src = fopen(src, "r");
	FILE* fp_dst = fopen(dst, "w");

	if (!fp_src) fatal("failed to open '%s' for reading", src);
	if (!fp_dst) fatal("failed to open '%s' for writing", dst);

	char buf[READ_SIZE] = {0};
	size_t sz = 0;
	while ((sz = fread(buf, 1, READ_SIZE, fp_src)))
	{
		if (fwrite(buf, 1, sz, fp_dst) != sz)
			fatal("failed to write %d bytes to '%s'", sz, dst);
	}
	fclose(fp_src);
	fclose(fp_dst);
	return true;
}

bool move_file(const char* src, const char* dst)
{
	if (!copy_file(src, dst)) return false;
	if (remove(src) != 0) return false;
	return true;
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

void delete_entries(WINDOW* wind, directory* cwd)
{
	selected_entries se = get_selected(cwd);

	char input;
	if (se.marked)
		input = confirm(wind, "delete marked files? (y/N)");
	else
		input = confirm(wind, "delete '%s'? (y/N)", se.entries.items[0]);

	if (toupper(input) != 'Y')
		return;

	for (int i = 0; i < se.entries.len; i++)
	{
		const char* name = se.entries.items[i];
		if (!strcmp(name, ".") ||
		    !strcmp(name, ".."))
		{
			info(wind, "tried to delete '%s', might be a bug", name);
		}
		if (!remove_recursive(name))
		{
			info(wind, "failed to delete '%s': %s",
			     name, strerror(errno));
			free(se.entries.items);
			return;
		}

	}
	free(se.entries.items);
	info(wind, "deleted successfully");
}
