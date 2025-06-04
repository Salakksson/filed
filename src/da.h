#ifndef DA_H_
#define DA_H_

#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <errno.h>

#define fatal(...) do {fprintf(stderr, __VA_ARGS__); exit(0); } while (0)

#define DA(contents) struct { \
	contents* items; \
	int len; \
	int cap; \
}

#define da_append(list, item) \
do { \
	if ((list).len >= (list).cap) { \
		(list).cap *= 2; \
		(list).items = realloc((list).items, sizeof(*(list).items) * (list).cap); } \
	(list).items[(list).len] = item; \
	(list).len++; \
} while(0)

#define da_construct(list, sz) \
do { \
	(list).cap = sz ? sz : 10; \
	(list).len = 0; \
	(list).items = malloc(sizeof(*(list).items) * (list).cap); \
	if(!(list).items) fatal("failed to malloc: %s", strerror(errno)); \
} while(0)

#define da_swap(list, type, a, b) \
do { \
	type tmp = (list).items[a]; \
	(list).items[a] = (list).items[b]; \
	(list).items[b] = tmp; \
} while (0);

#ifdef _WIN32
#define strcasecmp stricmp
#endif

#define da_sort_entry(list) \
do { \
	for (int i = 0; i < (list).len - 1; i++) \
	{ \
		for (int j = 0; j < (list).len - 1 - i; j++) \
		{ \
			if (strcasecmp((list).items[j].name, (list).items[j+1].name) > 0 ) \
				da_swap(list, entry, j, j + 1); \
		} \
	} \
\
} while (0);

#endif
