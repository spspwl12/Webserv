/* ioapi.h -- IO base function header for compress/uncompress .zip
   part of the MiniZip project - ( http://www.winimage.com/zLibDll/minizip.html )

         Copyright (C) 1998-2010 Gilles Vollant (minizip) ( http://www.winimage.com/zLibDll/minizip.html )

         Modifications for Zip64 support
         Copyright (C) 2009-2010 Mathias Svensson ( http://result42.com )

         For more info read MiniZip_info.txt

*/

#if defined(_WIN32) && (!(defined(_CRT_SECURE_NO_WARNINGS)))
        #define _CRT_SECURE_NO_WARNINGS
#endif

#if defined(__APPLE__) || defined(IOAPI_NO_64) || defined(__HAIKU__) || defined(MINIZIP_FOPEN_NO_64)
// In darwin and perhaps other BSD variants off_t is a 64 bit value, hence no need for specific 64 bit functions
#define FOPEN_FUNC(filename, mode) fopen(filename, mode)
#define FTELLO_FUNC(stream) ftello(stream)
#define FSEEKO_FUNC(stream, offset, origin) fseeko(stream, offset, origin)
#else
#define FOPEN_FUNC(filename, mode) fopen64(filename, mode)
#define FTELLO_FUNC(stream) ftello64(stream)
#define FSEEKO_FUNC(stream, offset, origin) fseeko64(stream, offset, origin)
#endif


#include "ioapi.h"

voidpf call_zopen64 (const zlib_filefunc64_32_def* pfilefunc, const void*filename, int mode) {
    if (pfilefunc->zfile_func64.zopen64_file != NULL)
        return (*(pfilefunc->zfile_func64.zopen64_file)) (pfilefunc->zfile_func64.opaque,filename,mode);
    else
    {
        return (*(pfilefunc->zopen32_file))(pfilefunc->zfile_func64.opaque,(const char*)filename,mode);
    }
}

long call_zseek64 (const zlib_filefunc64_32_def* pfilefunc,voidpf filestream, ZPOS64_T offset, int origin) {
    if (pfilefunc->zfile_func64.zseek64_file != NULL)
        return (*(pfilefunc->zfile_func64.zseek64_file)) (pfilefunc->zfile_func64.opaque,filestream,offset,origin);
    else
    {
        uLong offsetTruncated = (uLong)offset;
        if (offsetTruncated != offset)
            return -1;
        else
            return (*(pfilefunc->zseek32_file))(pfilefunc->zfile_func64.opaque,filestream,offsetTruncated,origin);
    }
}

ZPOS64_T call_ztell64 (const zlib_filefunc64_32_def* pfilefunc, voidpf filestream) {
    if (pfilefunc->zfile_func64.zseek64_file != NULL)
        return (*(pfilefunc->zfile_func64.ztell64_file)) (pfilefunc->zfile_func64.opaque,filestream);
    else
    {
        uLong tell_uLong = (uLong)(*(pfilefunc->ztell32_file))(pfilefunc->zfile_func64.opaque,filestream);
        if ((tell_uLong) == MAXU32)
            return (ZPOS64_T)-1;
        else
            return tell_uLong;
    }
}

void fill_zlib_filefunc64_32_def_from_filefunc32(zlib_filefunc64_32_def* p_filefunc64_32, const zlib_filefunc_def* p_filefunc32) {
    p_filefunc64_32->zfile_func64.zopen64_file = NULL;
    p_filefunc64_32->zopen32_file = p_filefunc32->zopen_file;
    p_filefunc64_32->zfile_func64.zread_file = p_filefunc32->zread_file;
    p_filefunc64_32->zfile_func64.zwrite_file = p_filefunc32->zwrite_file;
    p_filefunc64_32->zfile_func64.ztell64_file = NULL;
    p_filefunc64_32->zfile_func64.zseek64_file = NULL;
    p_filefunc64_32->zfile_func64.zclose_file = p_filefunc32->zclose_file;
    p_filefunc64_32->zfile_func64.zerror_file = p_filefunc32->zerror_file;
    p_filefunc64_32->zfile_func64.opaque = p_filefunc32->opaque;
    p_filefunc64_32->zseek32_file = p_filefunc32->zseek_file;
    p_filefunc64_32->ztell32_file = p_filefunc32->ztell_file;
}



