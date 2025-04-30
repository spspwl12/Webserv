#include <stdlib.h>
#include <string.h>
#include "base64.h"

#define __base64_encode_len(len) ((len + 3 - len % 3) * 4 / 3 + 1)

void 
__base64_encode(
	char* dst, 
	const char* src, 
	unsigned int length
) 
{
	static const char* __base64_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	const unsigned char* current = (const unsigned char*)src;
	unsigned int Index = 0;

	while (length > 2) 
	{ 
		/* keep going until we have less than 24 bits */
		dst[Index++] = __base64_table[current[0] >> 2];
		dst[Index++] = __base64_table[((current[0] & 0x03) << 4) + (current[1] >> 4)];
		dst[Index++] = __base64_table[((current[1] & 0x0f) << 2) + (current[2] >> 6)];
		dst[Index++] = __base64_table[current[2] & 0x3f];

		current += 3;
		length -= 3; /* we just handle 3 octets of data */
	}

	/* now deal with the tail end of things */
	if (length) 
	{
		dst[Index++] = __base64_table[current[0] >> 2];
		if (length > 1) 
		{
			dst[Index++] = __base64_table[((current[0] & 0x03) << 4) + (current[1] >> 4)];
			dst[Index++] = __base64_table[(current[1] & 0x0f) << 2];
			dst[Index++] = '=';
		}
		else 
		{
			dst[Index++] = __base64_table[(current[0] & 0x03) << 4];
			dst[Index++] = '=';
			dst[Index++] = '=';
		}
	}

	dst[Index] = 0;
}

/* as above, but backwards. :) */

void 
__base64_decode(
	char* dst, 
	const char* src, 
	unsigned int length
) 
{

	static char reverse_table[256] = {
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 00-0F */
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 10-1F */
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  /* 20-2F */
		52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,  /* 30-3F */
		-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  /* 40-4F */
		15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  /* 50-5F */
		-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  /* 60-6F */
		41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  /* 70-7F */
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 80-8F */
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 90-9F */
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* A0-AF */
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* B0-BF */
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* C0-CF */
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* D0-DF */
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* E0-EF */
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1   /* F0-FF */
	};

	const unsigned char* current = (const unsigned char*)src;

	int ch, i = 0, j = 0, k;

	/* run through the whole string, converting as we go */
	while (ch = *current++)
	{
		/* When Base64 gets POSTed, all pluses are interpreted as spaces.
		   This line changes them back.  It's not exactly the Base64 spec,
		   but it is completely compatible with it (the spec says that
		   spaces are invalid).  This will also save many people considerable
		   headache.  - Turadg Aleahmad <turadg@wise.berkeley.edu>
		*/
		if (ch == '=')
			break;

		if (ch == ' ') 
			ch = '+';

		ch = reverse_table[ch];

		if (ch < 0) 
			continue;

		switch (i % 4) 
		{
			case 0:
				dst[j] = ch << 2;
				break;
			case 1:
				dst[j++] |= ch >> 4;
				dst[j] = (ch & 0x0f) << 4;
				break;
			case 2:
				dst[j++] |= ch >> 2;
				dst[j] = (ch & 0x03) << 6;
				break;
			case 3:
				dst[j++] |= ch;
				break;
		}

		i++;
	}

	k = j;
	/* mop things up if we ended on a boundary */
	if (ch == '=') 
	{
		switch (i % 4) 
		{
			case 0:
			case 1:
				return;
			case 2:
				k++;
			case 3:
				dst[k++] = 0;
		}
	}

	dst[k] = 0;
}