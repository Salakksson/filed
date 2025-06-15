#ifndef FILED_H_
#define FILED_H_

#include "directory.h"
#include "window.h"

typedef struct
{
	DA(const char*) entries;
	bool marked;
} selected_entries;

selected_entries get_selected(directory* cwd);

bool exec_file(WINDOW* wind, directory* cwd, const char* path);

void delete_entries(WINDOW* wind, directory* cwd);

bool copy_file(const char* src, const char* dst);
bool move_file(const char* src, const char* dst);

#endif
