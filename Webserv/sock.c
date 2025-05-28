#include "sock.h"
#include "parser.h"
#include "http.h"
#include "union.h"
#include "logic.h"
#include "unit.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ws2tcpip.h>
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment ( lib, "ws2_32.lib" )

extern _sgv Sgv;

SOCKET 
InitializeSocket(
)
{
	WSADATA		wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR)
		return INVALID_SOCKET;

	return socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

BOOLEAN 
BindAndListenSocket(
	SOCKET			sck, 
	WORD			wSrvPort,
	INT				send_timeout,
	INT				recv_timeout
)
{
	struct sockaddr_in socket = { 0 };

	if (INVALID_SOCKET == sck)
		return FALSE;

	if (SOCKET_ERROR == setsockopt(sck, SOL_SOCKET, SO_SNDTIMEO, (const char*)&send_timeout, sizeof(send_timeout)))
		return FALSE;

	if (SOCKET_ERROR == setsockopt(sck, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recv_timeout, sizeof(recv_timeout)))
		return FALSE;

	socket.sin_family = AF_INET;
	socket.sin_addr.s_addr = INADDR_ANY;
	socket.sin_port = htons(wSrvPort);

	if (SOCKET_ERROR == bind(sck, (const struct sockaddr*)&socket, sizeof(SOCKADDR_IN)))
		goto RELEASE;

	if (SOCKET_ERROR == listen(sck, SOMAXCONN))
		goto RELEASE;

	return TRUE;

RELEASE:

	CloseSocket(sck);
	WSACleanup();

	return FALSE;
}

VOID 
CloseSocket2(
	PSOCKETEX psckEx
)
{
	if (NULL == psckEx)
		return;

	shutdown(psckEx->sck, 0x02); // SD_BOTH = 0x02
	closesocket(psckEx->sck);

	memset(psckEx, 0, sizeof(SOCKETEX));
}

VOID
CloseSocket(
	SOCKET sck
)
{
	shutdown(sck, 0x02); // SD_BOTH = 0x02
	closesocket(sck);
}

DWORD WINAPI 
ListenSocket(
	LPVOID	psck
)
{
	SOCKET			mySocket = (SOCKET)psck;

	UINT			Index;
	SOCKADDR_IN		sckaddr;
	INT				addrlen = sizeof(SOCKADDR);
	BOOLEAN			FullSock = FALSE;

	while (Sgv.WebServerStatus)
	{
		for (Index = 0; (Index < MAX_USER) && (0 != Sgv.ClientSck[Index].sck); ++Index);

		if (Index >= MAX_USER - 1)
			FullSock = TRUE;

		if (SOCKET_ERROR == (Sgv.ClientSck[Index].sck = accept(mySocket, (LPSOCKADDR)&sckaddr, (LPINT)&addrlen)))
			break;

		if (FullSock)
		{
			CloseSocket(Sgv.ClientSck[Index].sck);
			FullSock = FALSE;

			continue;
		}

		Sgv.ClientSck[Index].ip = sckaddr.sin_addr;
		Sgv.ClientSck[Index].Type = SOCK_HTTP;
		Sgv.ClientSck[Index].RcvPong = 0;

		CLOSEHANDLE(CreateThread(NULL, 0, ProcessDataThread, (LPVOID)(Sgv.ClientSck + Index), 0, NULL));
	}

	WSACleanup();

	Sgv.WebServerStatus = WEBSERV_END;
	return FALSE;
}

BOOLEAN 
StartWebServer(
	SOCKET			ServerSock,
	WORD			wSrvPort,
	INT				send_timeout,
	INT				recv_timeout
)
{
	if( FALSE == BindAndListenSocket(ServerSock, wSrvPort, send_timeout, recv_timeout))
		return FALSE;

	Sgv.WebServerStatus = WEBSERV_START;
	CLOSEHANDLE(CreateThread(NULL, 0, ListenSocket, (LPVOID)ServerSock, 0, NULL));

	return TRUE;
}

BOOLEAN
StopWebServer(
	SOCKET			ServerSock
)
{
	Sgv.WebServerStatus = WEBSERV_STOP;
	CloseSocket(ServerSock);

	while (Sgv.WebServerStatus != WEBSERV_END)
		Sleep(100);
	
	Sgv.WebServerStatus = WEBSERV_STOP;

	return TRUE;
}

