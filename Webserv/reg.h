#ifndef REG_H
#define REG_H

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdarg.h>
#include <stdio.h>

int ReadReg(const char* path, const char* key, const char* defaultVal, char* buf);
int WriteReg(const char* path, const char* key, char* value);
#endif
