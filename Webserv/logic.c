#include "logic.h"
#include "union.h"
#include "sock.h"
#include "wsock.h"
#include "parser.h"
#include "http.h"
#include "unit.h"

#include <stdlib.h>

extern _sgv Sgv;

DWORD WINAPI
ProcessDataThread(
	LPVOID lparg
)
{
	PSOCKETEX	psckEx = (PSOCKETEX)lparg;
	SOCKET		sck = psckEx->sck;

	LPSTR 		Buffer = NULL;
	LPSTR		pEndHdr;
	LPSTR		_Value = NULL;

	DWORD		ContentRead = 0;
	DWORD		urlLen = 0;
	Hdr			MainHeader;

	if (NULL == lparg)
		goto DOWN;

	memset(&MainHeader, 0, sizeof(Hdr));

	if (FALSE == ReadSocketHeader(sck, &Buffer, &pEndHdr, &ContentRead))
		goto DOWN;

	if (FALSE == AnalyzeHeader(Buffer, &MainHeader, pEndHdr, ContentRead))
		goto DOWN;

	psckEx->pHeader = &MainHeader;

	urlLen = (DWORD)strlen(MainHeader.RequestURL);

	switch (MainHeader.HdrMethods)
	{
		case _GET:
		{
			if (0 == MainHeader.RequestURL ||
				0 == *MainHeader.RequestURL ||
				0 == strncmp(MainHeader.RequestURL, "file", urlLen))
			{
				if (_Value = FindVarNameValue2(&MainHeader, "v"))
					URLDecode(_Value);

				HTTP_ProcessMain(sck, &MainHeader, Sgv.DOWNLOAD_PATH, _Value, Sgv.UpChat);
			}
			else if (Sgv.UpChat & 0x01 &&
					0 == strncmp(MainHeader.RequestURL, "upload", urlLen))
			{
				HTTP_ContextUpload(sck, &MainHeader);
			}
			else if (Sgv.UpChat & 0x01 &&
					0 == strncmp(MainHeader.RequestURL, "upstatus", urlLen))
			{
				HTTP_ProcessUploadProgress(sck, Sgv.ClientSck, &MainHeader);
			}
			else if (Sgv.UpChat & 0x02 &&
					0 == strncmp(MainHeader.RequestURL, "chat", urlLen))
			{
				HTTP_ContextChat(sck, &MainHeader, Sgv.SERVER_IP, Sgv.SERVER_PORT);
			}
			else if (Sgv.UpChat & 0x02 &&
				0 == strncmp(MainHeader.RequestURL, "sock", urlLen) &&
				0 == strncmp(FindHeaderValue(&MainHeader, "Upgrade"), "websocket", 9)) // WebSocket
			{
				if (WebSocket_HandShake(psckEx, &MainHeader))
				{
					WebSocket_ReceiveData(psckEx);
					WebSocket_ConnectionClose(psckEx);
				}
			}
			else if (!HTTP_ProcessIcon(sck, &MainHeader, urlLen))
			{
				HTTP_Context404(sck, &MainHeader);
			}

			break;
		}
		case _POST:
		{
			pReqVar		pQuery = NULL;
			LPSTR		Session = NULL;

			if (Sgv.UpChat & 0x01 &&
				0 == strncmp(MainHeader.RequestURL, "upload", urlLen))
			{
				HTTP_SetSckResponeSession(psckEx, &MainHeader);

				if (HTTP_Upload(psckEx, &MainHeader, Sgv.UPLOAD_PATH))
					HTTP_ContextUpload(sck, &MainHeader);

				psckEx->Session = NULL;
			}

			break;
		}
	}

DOWN:

	HTTP_FreeHeader(&MainHeader);
	FREE_DATA(Buffer);
	CloseSocket2(psckEx);

	return FALSE;
}