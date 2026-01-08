/*
   ambientsupport.library, library skeleton
   Generated with LibMaker 0.9.
*/

/****** ambientsupport.library/background ****************************************
*
* DESCRIPTION
*
* HISTORY
*
*****************************************************************************
*
*/

#include "lib_version.h"
#include "library.h"
#include "internal/altivec.h"

#include <proto/exec.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <exec/system.h>
#include <libraries/query.h>
#include <clib/alib_protos.h>

const char LibName[] = LIBNAME;
extern const char VTag[];

struct AmbientSupportBase *AmbientSupportBase;
struct ExecBase           *SysBase;
struct Library            *UtilityBase;
struct Library            *CyberGfxBase;
struct Library            *CGXDitherBase;
struct Library            *GfxBase;
struct Library            *LayersBase;
struct Library            *IntuitionBase;
struct Library            *DOSBase;


struct Library *LibInit(struct Library *unused, APTR seglist, struct Library *sysb);
struct AmbientSupportBase *lib_init(struct AmbientSupportBase *base, APTR seglist, struct Library *SysBase);
APTR lib_expunge(struct AmbientSupportBase *base);
struct Library *LibOpen(void);
ULONG LibClose(void);
APTR LibExpunge(void);
ULONG LibReserved(void);
BOOL InitResources(struct AmbientSupportBase *base);
VOID FreeResources(struct AmbientSupportBase *base);


BOOL InitResources(struct AmbientSupportBase *base)
{
	if( ( base->MemoryPool = CreatePool(MEMF_SEM_PROTECTED | MEMF_ANY, 65536, 8192 ) ) )
	{
		if ((UtilityBase = OpenLibrary("utility.library", 36)))
		{
			if ((CyberGfxBase = OpenLibrary("cybergraphics.library", 51)))
			{
				if ((CGXDitherBase = OpenLibrary("cgxdither.library", 50)))
				{
					if ((GfxBase = OpenLibrary("graphics.library", 51)))
					{
						if ((LayersBase = OpenLibrary("layers.library", 50)))
						{
							if ((IntuitionBase = OpenLibrary("intuition.library", 51)))
							{
								if ((DOSBase = OpenLibrary("dos.library", 51)))
								{
									AmbientSupportBase = base;
									altivec_init( base );
									return( TRUE );
								}
							}
						}
					}
				}
			}
		}
	}
	return( FALSE );
}


VOID FreeResources(struct AmbientSupportBase *base)
{
	//altivec_cleanup( base );

	if( UtilityBase ) {
		CloseLibrary( UtilityBase );
		UtilityBase = NULL;
	}
	if( CyberGfxBase ) {
		CloseLibrary( CyberGfxBase );
		CyberGfxBase = NULL;
	}
	if( CGXDitherBase ) {
		CloseLibrary( CGXDitherBase );
		CGXDitherBase = NULL;
	}
	if( LayersBase ) {
		CloseLibrary( LayersBase );
		LayersBase = NULL;
	}
	if( GfxBase ) {
		CloseLibrary( GfxBase );
		GfxBase = NULL;
	}
	if( IntuitionBase ) {
		CloseLibrary( IntuitionBase );
		IntuitionBase = NULL;
	}
	if( DOSBase ) {
		CloseLibrary( DOSBase );
		DOSBase = NULL;
	}
	if( base->MemoryPool ) {
		DeletePool( base->MemoryPool );
		base->MemoryPool = NULL;
	}
}


static const struct TagItem RTags[] =
{
	{ QUERYINFOATTR_NAME, (IPTR)LibName },
	{ QUERYINFOATTR_IDSTRING, (IPTR)&VTag[1] },
	{ QUERYINFOATTR_COPYRIGHT, (IPTR)"2014 Guido Mersmann" },
	{ QUERYINFOATTR_DATE, (IPTR)DATE },
	{ QUERYINFOATTR_VERSION, VERSION },
	{ QUERYINFOATTR_REVISION, REVISION },
	{ QUERYINFOATTR_SUBTYPE, QUERYSUBTYPE_LIBRARY },
	{ TAG_END,  0 }
};

static const struct Resident ROMTag
#if __GNUC__ > 2
__attribute__((used))
#endif
=
{
	RTC_MATCHWORD,
	(APTR)&ROMTag,
	(APRR)(&ROMTag + 1),
	RTF_EXTENDED | RTF_PPC,
	VERSION,
	NT_LIBRARY,
	0,
	(char*)LibName,
	VSTRING,
	(APTR)LibInit,
	REVISION,
	(struct TagItem*)RTags
};


