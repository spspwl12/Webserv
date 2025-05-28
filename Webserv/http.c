#include "http.h"
#include "res.h"
#include "unit.h"
#include "parser.h"
#include "sock.h"

#include <stdio.h>
#include <stdlib.h>

int
HTTP_SortFunc(
	void const* a, void const* b
);

BOOLEAN 
HTTP_SendClientHeader(
	SOCKET	sck, 
	pHdr	pHeader,
	DWORD	RequestCode,
	QWORD	ContentLength
)
{
	CHAR		Hdr[MAX_REQ_HDR_LEN];
	LPSTR		pTmp;
	LPCSTR		MIME = 0;
	LONG		bytesTransferred;

	DWORD		HeaderLength = 0;
	DWORD		TotalTransferred;
	QWORD		StartRange = 0;
	QWORD		EndRange = 0;

	BOOLEAN		UsageRange;

	if (NULL == pHeader)
		return FALSE;

	pTmp = pHeader->RequestURL;
	GETFILEEXT(pTmp);

	if (pTmp)
	{
		CHAR Buf[30];
		memcpy(Buf, pTmp, 30);
		Buf[29] = 0;

		tolowerStr(Buf);
		MIME = HTTP_MIME(Buf);
	}

	if (TRUE == (UsageRange = ParseRange(pHeader, &StartRange, &EndRange)))
		RequestCode = 206;

	switch (RequestCode)
	{
		case 200:
			HeaderLength = sprintf_s(Hdr, sizeof(Hdr), "HTTP/1.1 200 OK\n");
			break;
		case 206:
			HeaderLength = sprintf_s(Hdr, sizeof(Hdr), "HTTP/1.1 206 Partial Content\n");
			break;
		case 400:
			HeaderLength = sprintf_s(Hdr, sizeof(Hdr), "HTTP/1.1 400 Bad Request\n");
			break;
		case 404:
			HeaderLength = sprintf_s(Hdr, sizeof(Hdr), "HTTP/1.1 404 Not Found\n");
			break;
		default:
			return FALSE;
	}

	// CORS 대응
	HeaderLength += sprintf_s(Hdr + HeaderLength, sizeof(Hdr) - HeaderLength,
		"Access-Control-Allow-Origin: *\n");
	HeaderLength += sprintf_s(Hdr + HeaderLength, sizeof(Hdr) - HeaderLength,
		"Server: %s\n", SERVER_NAME);
	HeaderLength += sprintf_s(Hdr + HeaderLength, sizeof(Hdr) - HeaderLength,
		"Connection: keep-alive\n");
	HeaderLength += sprintf_s(Hdr + HeaderLength, sizeof(Hdr) - HeaderLength,
		"Keep-Alive: timeout=5\n");
	HeaderLength += sprintf_s(Hdr + HeaderLength, sizeof(Hdr) - HeaderLength,
		"Content-Type: %s\n", MIME ? MIME : "application/octet-stream");

	{
		CHAR Buf[100];

		GetTimeSrv(Buf, sizeof(Buf));

		HeaderLength += sprintf_s(Hdr + HeaderLength, sizeof(Hdr) - HeaderLength,
			"Date: %s\n", Buf);
	}

	if (pHeader->SendHeaderCount > 0 && pHeader->SendHeader)
	{
		for (UINT Index = 0; Index < pHeader->SendHeaderCount; ++Index)
		{
			HeaderLength += sprintf_s(Hdr + HeaderLength, sizeof(Hdr) - HeaderLength,
				"%s: %s\n", 
				pHeader->SendHeader[Index].Name, 
				pHeader->SendHeader[Index].Value);
		}
	}

	if (0 == EndRange)
		EndRange = ContentLength;

	if (UsageRange || 
		FALSE == pHeader->Download)
	{
		HeaderLength += sprintf_s(Hdr + HeaderLength, sizeof(Hdr) - HeaderLength,
			"Accept-Ranges: bytes\n");
	}

	if (UsageRange)
	{
		pHeader->Download = FALSE;

		HeaderLength += sprintf_s(Hdr + HeaderLength, sizeof(Hdr) - HeaderLength,
			"Content-Length: %llu\n", EndRange - StartRange);
		HeaderLength += sprintf_s(Hdr + HeaderLength, sizeof(Hdr) - HeaderLength,
			"Content-Range: bytes %llu-%llu/%llu\n", StartRange, EndRange - 1, ContentLength);
	}
	else if (ContentLength != 0)
	{
		HeaderLength += sprintf_s(Hdr + HeaderLength, sizeof(Hdr) - HeaderLength,
			"Content-Length: %llu\n", ContentLength);
	}

	if (TRUE == pHeader->Download)
	{
		GETFILENAME(pHeader->RequestURL);
		HeaderLength += sprintf_s(Hdr + HeaderLength, sizeof(Hdr) - HeaderLength,
			"Content-Disposition: attachment; filename=\"%s\";\n", pHeader->RequestURL);
	}

	// 버퍼 오버런 대응
	if (HeaderLength >= ARRAYSIZE(Hdr) - 1)
		HeaderLength = ARRAYSIZE(Hdr) - 2;

	Hdr[HeaderLength++] = '\n';
	Hdr[HeaderLength] = 0;

	bytesTransferred = 0;
	TotalTransferred = 0;

	while (TotalTransferred < HeaderLength)
	{
		if (0 >= (bytesTransferred = send(sck, Hdr + TotalTransferred, HeaderLength - TotalTransferred, 0)))
			return FALSE;

		TotalTransferred += bytesTransferred;
	}

	return TRUE;
}

