#include "parser.h"
#include "unit.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdlib.h>

BOOLEAN
AnalyzeHeader(
	LPCSTR		Hdr,
	MUST_FREE_MEMORY pHdr pHeader,	// pHeader->RequestHeader, pHeader->RequestURLQuery
	LPSTR		pHeaderEnd,
	DWORD		ContentRead
)
{
	BOOLEAN			HdrFound = FALSE;
	static LPCSTR	szmethods[] = { "DELETE", "GET", "HEAD", "OPTIONS", "PATCH", "POST", "PUT", "TRACE" };
	LPSTR			parsIndex = (LPSTR)Hdr;
	LPSTR			pTmp;
	LPSTR			pContext = NULL;
	DWORD			TypeLen;
	DWORD			i, j;

	if (NULL == pHeader ||
		NULL == Hdr)
		return FALSE;

	pHeader->HeaderEnd = pHeaderEnd;
	pHeader->ContentRead = ContentRead;

	char* dummy = NULL;
	const char* tok = strtok_s((char*)Hdr, " ", &dummy);

	{
		const int Index = BinarySearch(
			Hdr,
			strlen(Hdr),
			szmethods,
			sizeof(szmethods[0]),
			ARRAYSIZE(szmethods), 0);

		if (Index >= 0)
		{
			pHeader->HdrMethods = (enum methods)Index;
			TypeLen = (DWORD)strlen(szmethods[Index]);
			HdrFound = TRUE;
		}
	}

	if (FALSE == HdrFound)
		return FALSE;

	parsIndex += TypeLen + 1;

	++parsIndex; // Skip(/)
	pHeader->RequestURL = parsIndex;  // + 1= Skip(/)

	pTmp = strstr(parsIndex, " HTTP/");

	if (NULL == pTmp)
		return FALSE;

	*pTmp = 0;

	pTmp += 6;
	pHeader->HTTPVersion = pTmp;

	pTmp += 3;
	SKIP_RETURNCHAR(pTmp);

	parsIndex = pTmp;

	if (pTmp = strstr(pHeader->RequestURL, "?")) // URL 에 변수가 있을 시 
	{
		*pTmp++ = 0;
		AnalyzeQuery(pTmp, &pHeader->RequestURLQuery, &pHeader->RequestURLQueryCount);
	}

	j = 10;
	
	if (0 == (pHeader->RequestHeader = (pReqHdr)malloc(sizeof(ReqHdr) * j)))
		return fail_alloc_msg();

	pTmp = strtok_s(parsIndex, "\r\n", &pContext);
	i = 0;

	while (pTmp)
	{
		LPSTR	pTok;

		if (i >= j)
		{
			j += 10;
			const void* newBuf = realloc(pHeader->RequestHeader, sizeof(ReqHdr) * j);

			if (NULL == newBuf)
				return fail_alloc_msg();

			pHeader->RequestHeader = (pReqHdr)newBuf;
		}

		pHeader->RequestHeader[i].Alloc = 0;
		pHeader->RequestHeader[i].Name = pTmp;

		pTok = strchr(pTmp, ':');

		if (NULL == pTok)
			break;

		*pTok++ = 0;

		SKIP_SPACECHAR(pTok);
		pHeader->RequestHeader[i].Value = pTok;
		pHeader->RequestHeader[i].NameLen = (WORD)strlen(pTmp);

		++i;
		pTmp = strtok_s(NULL, "\r\n", &pContext);
	}

	pHeader->RequestHeaderCount = i;

	return TRUE;
}

BOOLEAN
ParseHeader(
	LPSTR		Hdr,
	pReqHdr		pHeader,
	DWORD*		HeaderCount
)
{
	LPSTR		pContext = NULL;
	LPSTR		pTok;
	LPSTR		pTmp;
	DWORD		Cnt = 0;

	*HeaderCount = 0;
	pTmp = strtok_s(Hdr, "\r\n", &pContext);

	while (pTmp)
	{
		pHeader[Cnt].Alloc = 0;
		pHeader[Cnt].Name = pTmp;

		pTok = strchr(pTmp, ':');

		if (NULL == pTok)
			return FALSE;

		*pTok++ = 0;

		SKIP_SPACECHAR(pTok);
		pHeader[Cnt].Value = pTok;
		pHeader[Cnt].NameLen = (WORD)strlen(pTmp);
		++Cnt;

		pTmp = strtok_s(NULL, "\r\n", &pContext);
	}

	*HeaderCount = Cnt;

	return TRUE;
}

BOOLEAN 
ParseRange(
	pHdr		pHeader, 
	QWORD*		StartPos, 
	QWORD*		EndPos
)
{
	LPSTR		pEndPtr = NULL;

	if (NULL == pHeader)
		return FALSE;

	for (DWORD Index = 0; Index < pHeader->RequestHeaderCount; ++Index)
	{
		if (0 == strncmp(pHeader->RequestHeader[Index].Name, "Range", 5) &&
			0 == strncmp(pHeader->RequestHeader[Index].Value, "bytes=", 6))
		{
			LPSTR Range = pHeader->RequestHeader[Index].Value + 6;
			LPSTR pRangeTok = strchr(Range, '-');

			if (NULL == pRangeTok)
				pRangeTok = Range + strlen(Range);

			*pRangeTok = 0;
			*StartPos = strtoull(Range, &pEndPtr, 10);
			*EndPos = *(pRangeTok + 1) ? strtoull(pRangeTok + 1, &pEndPtr, 10) : 0;

			return TRUE;
		}
	}

	return FALSE;
}

