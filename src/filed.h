#ifndef FILED_H_
#define FILED_H_

#include "directory.h"
#include "window.h"

bool exec_file(WINDOW* wind, directory* cwd, const char* path);

void delete_entries(WINDOW* wind, directory* cwd, entry* selected);


#endif
