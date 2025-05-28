#ifndef SOCKET_H
#define SOCKET_H

#include "union.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>

SOCKET 
InitializeSocket(
);

BOOLEAN
BindAndListenSocket(
	SOCKET			sck,
	WORD			wSrvPort,
	INT				send_timeout,
	INT				recv_timeout
);

VOID
CloseSocket2(
	PSOCKETEX		psckEx
);

VOID
CloseSocket(
	SOCKET			sck
);

BOOLEAN
StartWebServer(
	SOCKET			ServerSock,
	WORD			wSrvPort,
	INT				send_timeout,
	INT				recv_timeout
);

BOOLEAN
StopWebServer(
	SOCKET			ServerSock
);

BOOLEAN
ReadSocketHeader(
	SOCKET			sck,
	MUST_FREE_MEMORY LPSTR* Buffer,
	char**			pEnd,
	LPDWORD			lpContentRead
);

BOOLEAN
ReadSocketContent_Text(
	SOCKET			sck,
	MUST_FREE_MEMORY LPSTR* Buffer,
	DWORD			ReadSize,
	DWORD			Offset
);

DWORD
ReadSocketContent(
	SOCKET			sck,
	LPSTR			Buffer,
	DWORD			ReadSize,
	QWORD*			ReadPos,
	QWORD			ContentLength
);

BOOLEAN
GetHostIP(
	HWND	hComboBox,
	LPCSTR	szDefault
);

#endif