BOOLEAN
HTTP_SendClientFile(
	SOCKET		sck,
	HANDLE		hFile,
	QWORD		ContentLength
)
{
	DWORD		AllocSize;
	QWORD		totalToSend;
	DWORD		amountRead = 0;
	DWORD		TotalTransferred = 0;
	LONG		bytesTransferred = 0;
	LPSTR		Buffer;
	BOOLEAN		bResult = FALSE;

	if (0 == ContentLength)
		return FALSE;

	AllocSize = (DWORD)(ContentLength > MAX_FILEBUF ? MAX_FILEBUF : ContentLength);

	if (NULL == (Buffer = (LPSTR)malloc(sizeof(CHAR) * AllocSize)))
		return fail_alloc_msg();

	totalToSend = 0;

	do
	{
		if (0 == ReadFile(hFile, Buffer, AllocSize, &amountRead, NULL) || 0 == amountRead)
			goto RELEASE;

		bytesTransferred = 0;
		TotalTransferred = 0;

		while (TotalTransferred < amountRead)
		{
			if (0 >= (bytesTransferred = send(sck, Buffer + TotalTransferred, amountRead - TotalTransferred, 0)))
				goto RELEASE;

			TotalTransferred += bytesTransferred;
		}

		totalToSend += TotalTransferred;

	} while (ContentLength > totalToSend);

	bResult = TRUE;

RELEASE:

	free(Buffer);

	return bResult;
}

BOOLEAN
HTTP_SendClientBuffer(
	SOCKET		sck,
	LPCSTR		Buffer,
	DWORD		ContentLength
)
{

	DWORD		TotalTransferred = 0;
	LONG		bytesTransferred = 0;

	bytesTransferred = 0;
	TotalTransferred = 0;

	do
	{
		if (SOCKET_ERROR == (bytesTransferred = send(sck, Buffer + TotalTransferred, ContentLength - TotalTransferred, 0)))
			goto RELEASE;

		TotalTransferred += bytesTransferred;

	} while (ContentLength > TotalTransferred);


	return TRUE;

RELEASE:

	return FALSE;
}

BOOLEAN
HTTP_SendClientText(
	SOCKET		sck,
	LPCSTR		_Format,
	...
)
{

	DWORD		ContentLength = 0;
	DWORD		TotalTransferred = 0;
	LONG		bytesTransferred = 0;
	LPSTR		Buffer = NULL;
	va_list		arg_ptr = NULL;
	BOOLEAN		bResult = FALSE;

	if (NULL == _Format)
		return FALSE;

	va_start(arg_ptr, _Format);

	if (0 == (ContentLength = _vscprintf(_Format, arg_ptr)))
		goto RELEASE;

	++ContentLength;

	if (NULL == (Buffer = (LPSTR)malloc(sizeof(CHAR) * ContentLength)))
		return fail_alloc_msg();

	vsprintf_s(Buffer, ContentLength, _Format, arg_ptr);

	bytesTransferred = 0;
	TotalTransferred = 0;

	do
	{
		if (SOCKET_ERROR == (bytesTransferred = send(sck, Buffer + TotalTransferred, ContentLength - TotalTransferred, 0)))
			goto RELEASE;

		TotalTransferred += bytesTransferred;

	} while (ContentLength > TotalTransferred);


	bResult = TRUE;

RELEASE:

	FREE_DATA_NON(Buffer);

	if (arg_ptr)
		va_end(arg_ptr);

	return bResult;
}

BOOLEAN
HTTP_SendClientText_Pure(
	SOCKET		sck,
	LPCSTR		Text,
	DWORD		ContentLength
)
{

	DWORD		TotalTransferred = 0;
	LONG		bytesTransferred = 0;
	BOOLEAN		bResult = FALSE;

	bytesTransferred = 0;
	TotalTransferred = 0;

	do
	{
		if (SOCKET_ERROR == (bytesTransferred = send(sck, Text + TotalTransferred, ContentLength - TotalTransferred, 0)))
			goto RELEASE;

		TotalTransferred += bytesTransferred;

	} while (ContentLength > TotalTransferred);


	bResult = TRUE;

RELEASE:

	return bResult;
}

BOOLEAN 
HTTP_SendFile(
	SOCKET sck,
	pHdr pHeader,
	LPCSTR Path,
	LPCSTR RelativePath
)
{
	QWORD			ContentLength;
	QWORD			RangeStart = 0;
	QWORD			RangeEnd = 0;
	HANDLE			hFile;
	LPSTR			PATH;
	DWORD			dwSizeHigh;
	LARGE_INTEGER	li;

	if (NULL == pHeader)
		return FALSE;

	if (-1 == Dswprintf(&PATH, "%s%s", Path, RelativePath))
		return fail_alloc_msg();

	if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(PATH))
	{
		FREE_DATA_NON(PATH);
		return FALSE;
	}

	hFile = CreateFileA(
		PATH,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	FREE_DATA_NON(PATH);

	if (INVALID_HANDLE_VALUE == hFile)
		return FALSE;
	
	ContentLength = GetFileSize(hFile, &dwSizeHigh);

	if (dwSizeHigh > 0)
		ContentLength |= ((QWORD)dwSizeHigh) << 32;

	pHeader->Download = TRUE;

	LPCSTR IsDown = FindVarNameValue2(pHeader, "vw");

	if (IsDown && *IsDown == '1')
		pHeader->Download = FALSE;

	pHeader->RequestURL = (LPSTR)RelativePath;
	HTTP_SendClientHeader(sck, pHeader, 200, ContentLength);

	if (_HEAD != pHeader->HdrMethods)
	{
		if (ParseRange(pHeader, &RangeStart, &RangeEnd))
		{
			li.QuadPart = RangeStart;
			SetFilePointerEx(hFile, li, NULL, FILE_BEGIN);
		}

		HTTP_SendClientFile(sck, hFile, ContentLength);
	}

	CloseHandle(hFile);

	return TRUE;
}