void LIB_gfx_AlphaSet();
void LIB_gfx_AlphaIsPresent();
void LIB_gfx_AlphaCompose();
void LIB_gfx_AlphaSetMask();
void LIB_gfx_AlphaSetArrayAdd();
void LIB_gfx_AlphaSetRadial();
void LIB_gfx_AlphaTransfer();
void LIB_gfx_BitmapCreateA();
void LIB_gfx_BitmapCreateFromNative();
void LIB_gfx_BitmapDelete();
void LIB_gfx_BitmapCheckVMem();
void LIB_gfx_AnalyzeAverageBrightness();
void LIB_gfx_AnalyzeAverageAlpha();
void LIB_gfx_BlitTiled();
void LIB_gfx_BlitA();
void LIB_gfx_BlurAlpha();
void LIB_gfx_BlurAlphaTransfer();
void LIB_gfx_CMAPCreate();
void LIB_gfx_CMAPDelete();
void LIB_gfx_CMAPRemap();
void LIB_gfx_DoubleBufferAlloc();
void LIB_gfx_DoubleBufferFree();
void LIB_gfx_DoubleBufferAllocReuse();
void LIB_gfx_LineDrawAA();
void LIB_gfx_MaskCreatePlanar();
void LIB_gfx_MaskCreateChunky8();
void LIB_gfx_MaskCreateInverted();
void LIB_gfx_MaskCreateCGXFastRam();
void LIB_gfx_GetPenSpecValue();
void LIB_gfx_ScaleA();
void LIB_gfx_ScaleCalcAspect();
void LIB_gfx_ScaleCalcAspectConstraints();



APTR JumpTable[] =
{
	(APTR)FUNCARRAY_BEGIN,
	(APTR)FUNCARRAY_32BIT_NATIVE,
	(APTR)LibOpen,
	(APTR)LibClose,
	(APTR)LibExpunge,
	(APTR)LibReserved,
	(APTR)0xFFFFFFFF,
	(APTR)FUNCARRAY_32BIT_SYSTEMV,
	/* func_gfx/gfx_alpha.c */
	(APTR) LIB_gfx_AlphaSet,
	(APTR) LIB_gfx_AlphaIsPresent,
	(APTR) LIB_gfx_AlphaCompose,
	(APTR) LIB_gfx_AlphaSetMask,
	(APTR) LIB_gfx_AlphaSetArrayAdd,
	(APTR) LIB_gfx_AlphaSetRadial,
	(APTR) LIB_gfx_AlphaTransfer,
	/* func_gfx/gfx_bitmap.c */
	(APTR) LIB_gfx_BitmapCreateA,
	(APTR) LIB_gfx_BitmapCreateFromNative,
	(APTR) LIB_gfx_BitmapDelete,
	(APTR) LIB_gfx_BitmapCheckVMem,
	/* func_gfx/gfx_analyze.c */
	(APTR) LIB_gfx_AnalyzeAverageBrightness,
	(APTR) LIB_gfx_AnalyzeAverageAlpha,
	/* func_gfx/gfx_blit.c */
	(APTR) LIB_gfx_BlitTiled,
	(APTR) LIB_gfx_BlitA,
	/* func_gfx/gfx_blur.c */
	(APTR) LIB_gfx_BlurAlpha,
	(APTR) LIB_gfx_BlurAlphaTransfer,
	/* func_gfx/gfx_cmap.c */
	(APTR) LIB_gfx_CMAPCreate,
	(APTR) LIB_gfx_CMAPDelete,
	(APTR) LIB_gfx_CMAPRemap,
	/* func_gfx/gfx_dbuf.c */
	(APTR) LIB_gfx_DoubleBufferAlloc,
	(APTR) LIB_gfx_DoubleBufferFree,
	(APTR) LIB_gfx_DoubleBufferAllocReuse,
	/* func_gfx/gfx_linedraw.c */
	(APTR) LIB_gfx_LineDrawAA,
	/* func_gfx/gfx_mask.c */
	(APTR) LIB_gfx_MaskCreatePlanar,
	(APTR) LIB_gfx_MaskCreateChunky8,
	(APTR) LIB_gfx_MaskCreateInverted,
	(APTR) LIB_gfx_MaskCreateCGXFastRam,
	/* func_gfx/gfx_pen.c */
	(APTR) LIB_gfx_GetPenSpecValue,
	/* func_gfx/gfx_scale.c */
	(APTR) LIB_gfx_ScaleA,
	(APTR) LIB_gfx_ScaleCalcAspect,
	(APTR) LIB_gfx_ScaleCalcAspectConstraints,
	/* define other stuff here */
	(APTR)0xFFFFFFFF,
	(APTR)FUNCARRAY_END
};


