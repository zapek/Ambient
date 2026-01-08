#ifndef AMBIENT_STR_H
#define AMBIENT_STR_H
/*
 * $Id: str.h,v 1.8 2017/08/21 06:17:44 cyfm Exp $
 */

extern const UBYTE _hctodarray[256];

static inline ULONG hctod(CONST_STRPTR c)
{
	return _hctodarray[(BYTE)*c];
}


STRPTR stristr(CONST_STRPTR, CONST_STRPTR);

extern const UBYTE _validtextdata[256];
static inline BOOL isbinary(CONST UBYTE c) { return ! _validtextdata[c]; }
static inline BOOL istext  (CONST UBYTE c) { return _validtextdata[c];   }

STRPTR strpassws(STRPTR);
void   strterminate(STRPTR);
ULONG strescape(CONST_STRPTR s, STRPTR out);

extern CONST UBYTE __hex[16];

ULONG uri_encode(CONST_STRPTR, STRPTR);
ULONG uri_decode(CONST_STRPTR, STRPTR);

#endif /* AMBIENT_STR_H */
