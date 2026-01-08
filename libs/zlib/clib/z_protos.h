#ifndef CLIB_ZLIB_PROTOS_H
#define CLIB_ZLIB_PROTOS_H
// $Id: z_protos.h,v 1.2 2005/07/04 23:06:12 laire Exp $

LONG deflateInit_(APTR strm, LONG level, const STRPTR version, LONG stream_size);
LONG deflate(APTR strm, LONG flush);
LONG deflateEnd(APTR strm);

LONG inflateInit_(APTR strm, const STRPTR version, LONG stream_size);
LONG inflate(APTR strm, LONG flush);
LONG inflateEnd(APTR strm);

LONG deflateInit2_(APTR strm, LONG  level, LONG  method, LONG  windowBits, LONG  memLevel, LONG strategy, const STRPTR version, LONG stream_size);
LONG deflateSetDictionary(APTR strm, const UBYTE *dictionary, ULONG dictLength);
LONG deflateCopy(APTR dest, APTR source);
LONG deflateReset(APTR strm);
LONG deflateParams(APTR strm, LONG level, LONG strategy);

LONG inflateInit2_(APTR strm, LONG windowBits, const STRPTR version, LONG stream_size);
LONG inflateSetDictionary(APTR strm, const UBYTE *dictionary, ULONG dictLength);
LONG inflateSync(APTR strm);
LONG inflateReset(APTR strm);

LONG compress(UBYTE *dest, ULONG *destLen, const UBYTE *source, ULONG sourceLen);
LONG compress2(UBYTE *dest, ULONG *destLen, const UBYTE *source, ULONG sourceLen, LONG level);
LONG uncompress(UBYTE *dest, ULONG *destLen, const UBYTE *source, ULONG sourceLen);

ULONG adler32(ULONG adler, const UBYTE *buf, ULONG len);
ULONG crc32(ULONG crc, const UBYTE *buf, ULONG len);

#endif /* CLIB_ZLIB_PROTOS_H */
