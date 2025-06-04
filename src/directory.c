#include "directory.h"

#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#include "da.h"

#define KILOBYTE (off_t)1024
#define MEGABYTE (KILOBYTE*KILOBYTE)
#define GIGABYTE (MEGABYTE*KILOBYTE)
#define TERABYTE (GIGABYTE*KILOBYTE)

static int intlen(int n)
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
			free(e.link);
		}
		free(cwd->entries.items);
		free(cwd->path);
	}
	if (!strlen(path)) // if path is "" just free the memory
	{
		free((void*)path);
		return;
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

		e.color = ECOLOR_FILE;
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

		if(m & S_IXUSR) e.color = ECOLOR_EXE;
		if(m & S_IXGRP) e.color = ECOLOR_EXE;
		if(m & S_IXOTH) e.color = ECOLOR_EXE;
		if(S_ISDIR(m)) e.color = ECOLOR_DIR;
		if(S_ISLNK(m)) e.color = ECOLOR_LNK;
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

	closedir(dir);
	free((void*)path);
}

char* expand_home(const char* path)
{
	if (path[0] != '~') return strdup(path);
	const char* home = getenv("HOME");

	// no extra null byte is needed as the tilde gets removed
	char* fullpath = malloc(strlen(home) + strlen(path));
	strcpy(fullpath, home);
	strcat(fullpath, path + 1);

	return fullpath;
}

bool is_dir(const char* path)
{
	struct stat st;
	if (stat(path, &st) == -1) return false;
	return S_ISDIR(st.st_mode);
}