LPSTR 
FindHeaderValue(
	pHdr		pHeader, 
	LPCSTR		szName
)
{
	if (NULL == pHeader)
		return NULL;

	for (DWORD Index = 0, Len = (DWORD)strlen(szName); Index < pHeader->RequestHeaderCount; ++Index)
	{
		if (Len == pHeader->RequestHeader[Index].NameLen &&
			0 == strncmp(pHeader->RequestHeader[Index].Name, szName, Len))
			return pHeader->RequestHeader[Index].Value;
	}

	return NULL;
}

LPSTR
FindVarNameValue(
	pReqVar		_pReqVar,
	DWORD		VarCount,
	LPCSTR		szName
)
{
	if (NULL == _pReqVar)
		return NULL;

	for (DWORD Index = 0, Len = (DWORD)strlen(szName); Index < VarCount; ++Index)
	{
		if (Len == _pReqVar[Index].NameLen &&
			0 == strncmp(_pReqVar[Index].Name, szName, Len))
			return _pReqVar[Index].Value;
	}

	return NULL;
}

LPSTR
FindVarNameValue2(
	pHdr		pHeader,
	LPCSTR		szName
)
{
	if (NULL == pHeader)
		return NULL;

	return FindVarNameValue(pHeader->RequestURLQuery, 
		pHeader->RequestURLQueryCount, szName);
}

BOOLEAN
AnalyzeQuery(
	LPSTR		Str,
	MUST_FREE_MEMORY pReqVar* _pReqVar,
	DWORD*		VarCount
)
{
	DWORD		j = 5;
	DWORD		i = 0;

	LPSTR		delimiter = NULL;
	LPSTR		pContext = NULL;
	LPSTR		pTok;

	if (NULL == Str)
		return FALSE;

	if (NULL == (*_pReqVar = (pReqVar)malloc(sizeof(ReqVar) * j)))
		return fail_alloc_msg();

	delimiter = strtok_s(Str, "&", &pContext);

	while (delimiter)
	{
		if (i >= j)
		{
			j += 5;
			const void* newBuf = realloc(*_pReqVar, sizeof(ReqVar) * j);

			if (NULL == newBuf)
				return fail_alloc_msg();

			*_pReqVar = (pReqVar)newBuf;
		}

		if ('=' == *delimiter)
			goto RELEASE;

		pTok = strchr(delimiter, '=');

		if (NULL == pTok)
			goto RELEASE;

		(*_pReqVar)[i].Name = delimiter;
		*pTok++ = 0;
		(*_pReqVar)[i].Value = pTok;
		(*_pReqVar)[i].NameLen = (WORD)strlen(delimiter);
		(*_pReqVar)[i].Alloc = 0;

		++i;
		delimiter = strtok_s(NULL, "&", &pContext);
	}

	*VarCount = i;

	return TRUE;

RELEASE:

	free(*_pReqVar);
	*_pReqVar = 0;

	return FALSE;
}

BOOLEAN
AnalyzeCookie(
	LPSTR		Str,
	MUST_FREE_MEMORY pReqVar* _pReqVar,
	DWORD*		VarCount
)
{
	DWORD		j = 5;
	DWORD		i = 0;

	LPSTR		delimiter = NULL;
	LPSTR		pContext = NULL;
	LPSTR		pTok;

	if (NULL == Str)
		return FALSE;

	if (NULL == (*_pReqVar = (pReqVar)malloc(sizeof(ReqVar) * j)))
		return fail_alloc_msg();

	delimiter = strtok_s(Str, ";", &pContext);

	while (delimiter)
	{
		if (i >= j)
		{
			j += 5;

			const void* newBuf = (pReqVar)realloc(*_pReqVar, sizeof(ReqVar) * j);

			if (NULL == newBuf)
				return fail_alloc_msg();

			*_pReqVar = (pReqVar)newBuf;
		}

		SKIP_SPACECHAR(delimiter);

		if ('=' == *delimiter)
			goto RELEASE;

		(*_pReqVar)[i].Name = delimiter;
		(*_pReqVar)[i].Value = 0;
		(*_pReqVar)[i].NameLen = 0;

		if (pTok = strchr(delimiter, '='))
		{
			*pTok++ = 0;
			(*_pReqVar)[i].Value = pTok;
			(*_pReqVar)[i].NameLen = (WORD)strlen(delimiter);
		}

		++i;
		delimiter = strtok_s(NULL, ";", &pContext);
	}

	*VarCount = i;

	return TRUE;

RELEASE:

	free(*_pReqVar);
	*_pReqVar = NULL;

	return FALSE;
}