struct AmbientSupportBase* lib_init(struct AmbientSupportBase *base, APTR seglist, UNUSED struct Library *sysbase)
{
	InitSemaphore(&base->BaseLock);
	base->Seglist = seglist;
	return base;
}

struct TagItem LibTags[] = {
	{ LIBTAG_FUNCTIONINIT, (IPTR)JumpTable },
	{ LIBTAG_LIBRARYINIT,  (IPTR)lib_init },
	{ LIBTAG_MACHINE,      MACHINE_PPC },
	{ LIBTAG_BASESIZE,     sizeof(struct AmbientSupportBase) },
	{ LIBTAG_SEGLIST,      0 },
	{ LIBTAG_TYPE,         NT_LIBRARY },
	{ LIBTAG_NAME,         0 },
	{ LIBTAG_IDSTRING,     0 },
	{ LIBTAG_FLAGS,        LIBF_CHANGED | LIBF_SUMUSED },
	{ LIBTAG_VERSION,      VERSION },
	{ LIBTAG_REVISION,     REVISION },
	{ LIBTAG_PUBLIC,       TRUE },
	{ TAG_END,             0 }
};

struct Library* LibInit(UNUSED struct Library *unused, APTR seglist, struct Library *sysbase)
{
	SysBase = (APTR) sysbase;

	LibTags[4].ti_Data = (IPTR)seglist;
	LibTags[6].ti_Data = (IPTR)ROMTag.rt_Name;
	LibTags[7].ti_Data = (IPTR)ROMTag.rt_IdString;

	return (NewCreateLibrary(LibTags));
}


struct Library* LibOpen(void)
{
	struct AmbientSupportBase *base = (struct AmbientSupportBase*)REG_A6;
	struct Library *lib = (struct Library*)base;

	ObtainSemaphore(&base->BaseLock);

	if (!base->InitFlag)
	{
		if (InitResources(base)) base->InitFlag = TRUE;
		else
		{
			FreeResources(base);
			lib = NULL;
		}
	}

	if (lib)
	{
		base->LibNode.lib_Flags &= ~LIBF_DELEXP;
		base->LibNode.lib_OpenCnt++;
	}

	ReleaseSemaphore(&base->BaseLock);
	if (!lib) lib_expunge(base);
	return lib;
}


ULONG LibClose(void)
{
	struct AmbientSupportBase *base = (struct AmbientSupportBase*)REG_A6;
	ULONG ret = 0;

	ObtainSemaphore(&base->BaseLock);

	if (--base->LibNode.lib_OpenCnt == 0)
	{
		if (base->LibNode.lib_Flags & LIBF_DELEXP) ret = (ULONG)lib_expunge(base);
	}

	if (ret == 0) ReleaseSemaphore(&base->BaseLock);
	return ret;
}


APTR LibExpunge(void)
{
	struct AmbientSupportBase *base = (struct AmbientSupportBase *)REG_A6;

	return(lib_expunge(base));
}


APTR lib_expunge(struct AmbientSupportBase *base)
{
	APTR seglist = NULL;

	ObtainSemaphore(&base->BaseLock);

	if (base->LibNode.lib_OpenCnt == 0)
	{
		FreeResources(base);
		Forbid();
		Remove((struct Node*)base);
		Permit();
		seglist = base->Seglist;
		FreeMem((UBYTE*)base - base->LibNode.lib_NegSize, base->LibNode.lib_NegSize + base->LibNode.lib_PosSize);
		base = NULL;    /* freed memory, no more valid */
	}
	else base->LibNode.lib_Flags |= LIBF_DELEXP;

	if (base) ReleaseSemaphore(&base->BaseLock);
	return seglist;
}


ULONG LibReserved(void)
{
	return 0;
}
