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


void change_dir(directory* cwd, const char* path);

bool is_dir(const char* path);

#endif
