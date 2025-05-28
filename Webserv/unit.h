#ifndef UNIT_H
#define UNIT_H

#include <stddef.h>

void
SizeToUnit(
	char* dst,
	size_t dstlen,
	unsigned long High,
	unsigned long Low
);

int
Replace(
	char** dst,
	char* src,
	const char* find,
	const char* replace
);

void
MakeRandomString(
	char* sz,
	unsigned long len
);

void
RemoveSpecialChar(
	char* sz,
	size_t len
);

void
GetTimeSrv(
	char* dst,
	size_t	size
);

int
UTF8toANSI(
	const char* utf8,
	MUST_FREE_MEMORY char** ansi,
	int allocate
);

int
ANSItoUTF8(
	const char* ansi,
	MUST_FREE_MEMORY char** utf8
);

int
URLDecode(
	char* sz
);

MUST_FREE_MEMORY char*
URLEncode(
	const char* src
);

int
Dswprintf(
	char** _Buffer,
	const char* _Format,
	...
);

void*
memwchar(
	const void* haystack,
	wchar_t needle,
	int len
);

int
BinarySearch(
	const char* find_str,
	const size_t find_str_len,
	const void* array_ptr,
	const size_t array_size,
	const size_t array_len,
	const size_t cmp_offset
);

void
tolowerStr(
	char* sz
);

int
fail_alloc_msg(
);

#endif