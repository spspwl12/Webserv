#include "union.h"
#include "unit.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

void 
SizeToUnit(
	char* dst, 
	size_t dstlen,
	unsigned long High, 
	unsigned long Low
)
{
	const char* unit[] = { "KB", "MB", "GB", "TB", "PB", "EB" };

	unsigned long long qword = (((unsigned long long)High) << 32) | Low;
	unsigned long long seg = 0x1000000000000000;

	for (int i = 5; i >= 0; --i, seg /= 1024)
	{
		if (qword / seg > 0)
		{
			sprintf_s(dst, dstlen, "%.2f %s", (double)qword / seg, unit[i]);
			return;
		}
	}

	sprintf_s(dst, dstlen, "%d   B", Low);
}

int
Replace(
	char** dst,
	char* src,
	const char* find,
	const char* replace
)
{
	int count = 0;
	const int findLen = (int)strlen(find);
	const int repLen = (int)strlen(replace);
	char* sch;

	if (0 == findLen || 0 == repLen)
		return 0;

	const int srclen = (int)strlen(src);

	if (findLen != repLen)
	{
		sch = src;

		while (sch = strstr(sch, find))
		{
			++count;
			sch += findLen;
		}

		if (0 == count)
			return 0;
	}

	const int len = srclen + count * (repLen - findLen);

	if (0 == len)
		return 0;

	if (0 == (*dst = (char*)malloc(sizeof(char) * (len + 1))))
		return -1;

	sch = *dst;

	while (*src)
	{
		if (0 == memcmp(src, find, sizeof(char) * findLen))
		{
			memcpy(sch, replace, sizeof(char) * repLen);
			sch += repLen;
			src += findLen;

			continue;
		}

		*sch++ = *src++;
	}

	(*dst)[len] = 0;

	return len;
}

void
MakeRandomString(
	char* sz,
	unsigned long len
)
{
	const char* randomchars = "abcdefghijklmnopqrstuvwxyz0123456789";
	unsigned int idx = 0;

	srand(
#ifdef _M_X64
	(unsigned int)GetTickCount64()
#else
	(unsigned int)GetTickCount()
#endif
	);

	for (; idx < len; ++idx)
		sz[idx] = randomchars[rand() % 36];

	sz[idx] = 0;
}

void
RemoveSpecialChar(
	char* sz,
	size_t len
)
{
	const char* special = "\\/:*?\"<>|";
	unsigned int c = 0;

	for (unsigned int i = 0; i < len; ++i)
		for (unsigned int j = 0; j < 9; ++j)
			if (sz[i] == special[j])
			{
				j = 0;
				++c;
				for (unsigned int k = i; k < len - 1; ++k)
					sz[k] = sz[k + 1];
			}

	sz[len - c] = 0;
}