BOOLEAN
HTTP_SendFileBinary(
	SOCKET sck,
	pHdr pHeader,
	LPCSTR Binary,
	DWORD ContentLength
)
{
	if (NULL == pHeader)
		return FALSE;

	pHeader->Download = PASS;

	HTTP_SendClientHeader(sck, pHeader, 200, ContentLength);

	if (_HEAD != pHeader->HdrMethods)
		HTTP_SendClientBuffer(sck, Binary, ContentLength);

	return TRUE;
}

BOOLEAN
HTTP_AddHeader(
	pHdr	pHeader,
	LPCSTR	HeaderName,
	LPCSTR	HeaderValue
)
{
	DWORD		SndHdrIndex;
	DWORD		HeaderNameLen;
	DWORD		HeaderValueLen;

	LPSTR		NewHeaderName = NULL;
	LPSTR		NewHeaderValue = NULL;
	pReqHdr		NewHeader = NULL;

	if (NULL == pHeader ||
		NULL == HeaderName ||
		NULL == HeaderValue)
		return FALSE;
	
	HeaderNameLen = (DWORD)strlen(HeaderName);
	HeaderValueLen = (DWORD)strlen(HeaderValue);

	if (0 == HeaderNameLen ||
		0 == HeaderValueLen)
		return FALSE;

	SndHdrIndex = pHeader->SendHeaderCount;

	if (0 == SndHdrIndex)
		NewHeader = (pReqHdr)malloc(sizeof(ReqHdr));
	else
		NewHeader = (pReqHdr)realloc(pHeader->SendHeader, sizeof(ReqHdr) * (SndHdrIndex + 1));

	if (NULL == NewHeader)
		return fail_alloc_msg();

	NewHeaderName = (LPSTR)malloc(sizeof(CHAR) * (HeaderNameLen + 1));

	if (NULL == NewHeaderName)
		return fail_alloc_msg();

	NewHeaderValue = (LPSTR)malloc(sizeof(CHAR) * (HeaderValueLen + 1));

	if (NULL == NewHeaderValue)
		return fail_alloc_msg();

	memcpy(NewHeaderName, HeaderName, HeaderNameLen);
	memcpy(NewHeaderValue, HeaderValue, HeaderValueLen);

	NewHeaderName[HeaderNameLen] = NewHeaderValue[HeaderValueLen] = 0;

	NewHeader[SndHdrIndex].Alloc = 3;	// Name(1-HI), Value(1-LO) 
	NewHeader[SndHdrIndex].Name = NewHeaderName;
	NewHeader[SndHdrIndex].Value = NewHeaderValue;
	NewHeader[SndHdrIndex].NameLen = (WORD)HeaderNameLen;
	pHeader->SendHeader = NewHeader;

	++pHeader->SendHeaderCount;

	return TRUE;
}

VOID 
HTTP_FreeHeader(
	pHdr pHeader
)
{
	if (pHeader)
	{
		if (pHeader->SendHeaderCount > 0 && pHeader->SendHeader)
		{
			for (DWORD Index = 0; Index < pHeader->SendHeaderCount; ++Index)
			{
				if (pHeader->SendHeader[Index].Alloc & 0x02)
					FREE_DATA(pHeader->SendHeader[Index].Name);

				if (pHeader->SendHeader[Index].Alloc & 0x01)
					FREE_DATA(pHeader->SendHeader[Index].Value);
			}

			FREE_DATA(pHeader->SendHeader);
			pHeader->SendHeaderCount = 0;
		}

		FREE_DATA(pHeader->RequestHeader);
		FREE_DATA(pHeader->RequestURLQuery);
	}
}

BOOLEAN
HTTP_ExtUploadProgress(
	PSOCKETEX		ClientSckArray,
	DWORD			MaxClinet,
	pHdr			pHeader,
	QWORD*			TotalRead,
	QWORD*			ContentLength
)
{
	LPSTR		upSession = NULL;

	if (NULL == ClientSckArray ||
		NULL == pHeader)
		return FALSE;

	if (pHeader->RequestURLQuery && pHeader->RequestURLQueryCount > 0)
	{
		if (upSession = FindVarNameValue2(pHeader, "s"))
		{
			for (UINT Index = 0; Index < MaxClinet; ++Index)
			{
				if (ClientSckArray[Index].sck &&
					ClientSckArray[Index].Session &&
					0 == strncmp(ClientSckArray[Index].Session, upSession, strlen(ClientSckArray[Index].Session)))
				{
					if (ClientSckArray[Index].pHeader)
					{
						if (ClientSckArray[Index].pHeader->TotalRead)
							*TotalRead = *ClientSckArray[Index].pHeader->TotalRead;

						if (ClientSckArray[Index].pHeader->ContentLength)
							*ContentLength = *ClientSckArray[Index].pHeader->ContentLength;

						return TRUE;
					}
				}
			}
		}
	}

	return FALSE;
}

