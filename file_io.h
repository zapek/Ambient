#ifndef AMBIENT_DOS_IO_H
#define AMBIENT_DOS_IO_H
/*
 * $Id: file_io.h,v 1.4 2006/08/08 13:31:34 fab Exp $
 */

#if USE_ASYNC_IO
#include <proto/asyncio.h>
#else
#include <proto/dos.h>
#include <dos/dosextens.h>
#ifdef BUILD_ICONLIB
#include <dos/stdio.h>
#endif
#endif


#ifdef BUILD_ICONLIB
#define file_open(fn, mode) ((APTR)Open(fn, mode))
#define file_read(f, buf, size) (FRead((BPTR)f, buf, 1, size) == (LONG)size)
#define file_readmost(f, buf, size) (FRead((BPTR)f, buf, 1, size))
#define file_write(f, buf, size) (FWrite((BPTR)f, (APTR) buf, 1, size) == (LONG)size)
#if USE_LEGACY
static inline QUAD _seek64(BPTR f, QUAD p, LONG m)
{
	if (p >= -0x80000000LL && p <= 0x7fffffffLL)
		return (QUAD)Seek(f, (LONG)p, m);
	return -1LL;
}
#else
static inline QUAD _seek64(BPTR f, QUAD p, LONG m)
{
	if (DOSBase->dl_lib.lib_Version >= 51)
		return Seek64(f, p, m);
	if (p >= -0x80000000LL && p <= 0x7fffffffLL)
		return (QUAD)Seek(f, (LONG)p, m);
	return -1LL;
}
#endif
#define file_seek(f, pos, mode) (_seek64((BPTR)f, pos, mode))
#define file_close(f) (Close((BPTR)f))
#define file_handle(f) (f)
#else
#if USE_ASYNC_IO
#define file_open(fn, mode) ((APTR)OpenAsync((STRPTR)fn, (mode == MODE_OLDFILE) ? MODE_READ : (mode == MODE_READWRITE) ? MODE_APPEND : MODE_WRITE, IOBUFFERSIZE))
#define file_read(f, buf, size) (ReadAsync((struct AsyncFile *)f, buf, size) == (LONG)size)
#define file_readmost(f, buf, size) (ReadAsync((struct AsyncFile *)f, buf, size))
#define file_write(f, buf, size) (WriteAsync((struct AsyncFile *)f, (APTR)buf, size) == (LONG)size)
#if USE_LEGACY
static inline QUAD _seekasync64(struct AsyncFile *f, QUAD p, LONG m)
{
	if (p >= -0x80000000LL && p <= 0x7fffffffLL)
		return (QUAD)SeekAsync(f, (LONG)p, m);
	return -1LL;
}
#else
static inline QUAD _seekasync64(struct AsyncFile *f, QUAD p, LONG m)
{
	if (AsyncIOBase->lib_Version >= 50)
		return SeekAsync64(f, p, m);
	if (p >= -0x80000000LL && p <= 0x7fffffffLL)
		return (QUAD)SeekAsync(f, (LONG)p, m);
	return -1LL;
}
#endif
#define file_seek(f, pos, mode) (_seekasync64((struct AsyncFile *)f, pos, mode))
#define file_close(f) (CloseAsync((struct AsyncFile *)f))
#define file_readlong(f) ({ULONG v = 0; ReadAsync((struct AsyncFile *)f, &v, sizeof(v)); v;})
#define file_writelong(f, v) (WriteAsync((struct AsyncFile *)f, &v, sizeof(v)) == sizeof(v))
#define file_handle(f) (((struct AsyncFile *)f)->af_File)
#else
#define file_open(fn, mode) ((APTR)Open(fn, mode))
#define file_read(f, buf, size) (Read((BPTR)f, buf, size) == (LONG)size)
#define file_readmost(f, buf, size) (Read((BPTR)f, buf, size))
#define file_write(f, buf, size) (Write((BPTR)f, (APTR)buf, size) == (LONG)size)
#if USE_LEGACY
static inline QUAD _seek64(BPTR f, QUAD p, LONG m)
{
	if (p >= -0x80000000LL && p <= 0x7fffffffLL)
		return (QUAD)Seek(f, (LONG)p, m);
	return -1LL;
}
#else
static inline QUAD _seek64(BPTR f, QUAD p, LONG m)
{
	if (DOSBase->dl_lib.lib_Version >= 51)
		return Seek64(f, p, m);
	if (p >= -0x80000000LL && p <= 0x7fffffffLL)
		return (QUAD)Seek(f, (LONG)p, m);
	return -1LL;
}
#endif
#define file_seek(f, pos, mode) (_seek64((BPTR)f, pos, mode))
#define file_close(f) (Close((BPTR)f))
#define file_readlong(f) ({ULONG v = 0; Read((BPTR)f, &v, sizeof(v)); v;})
#define file_writelong(f, v) (Write((BPTR)f, &v, sizeof(v)) == sizeof(v))
#define file_handle(f) (f)
#endif
#endif

#endif /* AMBIENT_DOS_IO_H */