BOOLEAN 
ReadSocketHeader(
	SOCKET		sck, 
	MUST_FREE_MEMORY LPSTR* Buffer,
	char**		pEnd, 
	LPDWORD		lpContentRead
)
{
	LONG		bytesReceived = 0;
	DWORD		amountRead = 0;
	DWORD		allocSize = MAX_HDRBUF;

	if (NULL == (*Buffer = (LPSTR)malloc(sizeof(CHAR) * (allocSize + 1))))
		return fail_alloc_msg();

	do
	{
		do
		{
			if (0 >= (bytesReceived = recv(sck, *Buffer + amountRead, allocSize - amountRead, 0)))
			{
				FREE_DATA(*Buffer);
				return FALSE;
			}

			amountRead += bytesReceived;

			if (0 != (*pEnd = strstr(*Buffer, "\r\n\r\n")))
			{
				**pEnd = 0;
				*pEnd += 4;
				goto FOUND;
			}

		} while (allocSize > amountRead);

		allocSize *= 2;
		const void* newBuf = realloc(*Buffer, sizeof(CHAR) * (allocSize + 1));

		if (NULL == newBuf)
			return fail_alloc_msg();

		*Buffer = (LPSTR)newBuf;

	} while (1);

FOUND:
	(*Buffer)[amountRead] = 0;

	if (lpContentRead)	// 헤더를 읽는 도중에 Content 내용까지 읽을 경우 lpContentRead 값에 읽은 바이트 수를 입력한다.
		*lpContentRead = amountRead - (DWORD)(*pEnd - *Buffer);

	return TRUE;
}

BOOLEAN 
ReadSocketContent_Text(
	SOCKET sck, 
	MUST_FREE_MEMORY LPSTR* Buffer,
	DWORD ReadSize,
	DWORD Offset
)
{
	LONG		bytesReceived = 0;
	DWORD		amountRead = 0;

	if (NULL == (*Buffer = (LPSTR)malloc(sizeof(CHAR) * (ReadSize + Offset + 1))))
		return fail_alloc_msg();

	do
	{
		if (0 >= (bytesReceived = recv(sck, *Buffer + Offset + amountRead, ReadSize - amountRead, 0)))
			return FALSE;

		amountRead += bytesReceived;

	} while (amountRead < ReadSize);

	(*Buffer)[Offset + ReadSize] = 0;

	return TRUE;
}

DWORD
ReadSocketContent(
	SOCKET sck,
	LPSTR Buffer,
	DWORD ReadSize,
	QWORD* ReadPos,
	QWORD ContentLength
)
{
	LONG		bytesReceived = 0;
	DWORD		amountRead = 0;

	if (*ReadPos + ReadSize >= ContentLength)
		ReadSize = (DWORD)(ContentLength - *ReadPos);

	do
	{
		if (0 >= (bytesReceived = recv(sck, Buffer + amountRead, ReadSize - amountRead, 0)))
			return FALSE;

		amountRead += bytesReceived;

	} while (ReadSize > amountRead);

	*ReadPos += amountRead;

	return amountRead;
}

BOOLEAN
GetHostIP(
	HWND	hComboBox,
	LPCSTR	szDefault
)
{
	CHAR		hostn[MAX_PATH];
	CHAR		IP[50] = { 0 };
	IN_ADDR		addr = { 0 };
	ADDRINFO	hints;
	ADDRINFO	*result = NULL;
	WSADATA		wsaData;
	BOOLEAN		bResult = FALSE;
	BOOL		bDefault = FALSE;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR)
		return FALSE;

	ZeroMemory(&hints, sizeof(hints));

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	if (SOCKET_ERROR == gethostname(hostn, MAX_PATH))
		goto RELEASE;

	if (getaddrinfo(hostn, "80", &hints, &result))
		goto RELEASE;

	SendMessage(hComboBox, CB_ADDSTRING, (WPARAM)sizeof(IP), (LPARAM)"127.0.0.1");

	for (struct addrinfo *ptr = result; ptr; ptr = ptr->ai_next)
	{
		if (AF_INET == ptr->ai_family) //IPv4
		{
			inet_ntop(AF_INET, 
				(LPVOID)&((PSOCKADDR_IN)ptr->ai_addr)->sin_addr,
				IP,
				sizeof(IP)
			);

			SendMessage(hComboBox, CB_ADDSTRING, (WPARAM)sizeof(IP), (LPARAM)IP);

			if (!bDefault && szDefault && 0 == strcmp(szDefault, IP))
			{
				const int cnt = (int)SendMessage(hComboBox, CB_GETCOUNT, 0, 0);
				SendMessage(hComboBox, CB_SETCURSEL, (WPARAM)cnt - 1, 0);
				bDefault = TRUE;
			}
		}
	}

	bResult = TRUE;

RELEASE:

	WSACleanup();

	return bResult;
}