BOOLEAN
HTTP_SetSckResponeSession(
	PSOCKETEX		psckEx,
	pHdr			pHeader
)
{
	LPSTR		upSession = NULL;

	if (NULL == psckEx ||
		NULL == pHeader)
		return FALSE;

	if (pHeader->RequestURLQuery && pHeader->RequestURLQueryCount > 0)
	{
		if (upSession = FindVarNameValue2(pHeader, "s"))
			psckEx->Session = upSession;
	}

	return TRUE;
}

LPSTR
HTTP_ExtUploadFileName(
	LPSTR pHeaderStart
)
{
	LPSTR		pTmp;
	pReqVar		ContentType = NULL;
	DWORD		ContentTypeCount;
	ReqHdr		RequestHeader[5];
	DWORD		RequestHeaderCount = 0;

	if (NULL == pHeaderStart)
		return NULL;

	if (FALSE == ParseHeader(pHeaderStart, RequestHeader, &RequestHeaderCount))
		return NULL;

	if (NULL == (pTmp = FindVarNameValue(RequestHeader, RequestHeaderCount, "Content-Disposition")))
		return NULL;

	if (FALSE == AnalyzeCookie(pTmp, &ContentType, &ContentTypeCount))
		return NULL;

	pTmp = FindVarNameValue(ContentType, ContentTypeCount, "filename");

	FREE_DATA(ContentType);

	return pTmp;
}

