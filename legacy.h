#ifndef AMBIENT_LEGACY_H
#define AMBIENT_LEGACY_H
/*
 * $Id: legacy.h,v 1.2 2006/08/08 13:31:34 fab Exp $
 */


/*  detect MorphOS >1.4.x
 */
extern ULONG EnableExtensions;            /* initialized in main.c */

/*  detect MUI4
 */
extern ULONG EnableMUI4Extensions;        /* initialized in main.c */



/*  first we check if we have the header included after that we check if we might miss 
 *  defines or protos. This avoids useless defines if they are not actually required.
 */

/*  defines 
 */
#ifdef CYBERGRAPHX_CYBERGRAPHICS_H

#ifndef POP_BRIGHTEN
#define POP_BRIGHTEN   0
#endif

#ifndef POP_DARKEN
#define POP_DARKEN     1
#endif

#ifndef POP_SETALPHA
#define POP_SETALPHA   2
#endif

#ifndef POP_TINT
#define POP_TINT       3
#endif

#ifndef POP_BLUR
#define POP_BLUR       4
#endif

#ifndef POP_COLOR2GREY
#define POP_COLOR2GREY 5
#endif

#ifndef POP_NEGATIVE
#define POP_NEGATIVE   6
#endif

#ifndef POP_NEGFADE
#define POP_NEGFADE    7
#endif

#ifndef POP_TINTFADE
#define POP_TINTFADE   8
#endif

#ifndef POP_GRADIENT
#define POP_GRADIENT   9
#endif

#ifndef POP_SHIFTRGB
#define POP_SHIFTRGB   10
#endif

#endif /* CYBERGRAPHX_CYBERGRAPHICS_H */


#ifdef DATATYPES_SOUNDCLASS_H

#pragma pack(2)
struct _sdtFetch
{
	ULONG MethodID;
	APTR  sdtf_Buffer;
	ULONG sdtf_Length;
	ULONG sdtf_Actual;
	ULONG sdtf_EndOfStream;
};
#pragma pack()

#endif /* DATATYPES_SOUNDCLASS_H */



/*  protos
 */
#if USE_LEGACY
#ifdef PROTO_CYBERGRAPHICS_H

#ifndef ProcessPixelArray
VOID ProcessPixelArray(struct RastPort *, ULONG, ULONG, ULONG, ULONG, ULONG, LONG, struct TagItem *);
#endif

#ifndef WritePixelArrayAlpha
ULONG WritePixelArrayAlpha(APTR, UWORD, UWORD, UWORD, struct RastPort *, UWORD, UWORD, UWORD, UWORD, ULONG);
#endif

#endif /* PROTO_CYBERGRAPHICS_H */
#endif /* USE_LEGACY */


#endif /* AMBIENT_LEGACY_H */