static voidpf ZCALLBACK fopen_file_func(voidpf opaque, const char* filename, int mode) {
    FILE* file = NULL;
    const char* mode_fopen = NULL;
    (void)opaque;
    if ((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER)==ZLIB_FILEFUNC_MODE_READ)
        mode_fopen = "rb";
    else
    if (mode & ZLIB_FILEFUNC_MODE_EXISTING)
        mode_fopen = "r+b";
    else
    if (mode & ZLIB_FILEFUNC_MODE_CREATE)
        mode_fopen = "wb";

    if ((filename!=NULL) && (mode_fopen != NULL))
        file = fopen(filename, mode_fopen);
    return file;
}

static voidpf ZCALLBACK fopen64_file_func(voidpf opaque, const void* filename, int mode) {
    FILE* file = NULL;
    const char* mode_fopen = NULL;
    (void)opaque;
    if ((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER)==ZLIB_FILEFUNC_MODE_READ)
        mode_fopen = "rb";
    else
    if (mode & ZLIB_FILEFUNC_MODE_EXISTING)
        mode_fopen = "r+b";
    else
    if (mode & ZLIB_FILEFUNC_MODE_CREATE)
        mode_fopen = "wb";

    if ((filename!=NULL) && (mode_fopen != NULL))
        file = FOPEN_FUNC((const char*)filename, mode_fopen);
    return file;
}


static uLong ZCALLBACK fread_file_func(voidpf opaque, voidpf stream, void* buf, uLong size) {
    uLong ret;
    (void)opaque;
    ret = (uLong)fread(buf, 1, (size_t)size, (FILE *)stream);
    return ret;
}

static uLong ZCALLBACK fwrite_file_func(voidpf opaque, voidpf stream, const void* buf, uLong size) {
    uLong ret;
    (void)opaque;
    ret = (uLong)fwrite(buf, 1, (size_t)size, (FILE *)stream);
    return ret;
}

static long ZCALLBACK ftell_file_func(voidpf opaque, voidpf stream) {
    long ret;
    (void)opaque;
    ret = ftell((FILE *)stream);
    return ret;
}


static ZPOS64_T ZCALLBACK ftell64_file_func(voidpf opaque, voidpf stream) {
    ZPOS64_T ret;
    (void)opaque;
    ret = (ZPOS64_T)FTELLO_FUNC((FILE *)stream);
    return ret;
}

static long ZCALLBACK fseek_file_func(voidpf opaque, voidpf stream, uLong offset, int origin) {
    int fseek_origin=0;
    long ret;
    (void)opaque;
    switch (origin)
    {
    case ZLIB_FILEFUNC_SEEK_CUR :
        fseek_origin = SEEK_CUR;
        break;
    case ZLIB_FILEFUNC_SEEK_END :
        fseek_origin = SEEK_END;
        break;
    case ZLIB_FILEFUNC_SEEK_SET :
        fseek_origin = SEEK_SET;
        break;
    default: return -1;
    }
    ret = 0;
    if (fseek((FILE *)stream, (long)offset, fseek_origin) != 0)
        ret = -1;
    return ret;
}

static long ZCALLBACK fseek64_file_func(voidpf opaque, voidpf stream, ZPOS64_T offset, int origin) {
    int fseek_origin=0;
    long ret;
    (void)opaque;
    switch (origin)
    {
    case ZLIB_FILEFUNC_SEEK_CUR :
        fseek_origin = SEEK_CUR;
        break;
    case ZLIB_FILEFUNC_SEEK_END :
        fseek_origin = SEEK_END;
        break;
    case ZLIB_FILEFUNC_SEEK_SET :
        fseek_origin = SEEK_SET;
        break;
    default: return -1;
    }
    ret = 0;

    if(FSEEKO_FUNC((FILE *)stream, (z_off64_t)offset, fseek_origin) != 0)
                        ret = -1;

    return ret;
}

static int ZCALLBACK fclose_file_func(voidpf opaque, voidpf stream) {
    int ret;
    (void)opaque;
    ret = fclose((FILE *)stream);
    return ret;
}

static int ZCALLBACK ferror_file_func(voidpf opaque, voidpf stream) {
    int ret;
    (void)opaque;
    ret = ferror((FILE *)stream);
    return ret;
}

void fill_fopen_filefunc(zlib_filefunc_def* pzlib_filefunc_def) {
    pzlib_filefunc_def->zopen_file = fopen_file_func;
    pzlib_filefunc_def->zread_file = fread_file_func;
    pzlib_filefunc_def->zwrite_file = fwrite_file_func;
    pzlib_filefunc_def->ztell_file = ftell_file_func;
    pzlib_filefunc_def->zseek_file = fseek_file_func;
    pzlib_filefunc_def->zclose_file = fclose_file_func;
    pzlib_filefunc_def->zerror_file = ferror_file_func;
    pzlib_filefunc_def->opaque = NULL;
}