BOOLEAN
HTTP_Upload(
	PSOCKETEX	psckEx,
	pHdr		pHeader,
	LPCSTR		UploadPath
)
{
	SOCKET		sck;
	QWORD		ContentLength;		// Content의 총 길이
	QWORD		TotalRead;			// Content를 얼마나 읽었는지 나타내는 변수 ( 총 읽은 길이 )
	DWORD		ContentTypeCount;	// 요청 받은 헤더 중 Content-Type 헤더의 타입 수
	DWORD		ContentRead;		// AnalyzeHeader 함수에서 실수로 Content 내용까지 읽었을 경우 얼마나 읽었는지 나타내는 변수
	DWORD		BoundaryLen;		// Boundary의 길이를 나타내는 변수
	DWORD		BytesWritten;		// buffer 에 담긴 파일내용을 얼마나 쓸 것인지 또는 얼마나 쓰였는지 나타내는 변수
	DWORD		dwBytesToRead;		// 패킷을 얼마나 읽을 것인지 나타내는 변수
	DWORD		amountReads;
	LPSTR		Buffer;				// 버퍼 ( 임시공간 )
	LPSTR		Boundary;			// 경계
	LPSTR		pBoundary;			// 버퍼에서 경계 문자열을 찾을 경우 그 위치를 나타내는 변수
	LPSTR		pBoundaryEnd;		// 버퍼에서 경계를 포함한 헤더의 끝을 알려주는 변수
	LPSTR		pPos;				// 버퍼에서 헤더의 첫 시작의 위치를 알려주는 변수
	LPSTR		szFileName;			// 버퍼에서 파싱한 파일 이름을 나타내는 변수
	LPSTR		szFullPath = 0;
	pReqVar		ContentType;		// 요청 받은 헤더의 이름과 그 값들을 알려주는 구조체
	BOOL		ContentEnd;			// Content가 끝났으면 TRUE의 값을 가지고 아니라면 FALSE 의 값을 가지는 변수
	BOOL		BoundaryFound;		// Content 에서 Boundary(경계) 를 찾을 경우 TRUE 아니라면 FALSE 값을 가지는 변수
	BOOL		FileDownload;		// 파일을 다운로드 할 준비가 완료되면 TRUE 아니면 FALSE
	BOOL		OnceRead;
	BOOL		bResult = FALSE;	// 이 프로시져의 리턴 값을 결정짓는 변수 ( 성공적으로 업로드가 끝나면 TRUE 아니면 FALSE )
	HANDLE		hFile = INVALID_HANDLE_VALUE;

	if (NULL == psckEx ||
		NULL == psckEx->pHeader ||
		NULL == pHeader)
		return FALSE;

	sck = psckEx->sck;

	if (0 >= pHeader->RequestHeaderCount ||
		NULL == pHeader->RequestHeader)
		return FALSE;

	if (NULL == (Buffer = FindHeaderValue(pHeader, "Content-Length")))
		return FALSE;

	ContentLength = strtoull(Buffer, &Boundary, 10);	// Boundary : 아무 의미없음

	if (NULL == (Buffer = FindHeaderValue(pHeader, "Content-Type")))
		return FALSE;

	if (FALSE == AnalyzeCookie(Buffer, &ContentType, &ContentTypeCount))
		return FALSE;

	Boundary = FindVarNameValue(ContentType, ContentTypeCount, "boundary");

	FREE_DATA(ContentType);

	if (NULL == Boundary)
		return FALSE;

	if (NULL == (Buffer = (LPSTR)malloc(sizeof(CHAR) * (MAX_FILEBUF + 1))))
		return fail_alloc_msg();

	// Boundary 문자열 앞에 '--' 를 더 추가한다.
	Boundary -= 2;
	Boundary[0] = Boundary[1] = '-';

	BoundaryLen = (DWORD)strlen(Boundary);

	// AnalyzeHeader 함수가 Content 를 읽었을 경우 Buffer에 Content 값을 복사한다.
	ContentRead = pHeader->ContentRead;

	if (ContentRead > 0)
	{
		if (ContentRead > MAX_FILEBUF)
		{
			// 위에서 할당한 버퍼의 최대용량을 넘어선 경우
			const void* newBuf = realloc(Buffer, sizeof(CHAR) * (ContentRead + 1));

			if (NULL == newBuf)
				return fail_alloc_msg();

			Buffer = (LPSTR)newBuf;
		}

		memcpy(Buffer, pHeader->HeaderEnd, ContentRead);
	}

	// 이미 앞서 읽은 패킷이 있으므로 TotalRead에 읽은 수만큼 플러스 시킨다.
	TotalRead = ContentRead;
	amountReads = ContentRead;

	FileDownload = FALSE;
	ContentEnd = FALSE;

	dwBytesToRead = MAX_FILEBUF - BoundaryLen - 3; // 3 은 --

	psckEx->pHeader->TotalRead = &TotalRead;
	psckEx->pHeader->ContentLength = &ContentLength;

	while (TotalRead < ContentLength)
	{
		if (FALSE == (amountReads += ReadSocketContent(sck,
			Buffer + ContentRead,
			dwBytesToRead - ContentRead, &TotalRead, ContentLength)))
			goto RELEASE;

		ContentRead = 0;
		pBoundary = Buffer;

		while (pBoundary)
		{
			BoundaryFound = FALSE;
			OnceRead = FALSE;

			while (pBoundary = (LPSTR)memwchar(pBoundary, 0x2D2D, (int)(amountReads - (pBoundary - Buffer)))) // 0x2D2D == '-' '-'
			{
				if (FALSE == OnceRead &&
					amountReads <= (DWORD)((pBoundary - Buffer) + BoundaryLen))
				{
					DWORD reads = 0;

					if (FALSE == (reads = ReadSocketContent(sck,
						Buffer + amountReads,
						BoundaryLen, &TotalRead, ContentLength)))
						goto RELEASE;

					amountReads += reads;
					OnceRead = TRUE;
				}

				// 만약 Boundary를 찾으면
				if (0 == memcmp(pBoundary, Boundary, BoundaryLen))
				{
					BoundaryFound = TRUE;
					ContentEnd = 0x2D2D == *(wchar_t*)(pBoundary + BoundaryLen);

					break;
				}
				
				pBoundary += 2;
			}

			if (BoundaryFound)
			{
				// 파일 다운로드 중이였다면
				if (FileDownload)
				{
					BytesWritten = (DWORD)(pBoundary - pBoundaryEnd - 2);

					WriteFile(hFile, pBoundaryEnd, BytesWritten, &BytesWritten, NULL); // 파일에 기록한다.
					CloseHandle(hFile); // 파일을 저장한다.

					hFile = INVALID_HANDLE_VALUE;
					FileDownload = FALSE;
				}

				//  Content 가 끝나면 Upload Procedure를 종료한다.
				if (ContentEnd)
				{
					bResult = TRUE;
					goto RELEASE;
				}

				// 만약 헤더의 끝 문자열을 찾을 수 없을 경우
				if (NULL == (pBoundaryEnd = strstr(pBoundary, "\r\n\r\n")))
				{
					DWORD AlreadyReads = amountReads - (DWORD)(pBoundary - Buffer);

					memcpy(Buffer, pBoundary, AlreadyReads); // Boundary가 있는곳부터 복사한다

					if (FALSE == (amountReads = ReadSocketContent(sck, 
						Buffer + AlreadyReads,
						dwBytesToRead - AlreadyReads, &TotalRead, ContentLength)))
						goto RELEASE;

					amountReads = AlreadyReads + amountReads;
					pBoundary = Buffer; // 처음부터 Boundary 의 문자열이 Buffer안에 들어가 있으므로 포인터 변수에 시작주소를 넣는다

					// 그래도 못찾으면 접속종료 시킨다. ( 비정상 )
					if (NULL == (pBoundaryEnd = strstr(Buffer, "\r\n\r\n")))
						goto RELEASE;
				}

				// 바로 헤더 파싱을 시작한다.
				*pBoundaryEnd = 0;

				pBoundaryEnd += 4;
				pPos = pBoundary + BoundaryLen;
				SKIP_RETURNCHAR(pPos);

				// 파일 이름을 추출한다.
				if (0 == (szFileName = HTTP_ExtUploadFileName(pPos)))
					goto RELEASE;

				// 파일 이름에 "가 포함되있는지 확인한다.
				if (szFileName[0] == '"')
				{
					++szFileName;
					szFileName[strlen(szFileName) - 1] = 0;
				}

				// UTF-8을 ANSI로 변환한다.
				UTF8toANSI(szFileName, &szFileName, 0);

				// 파일 이름에 들어가선 안되는 문자가 있는지 검사 후 재배치 한다.
				RemoveSpecialChar(szFileName, strlen(szFileName));

				// 저장될 폴더 경로와 위에서 추출한 문자열이랑 합친다.
				if (-1 == Dswprintf(&szFullPath, "%s\\%s", UploadPath, szFileName))
					return fail_alloc_msg();

				// 중복 파일 체크
				if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(szFullPath))
				{
					LPSTR Ext = szFileName;
					BOOLEAN ctExt = FALSE;
					GETFILEEXT(Ext);

					if (Ext)
					{
						--Ext;
						*Ext = 0;
						ctExt = TRUE;
					}

					free(szFullPath);

					for (DWORD Index = 1; Index < (DWORD)-1; ++Index)
					{
						if (ctExt)
						{
							if (-1 == Dswprintf(&szFullPath, "%s\\%s (%d).%s", UploadPath, szFileName, Index, Ext + 1))
								return fail_alloc_msg();
						}
						else
						{
							if (-1 == Dswprintf(&szFullPath, "%s\\%s (%d)", UploadPath, szFileName, Index))
								return fail_alloc_msg();
						}

						if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(szFullPath))
							break;

						free(szFullPath);
					}
				}

				// CreateFile API로 파일을 생성한다.
				hFile = CreateFileA(
					szFullPath,
					GENERIC_WRITE,
					0,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL
				);

				if (INVALID_HANDLE_VALUE == hFile)
					goto RELEASE;

				FileDownload = TRUE;
				pBoundary = pBoundaryEnd;
			}
			else if (FileDownload)
			{
				BytesWritten = amountReads - (DWORD)(pBoundaryEnd - Buffer);
				WriteFile(hFile, pBoundaryEnd, BytesWritten, &BytesWritten, NULL); 
			}
		}

		amountReads = 0;
		pBoundaryEnd = Buffer;
	}