void 
GetTimeSrv(
	char*	dst,
	size_t	size
)
{
	static const char* days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	static const char* months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
							"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

	time_t t = time(NULL);
	struct tm tm;
		
	gmtime_s(&tm, &t);

	sprintf_s(dst, size,
		"%s, %02d %s %d %02d:%02d:%02d GMT",
		days[tm.tm_wday], tm.tm_mday, months[tm.tm_mon],
		tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

int
UTF8toANSI(
	const char* utf8,
	MUST_FREE_MEMORY char** ansi,
	int allocate
)
{
	// convert multibyte UTF-8 to wide string UTF-16
	int length = MultiByteToWideChar(CP_UTF8, 0, (const char*)utf8, -1, NULL, 0) - 1;

	if (length > 0)
	{
		wchar_t* lpwsz = (wchar_t*)malloc(sizeof(wchar_t) * (length + 1));

		if (NULL == lpwsz)
			return fail_alloc_msg();

		MultiByteToWideChar(CP_UTF8, 0, (char*)utf8, -1, lpwsz, length);

		lpwsz[length] = 0;

		length = WideCharToMultiByte(CP_ACP, 0, lpwsz, -1, NULL, 0, NULL, NULL) - 1;

		char* lpszbuf = allocate ? (char*)malloc(sizeof(char) * (length + 1)) : *ansi;

		if (NULL == lpszbuf)
			return fail_alloc_msg();

		if (length)
			WideCharToMultiByte(CP_ACP, 0, lpwsz, -1, lpszbuf, length, 0, 0);

		lpszbuf[length] = 0;

		free(lpwsz);

		*ansi = lpszbuf;
	}

	return 1;
}

int
ANSItoUTF8(
	const char* ansi,
	MUST_FREE_MEMORY char** utf8
)
{
	int length = MultiByteToWideChar(CP_ACP, 0, ansi, -1, NULL, 0) - 1;

	if (length > 0)
	{
		wchar_t* lpwsz = (wchar_t*)malloc(sizeof(wchar_t) * (length + 1));

		if (NULL == lpwsz)
			return fail_alloc_msg();

		MultiByteToWideChar(CP_ACP, 0, ansi, -1, lpwsz, length);

		lpwsz[length] = 0;

		length = WideCharToMultiByte(CP_UTF8, 0, lpwsz, -1, NULL, 0, NULL, NULL) - 1;

		char* lpszbuf = (char*)malloc(sizeof(char) * (length + 1));

		if (NULL == lpszbuf)
			return fail_alloc_msg();

		if (length)
			WideCharToMultiByte(CP_UTF8, 0, lpwsz, -1, lpszbuf, length, 0, 0);

		lpszbuf[length] = 0;

		free(lpwsz);

		*utf8 = lpszbuf;
	}

	return length;
}

int
URLDecode(
	char* src
)
{
	unsigned int j = 0;
	unsigned int k = 0;

	if (NULL == src)
		return 0;

	size_t len = (unsigned int)strlen(src);

	while (*(src + k))
	{
		switch (*(src + k))
		{
		case '%':
		{
			if (k + 2 >= len)
				return 0;

			{
				const char* base = src + k + 1;
				char c;

				if (base[0] >= 'A' && base[0] <= 'F')
					c = (base[0] - 'A' + 10) * 16;
				else if (base[0] >= 'a' && base[0] <= 'f')
					c = (base[0] - 'a' + 10) * 16;
				else
					c = (base[0] - '0') * 16;

				if (base[1] >= 'A' && base[1] <= 'F')
					c += (base[1] - 'A' + 10);
				else if (base[1] >= 'a' && base[1] <= 'f')
					c += (base[1] - 'a' + 10);
				else
					c += (base[1] - '0');

				*(src + j) = c;
			}

			k += 2;

			break;
		}
		case '+':
		{
			*(src + j) = ' ';
			break;
		}
		default:
			*(src + j) = *(src + k);
		}

		++j;
		++k;
	}

	*(src + j) = 0;

	return UTF8toANSI(src, &src, 0);
}

MUST_FREE_MEMORY char*
URLEncode(
	const char* src
)
{
	char*			covStr = 0;
	char*			utf8 = 0;
	unsigned int	i = 0;
	unsigned int	j = 0;

	if (NULL == src)
		return NULL;

	if (0 == ANSItoUTF8(src, &utf8))
		return NULL;

	size_t len = strlen(utf8);

	if (NULL == (covStr = (char*)malloc(sizeof(char) * (len * 3 + 1))))
		return (char*)(__int64)fail_alloc_msg();

	while (utf8[i])
	{
		if (utf8[i] == '\\' ||
			utf8[i] == '/' ||
			utf8[i] == '.' ||
			utf8[i] == '@' ||
			utf8[i] == '-' ||
			utf8[i] == '_' ||
			utf8[i] == ':' ||
			(utf8[i] >= 'a' && utf8[i] <= 'z') ||
			(utf8[i] >= 'A' && utf8[i] <= 'Z') ||
			(utf8[i] >= '0' && utf8[i] <= '9'))
		{
			covStr[j++] = utf8[i];
		}
		else
		{
			sprintf_s(covStr + j, len * 3 + 1 - j, "%%%02x", (unsigned char)utf8[i]);
			j += 3;
		}

		++i;
	}

	covStr[j] = 0;

	if (utf8)
		free(utf8);

	return covStr;
}

int
Dswprintf(
	char** _Buffer,
	const char* _Format,
	...
)
{
	va_list args;

	*_Buffer = 0;

	va_start(args, _Format);
	const int length = _vscprintf(_Format, args);
	va_end(args);

	if (length <= 0)
		return 0;

	if (0 == (*_Buffer = (char*)malloc(sizeof(char) * (length + 1))))
		return -1;

	va_start(args, _Format);
	vsprintf_s(*_Buffer, length + 1, _Format, args);
	va_end(args);

	return length;
}

void*
memwchar(
	const void* haystack,
	wchar_t needle,
	int len
)
{
	const char* data = (char*)haystack;

	if (len <= 0)
		return 0;

	for (int i = 0, l = len; i < l; ++i)
	{
		if (0 == memcmp(data + i, &needle, 2))
			return (void*)(data + i);
	}

	return 0;
}

int
BinarySearch(
	const char* find_str,
	const size_t find_str_len,
	const void* array_ptr,
	const size_t array_size,
	const size_t array_len,
	const size_t cmp_offset
)
{
	for (int cmp, m, l = 0, r = (int)array_len - 1; l <= r;)
	{
		m = l + (r - l) / 2;

		if (0 == (cmp = _strnicmp(find_str, 
								*(char**)((char*)array_ptr + (m * array_size) + cmp_offset), 
								find_str_len)))
			return m;

		if (cmp > 0)
			l = m + 1;
		else
			r = m - 1;
	}

	return -1;
}

void
tolowerStr(
	char* sz
)
{
	while (*sz)
		*sz++ = tolower(*sz);
}

int
fail_alloc_msg(
)
{
	MessageBox(0, "Error: Unable to allocate memory. \n"
		"Ensure sufficient resources are available and restart the application.", "Err", MB_ICONERROR | MB_TOPMOST);

	exit(1);

	return 0;
}