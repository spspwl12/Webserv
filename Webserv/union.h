#ifndef UNION_H
#define UNION_H

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>

#define MAX_FILEBUF						1024 * 128
#define MAX_HDRBUF						1024
#define MAX_USER						1024
#define MAX_REQ_HDR_LEN					1024
#define SERVER_NAME						"MyWebServ"

#define GETFILENAME(sz)					do{if(!sz)break;for(int i=(int)strlen(sz);i>=0;--i){if(sz[i]	\
										=='/'){sz=sz+i+1;break;}}}while(0)
#define GETFILEEXT(sz)					do{if(!sz)break;int _=0;for(int i=(int)strlen(sz);i>=0;--i){	\
										if(sz[i]=='.'){sz=sz+i+1;_=1;break;}}if(!_)sz=0;}while(0)
#define CLOSEHANDLE(h)					do{HANDLE _h=h;if (_h) CloseHandle(_h);}while(0)

#define SKIP_RETURNCHAR(p)				for(;(*p=='\r'||*p=='\n');++p)*p=0;
#define SKIP_SPACECHAR(p)				for(;*p==' ';++p)*p=0;
#define FREE_DATA(p)					if(p) {free(p); p=NULL;}
#define FREE_DATA_NON(p)				if(p) {free(p);}
#define EXT_PAYLOAD_LENGTH_M(b1, b2)	((unsigned char)b1 << 8 | (unsigned char)b2)

#define PATH_ROOT						1
#define PATH_FILE						2

#define SOCK_HTTP						1
#define SOCK_WEBSOCK					2
#define MUST_FREE_MEMORY

#define WEBSERV_STOP					0
#define WEBSERV_START					1
#define WEBSERV_END						2

#define WEBSERV_NICKLEN					7
#define PASS							3	// Hdr.Download 에 쓰이는 flag

// ENUM
enum methods
{
	_DELETE = 0,
	_GET,
	_HEAD,
	_OPTIONS,
	_PATCH,
	_POST,
	_PUT,
	_TRACE
};

enum _Opcode
{
	Continuation = 0,	// 단편화
	UTF_8,
	Binary,
	Control_Code1,
	Control_Code2,
	Control_Code3,
	Control_Code4,
	Control_Code5,
	Close,
	Ping,
	Pong
};

// typedef

typedef unsigned long long QWORD, *LPQWORD;

typedef struct RequestHdr
{
	LPSTR			Name;
	LPSTR			Value;
	WORD			NameLen;
	CHAR			Alloc;
}ReqHdr, ReqVar, *pReqHdr, *pReqVar;

typedef struct Header
{
	enum methods	HdrMethods;
	LPSTR			RequestURL;
	LPSTR			HTTPVersion;
	pReqHdr			RequestHeader;
	DWORD			RequestHeaderCount;
	pReqHdr			SendHeader;
	DWORD			SendHeaderCount;
	pReqVar			RequestURLQuery;
	DWORD			RequestURLQueryCount;
	BOOLEAN			Download;

	LPSTR			HeaderEnd;
	DWORD			ContentRead;
	LPQWORD			TotalRead;
	LPQWORD			ContentLength;
}Hdr, *pHdr;

typedef struct SOCKET_EXTENSION
{
	SOCKET			sck;
	IN_ADDR			ip;
	BYTE			Type;
	BOOLEAN			RcvPong;
	pHdr			pHeader;
	LPSTR			Session;
}SOCKETEX, *PSOCKETEX;

typedef struct Send_Header
{
	pReqHdr			RequestHeader;
	DWORD			RequestHeaderCount;
}sHdr, *psHdr;

typedef struct _Store_Global_Variable {
	HINSTANCE		hInst;
	HWND			_hDlg;
	CHAR			DOWNLOAD_PATH[MAX_PATH];
	CHAR			UPLOAD_PATH[MAX_PATH];
	CHAR			SERVER_IP[16];
	WORD			SERVER_PORT;
	BYTE			UpChat;
	SOCKETEX		ClientSck[MAX_USER];
	BOOLEAN			bStartServ;
	BOOLEAN			WebServerPingThread;
	INT				WebServerStatus;
	INT				WebServerClientCount;
}_sgv;

typedef struct _wsckHdr
{
	union
	{
		struct
		{
#ifdef __BIG_ENDIAN__
			BOOLEAN		FIN : 1;
			BYTE		RSV : 3;
			BYTE		OpCode : 4;
			BOOLEAN		MASK : 1;
			BYTE		PayloadLength : 7;
#else
			BYTE		OpCode : 4;
			BYTE		RSV : 3;
			BOOLEAN		FIN : 1;
			BYTE		PayloadLength : 7;
			BOOLEAN		MASK : 1;
#endif
		};

		WORD Data;
	};
}sckDataFrame;

typedef struct myfd_set
{
	u_int   fd_count;               /* how many are SET? */
	SOCKET  fd_array[1];			/* an array of SOCKETs */
}myfd_set;

typedef struct MIME_TYPE
{
	CHAR*	Extension;
	CHAR*	Mime;
}MIME_TYPE;

typedef struct _FILE_DIR_INF
{
	BOOLEAN IsDir;
	LPSTR FullPath;
	LPSTR FileName;
	LPSTR utf16;
	LPSTR utf8;
	LPSTR Unit;
	SYSTEMTIME stime;
}fileDir, *pfileDir;

#endif