RELEASE:

	FREE_DATA(Buffer);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);

		if (FALSE == bResult)
			DeleteFile(szFullPath);
	}

	FREE_DATA_NON(szFullPath);

	return bResult;
}

VOID 
HTTP_ProcessMain(
	SOCKET		sck,
	pHdr		pHeader,
	LPCSTR		Path,
	LPCSTR		RelativePath,
	BYTE		IsUploadChat
)
{
	if (FALSE == HTTP_SendFile(sck, pHeader, Path, RelativePath))
	{
		if (FALSE == HTTP_ContextHtml(sck, pHeader, Path, RelativePath, IsUploadChat))
			HTTP_Context404(sck, pHeader);
	}
}

BOOLEAN
HTTP_ProcessUploadProgress(
	SOCKET		sck,
	PSOCKETEX	ClientSckArray,
	pHdr		pHeader
)
{
	QWORD	querylen;
	QWORD	totalread;
	QWORD	ContentLength;
	CHAR	tmp[60];

	if (FALSE == HTTP_ExtUploadProgress(ClientSckArray, MAX_USER, pHeader, &totalread, &ContentLength))
		return FALSE;

	querylen = sprintf_s(tmp, sizeof(tmp), "{\"pos\":%llu, \"length\":%llu}", totalread, ContentLength);

	pHeader->Download = PASS;
	HTTP_SendClientHeader(sck, pHeader, 200, querylen);
	HTTP_SendClientText(sck, tmp, querylen);

	return TRUE;
}

BOOLEAN
HTTP_ProcessIcon(
	SOCKET		sck,
	pHdr		pHeader,
	DWORD		urlLen
)
{
	size_t filesize;
	char* bin = GetResourcesFile(pHeader->RequestURL, &filesize);

	if (bin)
	{
		HTTP_SendFileBinary(sck, pHeader, bin, (DWORD)filesize);
		return TRUE;
	}

	return FALSE;
}

