#include "res.h"
#include "../zlib/unzip.h"
#include "resource/resource.h"
#include "union.h"
#include "unit.h"

#include <stdarg.h>
#include <stdio.h>

#ifdef _DEBUG
    #if defined(_M_X64)
        #pragma comment(lib, "..\\Build\\zlib_x64d.lib")
    #elif defined(_M_IX86)
        #pragma comment(lib, "..\\Build\\zlibd.lib")
    #endif
#else
    #if defined(_M_X64)
        #pragma comment(lib, "..\\Build\\zlib_x64.lib")
    #elif defined(_M_IX86)
        #pragma comment(lib, "..\\Build\\zlib.lib")
    #endif
#endif

#define MAXFILENAME (256)

static int
sortname(
    void const* a, void const* b
)
{
    sbb* c = (sbb*)a;
    sbb* d = (sbb*)b;

    return strcmp(c->name, d->name);
}

static int 
unzipFile(
    unzFile uf,
    sbb**   output,
    size_t* count
) 
{
    unz_global_info64   gi;
    unz_file_info64     file_info;
    int                 err;
    size_t              size;
    char                filename_inzip[256];
    FILE*               fout = NULL;
    char*               buf;
    sbb*                extsbb = 0;

    err = unzGetGlobalInfo64(uf, &gi);

    if (err != UNZ_OK)
        return UNZ_INTERNALERROR;

    if (NULL == (extsbb = (sbb*)malloc(sizeof(sbb) * (size_t)gi.number_entry)))
        return UNZ_INTERNALERROR;

    memset(extsbb, 0, sizeof(sbb) * (size_t)gi.number_entry);

    for (int i = 0; i < gi.number_entry; i++)
    {
        if (UNZ_OK != (err = unzGetCurrentFileInfo64(uf, &file_info, filename_inzip,
            sizeof(filename_inzip), NULL, 0, NULL, 0)))
            goto ERROR_SUC;

        if (UNZ_OK != (err = unzOpenCurrentFilePassword(uf, NULL)))
            goto ERROR_SUC;

        size = (size_t)file_info.uncompressed_size;

        if (NULL == (buf = (void*)malloc(size + 1)))
            goto ERROR_SUC;

        extsbb[i].name = _strdup(filename_inzip);
        extsbb[i].buf = buf;
        extsbb[i].size = size;

        err = unzReadCurrentFile(uf, buf, (unsigned int)size);
        buf[size] = 0;

        if (UNZ_OK != (err = unzCloseCurrentFile(uf)))
            goto ERROR_SUC;

        if (i + 1 >= gi.number_entry)
            break;

        if (UNZ_OK != (err = unzGoToNextFile(uf)))
            goto ERROR_SUC;
    }

    qsort(extsbb, (size_t)gi.number_entry, sizeof(sbb), sortname);

    *output = extsbb;
    *count = (size_t)gi.number_entry;

    return 0;

ERROR_SUC:

    if (extsbb)
    {
        for (int i = 0; i < gi.number_entry; ++i)
        {
            FREE_DATA_NON(extsbb[i].name);
            FREE_DATA_NON(extsbb[i].buf);
        }

        free(extsbb);
    }

    return UNZ_INTERNALERROR;
}

char* 
GetResourcesFile(
	const char* filename,
    size_t*     buflen
)
{
    int ret = 0;
    static sbb* store = 0;
    static size_t count = 0;

    if (0 == store)
    {
        HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_ZIP), RT_RCDATA);

        if (hRes == NULL)
            return NULL;

        HGLOBAL hData = LoadResource(NULL, hRes);
        if (hData == NULL)
            return NULL;

        DWORD dataSize = SizeofResource(NULL, hRes);
        void* pData = LockResource(hData);

        unzFile uf = unzOpenBinary(pData, dataSize);
        ret = unzipFile(uf, &store, &count);
        unzClose(uf);
    }

    if (0 != store)
    {
        if (((void*)-1) == filename)
        {
            for (size_t i = 0; i < count; ++i)
            {
                FREE_DATA_NON(store[i].name);
                FREE_DATA_NON(store[i].buf);
            }

            free(store);
            store = 0;
            count = 0;
            return ((void*)-1);
        }
        else if (0 == filename)
            return NULL;

        const int Index = BinarySearch(
            filename,
            strlen(filename),
            store,
            sizeof(sbb),
            count,
            offsetof(sbb, name));

        if (Index >= 0)
        {
            if (buflen)
                *buflen = store[Index].size;

            return store[Index].buf;
        }
    }
 
    return NULL;
}