void fill_fopen64_filefunc(zlib_filefunc64_def* pzlib_filefunc_def) {
    pzlib_filefunc_def->zopen64_file = fopen64_file_func;
    pzlib_filefunc_def->zread_file = fread_file_func;
    pzlib_filefunc_def->zwrite_file = fwrite_file_func;
    pzlib_filefunc_def->ztell64_file = ftell64_file_func;
    pzlib_filefunc_def->zseek64_file = fseek64_file_func;
    pzlib_filefunc_def->zclose_file = fclose_file_func;
    pzlib_filefunc_def->zerror_file = ferror_file_func;
    pzlib_filefunc_def->opaque = NULL;
}

// ===================== subterfuge

static voidpf ZCALLBACK binary_open_func(voidpf opaque, const void* buf, int size) {
    BID* file = NULL;
    (void)opaque;

    if ((buf == NULL) || (size <= 0))
        return NULL;

    if (0 == (file = (BID*)malloc(sizeof(BID))))
        return NULL;

    file->_base = file->_ptr = (void*)buf;
    file->_bufsiz = size;

    return file;
}

static uLong ZCALLBACK binary_read_func(voidpf opaque, voidpf stream, void* buf, uLong size) {
    uLong ret;
    (void)opaque;

    if (size <= 0)
        return 0;

    BID* file = (BID*)stream;
    uLong readsz = size;

    if ((char*)file->_ptr + readsz > (char*)file->_base + file->_bufsiz)
        readsz = (uLong)(file->_bufsiz - ((char*)file->_ptr - (char*)file->_base));

    if (readsz <= 0)
        return 0;

    if (0 == memcpy(buf, file->_ptr, readsz))
        return 0;

    (char*)file->_ptr += readsz;
    ret = readsz;

    return ret;
}

static uLong ZCALLBACK binary_write_func(voidpf opaque, voidpf stream, const void* buf, uLong size) {
    return -1;
}

static ZPOS64_T ZCALLBACK binary_tell_func(voidpf opaque, voidpf stream) {
    ZPOS64_T ret;
    (void)opaque;

    BID* file = (BID*)stream;

    ret = (ZPOS64_T)((char*)file->_ptr - (char*)file->_base);
    return ret;
}

static long ZCALLBACK binary_seek_func(voidpf opaque, voidpf stream, ZPOS64_T offset, int origin) {
    long ret = 0;
    (void)opaque;

    BID* file = (BID*)stream;

    switch (origin)
    {
    case ZLIB_FILEFUNC_SEEK_CUR:    // SEEK_CUR
    {
        if ((char*)file->_ptr + offset > (char*)file->_base + file->_bufsiz)
            offset = file->_bufsiz - ((char*)file->_ptr - (char*)file->_base);

        file->_ptr = (char*)file->_base + offset;
        ret = 0;
        break;
    }
    case ZLIB_FILEFUNC_SEEK_END:    // SEEK_END
    {
        if (offset > 0 ||
            (__int64)offset < -(__int64)file->_bufsiz)
            return -1;

        file->_ptr = (char*)file->_base + (file->_bufsiz + offset);
        ret = 0;
        break;
    }
    case ZLIB_FILEFUNC_SEEK_SET:    // SEEK_SET
    {
        if (offset > file->_bufsiz)
            offset = file->_bufsiz;

        file->_ptr = (char*)file->_base + offset;
        ret = 0;
        break;
    }
    default: 
        return -1;
    }

    return ret;
}

static int ZCALLBACK binary_close_func(voidpf opaque, voidpf stream) {
    int ret;
    (void)opaque;
    free(stream);
    ret = 0;
    return ret;
}

static int ZCALLBACK binary_error_func(voidpf opaque, voidpf stream) {
    int ret;
    (void)opaque;
    ret = 0;
    return ret;
}

void fill_fopen64_binary(zlib_filefunc64_def* pzlib_filefunc_def) {
    pzlib_filefunc_def->zopen64_file = binary_open_func;
    pzlib_filefunc_def->zread_file = binary_read_func;
    pzlib_filefunc_def->zwrite_file = binary_write_func;
    pzlib_filefunc_def->ztell64_file = binary_tell_func;
    pzlib_filefunc_def->zseek64_file = binary_seek_func;
    pzlib_filefunc_def->zclose_file = binary_close_func;
    pzlib_filefunc_def->zerror_file = binary_error_func;
    pzlib_filefunc_def->opaque = NULL;
}