BOOLEAN 
HTTP_ContextHtml(
	SOCKET		sck,
	pHdr		pHeader,
	LPCSTR		Path,
	LPCSTR		RelativePath,
	BYTE		IsUploadChat
)
{
	WIN32_FIND_DATAA	fd;
	LPSTR				PATH = NULL;
	size_t				len = 0;
	HANDLE				hFind;
	LPSTR				utf8 = NULL;
	LPSTR				utf16 = NULL;

	if (0 == RelativePath ||
		0 == *RelativePath)
		RelativePath = "/";

	len = strlen(RelativePath);

	if(RelativePath[len - 1] == '/' || RelativePath[len - 1] == '\\')
	{
		if (-1 == Dswprintf(&PATH, "%s\\%s*.*", Path, RelativePath))
			return fail_alloc_msg();
	}
	else
	{
		if (-1 == Dswprintf(&PATH, "%s\\%s\\*.*", Path, RelativePath))
			return fail_alloc_msg();
	}

	if (INVALID_HANDLE_VALUE == (hFind = FindFirstFileA(PATH, &fd)))
	{
		free(PATH);
		return FALSE;
	}

	ANSItoUTF8(RelativePath, &utf8);

	pHeader->RequestURL = (LPSTR)".html";
	pHeader->Download = PASS;

	HTTP_SendClientHeader(sck, pHeader, 200, 0);

	HTTP_SendClientText(sck, 
		"<html>"
		"<head>"
		"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
		"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
		"<title>Index of %s</title>"
		"</head>"
		"<body>"
		"<h1>Index of %s</h1>", 
		utf8, utf8);

	HTTP_SendClientText(sck,
		"<table>"
		"<tr>"
		"<th valign=\"top\"><img src=\"/blank.gif\"></th>"
		"<th>&nbsp;&nbsp;Name&nbsp;&nbsp;</th>"
		"<th>&nbsp;&nbsp;Last modified&nbsp;&nbsp;</th>"
		"<th>&nbsp;&nbsp;Size&nbsp;&nbsp;</th>"
		"<th>&nbsp;&nbsp;Description&nbsp;&nbsp;</th></tr>");

	HTTP_SendClientText(sck, "<tr><th colspan=\"10\"><hr></th></tr>");
	FREE_DATA(utf8);

	memcpy(PATH, RelativePath, len);

	for (int x = (int)len; x >= 0; --x)
	{
		if (PATH[x] == '/')
		{
			PATH[x] = 0;
			break;
		}

	}

	PATH[len] = 0;

	utf16 = URLEncode(PATH);

	// Parent Directory 출력
	if (len > 1)
	{
		HTTP_SendClientText(sck,
			"<tr>"
			"<td valign=\"top\"><img src=\"/back.gif\"></td>"
			"<td>&nbsp;&nbsp;<a href=\"%s%s\">Parent Directory</a>&nbsp;&nbsp;</td>"
			"<td>&nbsp;&nbsp;&nbsp;&nbsp;</td>"
			"<td align=\"center\">&nbsp;&nbsp;  - &nbsp;&nbsp;</td>"
			"<td>&nbsp;&nbsp;Directory&nbsp;&nbsp;</td>"
			"</tr>",
			0 == PATH || 0 == *PATH ? "/" : "file?v=",
			0 == utf16 || 0 == *utf16 ? "" : utf16
		);
	}

	FREE_DATA(utf16);
	FREE_DATA(PATH);

	// 처음에는 10 할당
	pfileDir	pfD = (pfileDir)malloc(sizeof(fileDir) * 10);
	DWORD		allocSize = 10;
	DWORD		fileCnt = 0;

	if (NULL == pfD)
		return fail_alloc_msg();

	do
	{
		if (*fd.cFileName == '.')
			continue;
		
		SYSTEMTIME stime;

		if (RelativePath[len - 1] == '/')
		{
			if (-1 == Dswprintf(&PATH, "%s%s", RelativePath, fd.cFileName))
				return fail_alloc_msg();
		}
		else
		{
			if (-1 == Dswprintf(&PATH, "%s/%s", RelativePath, fd.cFileName))
				return fail_alloc_msg();
		}

		ANSItoUTF8(fd.cFileName, &utf8);
		utf16 = URLEncode(PATH);

		FileTimeToSystemTime(&fd.ftLastWriteTime, &stime);

		pfD[fileCnt].IsDir = TRUE && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
		pfD[fileCnt].FileName = _strdup(fd.cFileName);
		pfD[fileCnt].FullPath = PATH;
		pfD[fileCnt].stime = stime;
		pfD[fileCnt].utf16 = utf16;
		pfD[fileCnt].utf8 = utf8;

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			pfD[fileCnt].Unit = 0;
		}
		else
		{
			CHAR Unit[30];

			SizeToUnit(Unit, sizeof(Unit), fd.nFileSizeHigh, fd.nFileSizeLow);
			
			pfD[fileCnt].Unit = _strdup(Unit);
		}

		++fileCnt;

		if (fileCnt + 1 >= allocSize)
		{
			allocSize *= 2;

			const void* newBuf = realloc(pfD, sizeof(fileDir) * allocSize);

			if (NULL == newBuf)
				return fail_alloc_msg();

			pfD = (pfileDir)newBuf;
		}
		
	}while (FindNextFileA(hFind, &fd));

	FindClose(hFind);

	// 파일 목록 정렬
	qsort(pfD, fileCnt, sizeof(fileDir), HTTP_SortFunc);

	// 정렬된 파일 목록 보내기
	for (DWORD i = 0; i < fileCnt; ++i)
	{
		if (pfD[i].IsDir)
		{
			HTTP_SendClientText(sck,
				"<tr>"
				"<td valign=\"top\"><img src=\"/folder.gif\"></td>"
				"<td>&nbsp;&nbsp;<a href=\"file?v=%s\">%s/</a>&nbsp;&nbsp;</td>"
				"<td>&nbsp;&nbsp;%4d-%02d-%02d %02d:%02d:%02d&nbsp;&nbsp;</td>"
				"<td align=\"center\">&nbsp;&nbsp;  - &nbsp;&nbsp;</td>"
				"<td>&nbsp;&nbsp;Directory&nbsp;&nbsp;</td>"
				"</tr>",
				pfD[i].utf16,
				pfD[i].utf8,
				pfD[i].stime.wYear,
				pfD[i].stime.wMonth,
				pfD[i].stime.wDay,
				pfD[i].stime.wHour,
				pfD[i].stime.wMinute,
				pfD[i].stime.wSecond
			);
		}
		else
		{
			HTTP_SendClientText(sck,
				"<tr>"
				"<td valign=\"top\"><a href=\"file?vw=1&v=%s\" target=\"_blank\"><img src=\"/unknown.gif\"></a></td>"
				"<td>&nbsp;&nbsp;<a href=\"file?v=%s\">%s</a>&nbsp;&nbsp;</td>"
				"<td>&nbsp;&nbsp;%4d-%02d-%02d %02d:%02d:%02d&nbsp;&nbsp;</td>"
				"<td align=\"right\">&nbsp;&nbsp;%s&nbsp;&nbsp;</td>"
				"<td>&nbsp;&nbsp;File&nbsp;&nbsp;</td>"
				"</tr>",
				pfD[i].utf16,
				pfD[i].utf16,
				pfD[i].utf8,
				pfD[i].stime.wYear,
				pfD[i].stime.wMonth,
				pfD[i].stime.wDay,
				pfD[i].stime.wHour,
				pfD[i].stime.wMinute,
				pfD[i].stime.wSecond,
				pfD[i].Unit
			);
		}

		FREE_DATA_NON(pfD[i].FileName);
		FREE_DATA_NON(pfD[i].FullPath);
		FREE_DATA_NON(pfD[i].utf16);
		FREE_DATA_NON(pfD[i].utf8);
		FREE_DATA_NON(pfD[i].Unit);
	}

	FREE_DATA_NON(pfD);

	HTTP_SendClientText(sck, 
		"<tr><th colspan=\"10\">"
		"<hr></th></tr>"
		"</table><div class=\"foot\">%s", SERVER_NAME);

	if (IsUploadChat & 0x01)
		HTTP_SendClientText_Pure(sck, 
			" - <a href='upload' onclick=\"window.open(this.href, '_blank', 'width=400px,height=105px,toolbars=no,scrollbars=no'); return false;\">Upload</a>", 142);

	if (IsUploadChat & 0x02)
		HTTP_SendClientText_Pure(sck,
			" - <a href='chat' onclick=\"window.open(this.href, '_blank', 'width=600px,height=600px,toolbars=no,scrollbars=no'); return false;\">Chat</a>", 138);

	HTTP_SendClientText_Pure(sck, "</div></body></html>", 20);

	return TRUE;
}

