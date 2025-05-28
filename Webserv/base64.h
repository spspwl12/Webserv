#ifndef BASE64_H
#define BASE64_H

void 
__base64_encode(
	char* dst,
	const char* src,
	unsigned int length
);

void
__base64_decode(
	char* dst,
	const char* src,
	unsigned int length
);

#endif
