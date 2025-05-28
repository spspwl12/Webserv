#ifndef WSOCK_H
#define WSOCK_H

#include "union.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// FUNCTION
BOOLEAN
WebSocket_HandShake(
	PSOCKETEX psckEx,
	pHdr pHeader
);

VOID
WebSocket_ReceiveData(
	PSOCKETEX psckEx
);

VOID
WebSocket_ConnectionClose(
	PSOCKETEX psckEx
);

#endif