VOID 
HTTP_Context404(
	SOCKET		sck,
	pHdr		pHeader
)
{
	if (NULL == pHeader)
		return;

	size_t		filesize;
	LPCSTR		HTMLCode = GetResourcesFile("404.html", &filesize);

	pHeader->RequestURL = (LPSTR)".html";

	HTTP_SendClientHeader(sck, pHeader, 404, filesize);
	HTTP_SendClientText(sck, HTMLCode);
}

VOID
HTTP_ContextUpload(
	SOCKET		sck,
	pHdr		pHeader
)
{
	DWORD		UTF8Size;
	LPSTR		UTF8;

	LPCSTR		HTMLCode = GetResourcesFile("upload.html", NULL);

	if (NULL == pHeader)
		return;

	pHeader->RequestURL = (LPSTR)".html";

	if (0 == (UTF8Size = ANSItoUTF8(HTMLCode, &UTF8)))
		return;

	pHeader->Download = PASS;
	HTTP_SendClientHeader(sck, pHeader, 200, UTF8Size);
	HTTP_SendClientText_Pure(sck, UTF8, UTF8Size);

	FREE_DATA(UTF8);
}

VOID
HTTP_ContextChat(
	SOCKET		sck,
	pHdr		pHeader,
	LPCSTR		ServerIP,
	WORD		Port
)
{
	DWORD		UTF8Size;
	LPSTR		UTF8 = NULL;
	LPSTR		ReplaceString = NULL;

	size_t		filesize;
	LPCSTR		HTMLCode = GetResourcesFile("chat.html", &filesize);
	
	if (NULL == pHeader)
		return;

	pHeader->RequestURL = (LPSTR)".html";

	CHAR Buf[50];

	sprintf_s(Buf, sizeof(Buf), "%s:%d", ServerIP, Port);

	int res;

	if (-1 == (res = Replace(&ReplaceString, (LPSTR)HTMLCode, "%%%SERVER_IP%%%", Buf)))
	{
		fail_alloc_msg();
		return;
	}

	if (0 == res)
		return;

	if (0 == (UTF8Size = ANSItoUTF8(ReplaceString, &UTF8)))
		goto RELEASE;

	pHeader->Download = PASS;
	HTTP_SendClientHeader(sck, pHeader, 200, UTF8Size);
	HTTP_SendClientText_Pure(sck, UTF8, UTF8Size);

	FREE_DATA(UTF8);

RELEASE:
	FREE_DATA(ReplaceString);
}

int 
HTTP_SortFunc(
	void const* a, void const* b
)
{
	pfileDir c = (pfileDir)a;
	pfileDir d = (pfileDir)b;

	if (c->IsDir != d->IsDir)
		return d->IsDir - c->IsDir;

	return strcmp(c->FileName, d->FileName);
}

char*
HTTP_MIME(
	const char* extensions
)
{
	static MIME_TYPE* store = 0;
	static size_t size = 0;
	static size_t count = 0;

	if (0 == store)
	{
		char* data = GetResourcesFile("mime_types.txt", NULL);

		if (0 == data)
			return NULL;

		size = 10;

		if (0 == (store = (MIME_TYPE*)malloc(sizeof(MIME_TYPE) * size)))
			return NULL;

		// parsing
		char* token;
		int index = 0;
		char* context = NULL;

		for (token = strtok_s(data, ",\r\n", &context);
			token != NULL;
			token = strtok_s(NULL, ",\r\n", &context))
		{
			if (0 == *token)
				continue;

			switch (index++ % 2)
			{
				case 0:
					store[count].Extension = token;
					break;
				case 1:
					store[count].Mime = token;
					count++;

					if (count >= size)
					{
						size *= 2;
						const void* newBuf = realloc(store, sizeof(MIME_TYPE) * size);

						if (0 == newBuf)
						{
							free(store);
							return NULL;
						}

						store = (MIME_TYPE*)newBuf;
					}

					break;
			}
		}
	}

	if (0 != store)
	{
		if (((void*)-1) == extensions)
		{
			free(store);
			store = 0;
			count = 0;
			size = 0;
			return ((void*)-1);
		}
		else if (0 == extensions)
			return NULL;

		const int Index = BinarySearch(
			extensions,
			strlen(extensions),
			store,
			sizeof(MIME_TYPE),
			count,
			offsetof(MIME_TYPE, Extension));

		if (Index >= 0)
			return store[Index].Mime;
	}

	return NULL;
}