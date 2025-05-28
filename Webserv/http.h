#ifndef HTTP_H
#define HTTP_H

#include "union.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>

BOOLEAN
HTTP_SendClientHeader(
	SOCKET		sck,
	pHdr		pHeader,
	DWORD		RequestCode,
	QWORD		ContentLength
);

BOOLEAN
HTTP_SendClientFile(
	SOCKET		sck,
	HANDLE		hFile,
	QWORD		ContentLength
);

BOOLEAN
HTTP_SendClientBuffer(
	SOCKET		sck,
	LPCSTR		Buffer,
	DWORD		ContentLength
);

BOOLEAN
HTTP_SendClientText(
	SOCKET		sck,
	LPCSTR		_Format,
	...
);

BOOLEAN
HTTP_SendClientText_Pure(
	SOCKET		sck,
	LPCSTR		Text,
	DWORD		ContentLength
);

BOOLEAN
HTTP_SendFile(
	SOCKET sck,
	pHdr pHeader,
	LPCSTR Path,
	LPCSTR RelativePath
);

BOOLEAN
HTTP_SendFileBinary(
	SOCKET sck,
	pHdr pHeader,
	LPCSTR Binary,
	DWORD ContentLength
);

BOOLEAN
HTTP_AddHeader(
	pHdr	pHeader,
	LPCSTR	HeaderName,
	LPCSTR	HeaderValue
);

VOID
HTTP_FreeHeader(
	pHdr pHeader
);

BOOLEAN
HTTP_ExtUploadProgress(
	PSOCKETEX		ClientSckArray,
	DWORD			MaxClinet,
	pHdr			pHeader,
	QWORD*			TotalRead,
	QWORD*			ContentLength
);

BOOLEAN
HTTP_SetSckResponeSession(
	PSOCKETEX		psckEx,
	pHdr			pHeader
);

BOOLEAN
HTTP_Upload(
	PSOCKETEX	psckEx,
	pHdr		pHeader,
	LPCSTR		UploadPath
);

VOID
HTTP_ProcessMain(
	SOCKET		sck,
	pHdr		pHeader,
	LPCSTR		Path,
	LPCSTR		RelativePath,
	BYTE		IsUploadChat
);

BOOLEAN
HTTP_ProcessIcon(
	SOCKET		sck,
	pHdr		pHeader,
	DWORD		urlLen
);

BOOLEAN
HTTP_ProcessUploadProgress(
	SOCKET		sck,
	PSOCKETEX	ClientSckArray,
	pHdr		pHeader
);

BOOLEAN
HTTP_ContextHtml(
	SOCKET		sck,
	pHdr		pHeader,
	LPCSTR		Path,
	LPCSTR		RelativePath,
	BYTE		IsUploadChat
);

VOID
HTTP_Context404(
	SOCKET		sck,
	pHdr		pHeader
);

VOID
HTTP_ContextUpload(
	SOCKET		sck,
	pHdr		pHeader
);

VOID
HTTP_ContextChat(
	SOCKET		sck,
	pHdr		pHeader,
	LPCSTR		ServerIP,
	WORD		Port
);

char*
HTTP_MIME(
	const char* extensions
);
#endif