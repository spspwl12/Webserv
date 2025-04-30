#ifndef RES_H
#define RES_H

#include <stddef.h>

typedef struct store_binary_buff {
	char* name;
	void* buf;
	size_t size;
}sbb;

char*
GetResourcesFile(
	const char* filename,
	size_t* buflen
);
#endif
