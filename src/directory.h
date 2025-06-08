#ifndef DIRECTORY_H_
#define DIRECTORY_H_

#include <stdbool.h>
#include "da.h"

#define ECOLOR_FILE 1
#define ECOLOR_DIR 2
#define ECOLOR_LNK 3
#define ECOLOR_EXE 4

#define ECOLOR_MSG 5
#define ECOLOR_HEAD 6
#define ECOLOR_MARKED 7

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
	int color;
	bool marked;
} entry;

typedef struct
{
	char* path;
	DA(entry) entries;
	int longest_links;
	int longest_owner;
	int longest_group;
	int longest_date;
	int longest_name;
	int y, x;
	int current;
	int scroll;
	bool soft;
} directory;

void change_dir(directory* cwd, const char* path);

char* expand_home(const char* path);

bool is_dir(const char* path);

bool is_dir_empty(const char* path);

bool remove_recursive(const char* path);

#endif
