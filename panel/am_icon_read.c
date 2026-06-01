#define NO_AMIGA_LISTS 1
/* but use these */
#define AROS_ALMOST_COMPATIBLE
#define USE_SHARED_LIBPNG 1
#define PNG_SKIP_SETJMP_CHECK 1

#include <exec/types.h>
#include <intuition/classusr.h>
#include <libraries/png.h>
#include <libraries/mui.h>
#include <libraries/vgraphics.h>
#include <proto/png.h>

#include <proto/exec.h>
#include <setjmp.h>
#include <proto/dos.h>
#include <dos/dos.h>



#include <proto/vgraphics.h>
#include <cybergraphx/cybergraphics.h>
#include <datatypes/pictureclass.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>

#include <clib/datatypes_protos.h>

#include <graphics/rpattr.h>

#include <workbench/workbench.h>



#include <proto/cybergraphics.h>


/*
 * min()/max()/abs() without macro side effects.
 */
#define max(a,b) \
	({typeof(a) _a = (a); \
	typeof(b) _b = (b);	\
	_a > _b ? _a : _b;})

#define min(a,b) \
	({typeof(a) _a = (a); \
	typeof(b) _b = (b); \
	_a > _b ? _b : _a;})

#define abs(a) \
	({typeof(a) _a = (a); \
	_a < 0 ? -_a : _a;})

#define pos(x) \
	({typeof(x) _x = (x); \
	_x > 0 ? _x : 0;})

#define neg(x) \
	({typeof(x) _x = (x); \
	_x < 0 ? _x : 0;})

#define minmax(a,x,b) (max((a),min((x),(b))))

#define swap(a,b) \
	({typeof(a) _swp = a; \
	a = b; b = _swp;})


/* Long word alignement (mainly used to get
 * FIB or DISK_INFO as auto variables)
 */
#define D_S(type,name) char a_##name[sizeof(type)+3]; \
                       type *name = (type *)((IPTR)(a_##name+3) & ~3)


#include "debug.h"

//#include "errormsg.h"

#define memclr(_x, _y) memset(_x, '\0', _y)

/*
 * Some functions return TRUE, FALSE
 * and ASYNC.
 */
#define ASYNC 2

/*
 * Frequently used functions
 */
#define USE_BUILTIN_MATH
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>



#include "../mui_func.h"
#include "../pngicon_specs.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "../image.h"
#include "../classes.h"
#include "name.h"


/*
 * General memory allocation
 */
 
 
APTR icon_pool = 0;

struct ColorMap *magic_wb_cm;
extern struct Library            *CGXDitherBase;



ULONG malloc_init(void);
void malloc_cleanup(void);
ULONG magic_wb_cm_init(void);
ULONG icon_read(STRPTR filename, APTR obj);
ULONG icon_init(void);


#define malloc(size) AllocVecPooled(icon_pool, size)
#define free(ptr) do { if(ptr) FreeVecPooled(icon_pool, ptr); } while(0);


#define icon_malloc(size) AllocVecPooled(icon_pool, size)
#define icon_free(ptr) FreeVecPooled(icon_pool, ptr)



ULONG icon_init(void)
{
	
	if ((icon_pool = CreatePool(MEMF_SEM_PROTECTED | MEMF_ANY, 65536, 8192)))
	{
		magic_wb_cm_init();
	
		return (TRUE);
	}
	return (FALSE);
	
}


#include <proto/asyncio.h>

#define file_open(fn, mode) ((APTR)OpenAsync((STRPTR)fn, (mode == MODE_OLDFILE) ? MODE_READ : (mode == MODE_READWRITE) ? MODE_APPEND : MODE_WRITE, IOBUFFERSIZE))
#define file_read(f, buf, size) (ReadAsync((struct AsyncFile *)f, buf, size) == (LONG)size)
#define file_readmost(f, buf, size) (ReadAsync((struct AsyncFile *)f, buf, size))
#define file_write(f, buf, size) (WriteAsync((struct AsyncFile *)f, (APTR)buf, size) == (LONG)size)

static inline QUAD _seekasync64(struct AsyncFile *f, QUAD p, LONG m)
{
	if (AsyncIOBase->lib_Version >= 50)
		return SeekAsync64(f, p, m);
	if (p >= -0x80000000LL && p <= 0x7fffffffLL)
		return (QUAD)SeekAsync(f, (LONG)p, m);
	return -1LL;
}

#define file_seek(f, pos, mode) (_seekasync64((struct AsyncFile *)f, pos, mode))
#define file_close(f) (CloseAsync((struct AsyncFile *)f))
#define file_readlong(f) ({ULONG v = 0; ReadAsync((struct AsyncFile *)f, &v, sizeof(v)); v;})
#define file_writelong(f, v) (WriteAsync((struct AsyncFile *)f, &v, sizeof(v)) == sizeof(v))
#define file_handle(f) (((struct AsyncFile *)f)->af_File)




static BOOL svg_signature_buffer(const void *buffer, size_t len)
{
	const char *header = buffer;
	return len >= 5 &&
	       header[0] == '<' &&
	       (strncasecmp(header+1, "?xml", 4) == 0 ||
	        strncasecmp(header+1, "svg", 3) == 0 ||
	        memcmp(header+1, "!--", 3) == 0);
}

static BOOL svg_signature(STRPTR filename)
{
	BOOL rc = FALSE;

	BPTR lock = Open(filename, MODE_OLDFILE);

	if(lock)
	{
		char header[5];

		int len = Read(lock, header, 5);

		Close(lock);

		if (svg_signature_buffer(header, len))
			rc = TRUE;
	}

	return rc;
}



static APTR svg_icon_read(STRPTR filename, APTR obj, ULONG mode,  ULONG ancillary)
{
	APTR retval = FALSE;
	
	if(VGraphicsBase)
	{
		APTR svg = VG_ImportSVG(filename, VG_Nano, TRUE, TAG_END);

		if(svg)
		{
			struct RastPort rp;
			FLOAT vec_width;
			FLOAT vec_height;
			FLOAT scalex;
			FLOAT scaley;
			FLOAT movementx = 0;
			FLOAT movementy = 0;

			/* scale the vector to fit a 64x64 icon */
			VG_GetAttr(svg, VG_BBWidth, (ULONG*)&vec_width);
			VG_GetAttr(svg, VG_BBHeight,(ULONG*) &vec_height);

			scalex = (64. / vec_width);
			scaley = (64. / vec_height);

			VG_SetAttrs(svg, VG_ScaleX, (ULONG)&scalex, TAG_END);
			VG_SetAttrs(svg, VG_ScaleY, (ULONG)&scaley, TAG_END);
			VG_SetAttrs(svg, VG_MovementX, (ULONG)&movementx, TAG_END);
			VG_SetAttrs(svg, VG_MovementY, (ULONG)&movementy, TAG_END);

			/* create a dummy rastport */
			InitRastPort(&rp);

			if( (rp.BitMap = AllocBitMap(64, 64, 32, BMF_CLEAR | BMF_SPECIALFMT | SHIFT_PIXFMT(PIXFMT_ARGB32), NULL)) != NULL)
			{
				struct bitmap_ctx *ct;
				if ((ct = malloc(sizeof(*ct))))
				{
					ct->isnative = FALSE;
					ct->usecount = 1;
					ct->ref = NULL;
					ct->width  = 64;
					ct->height = 64;
					ct->depth  = 32;
					ct->bm = rp.BitMap;
					ct->bpr = GetCyberMapAttr(ct->bm, CYBRMATTR_XMOD);
					ct->modulo = ct->bpr / GetCyberMapAttr(ct->bm, CYBRMATTR_BPPIX);

					/* render vectors */
					VG_Render(svg, VGR_DestWidth, 64, VGR_DestHeight, 64, VGR_DestDepth, 32, TAG_DONE);
					/* and blit that */
					VG_Blit(&rp, svg, VGR_RawAlpha, TRUE, TAG_DONE);

					//methodstack_push(obj, 4,
					DoMethod(obj,	MM_Icon_AddBitMap, ct, MV_Icon_BitMap_SVGicon, MV_Icon_BitMap_Normal
					);

					
					retval = (APTR)TRUE;
				}
				else
				{
					FreeBitMap(rp.BitMap);
				
				}
			}
	
			#if defined(BUILD_ICONLIB) || !USE_VECTOR_SCALER
			VG_DisposeVGObject(svg);
			#endif
		}
		
	
		retval = 0;//read_svgtags(filename, obj);
		
	}
	
	return retval;
}








#define IOBUFFERSIZE (8192 * 2)
#define MAXTOOLTYPES (8192)
/*
 * Reads an icon string (braindead format).
 * If 'size' is supplied, store the length there.
 * Protection: no strings longer than 255 chars
 */
 /* public */
#include <proto/exec.h>
#if USE_SHARED_LIBPNG
#if _JBLEN != 59
/* shared libpng (png.library) requires use of very specific setjmp */
#define PNG_SKIP_SETJMP_CHECK 1
#endif
#include <proto/png.h>
#endif

//#include "prefs.h"



static png_voidp png_icon_malloc(png_structp png_ptr, png_size_t size)
{
	return (icon_malloc(size));
}

static void png_icon_free(png_structp png_ptr , png_voidp ptr)
{
	#warning
	icon_free(ptr);
}





static void user_read_data(png_structp png_ptr, png_bytep data, png_uint_32 length)
{
	APTR fh = png_get_io_ptr(png_ptr);

	if (!file_read(fh, data, length))
	{
		/* XXX: print some error message there.. */
	//	PDB(("argh! failed to read\n"));
	//	png_error(png_ptr, "Read error");
	}
}

static const UBYTE png_sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
static ULONG pngio_sig_valid(CONST_STRPTR s)
{
	return (!memcmp(png_sig, s, 8));
}

#ifndef CTABFMT_RGB8 /* XXX: fix that mess in the cgx includes.. and in glowicon/newicon.c too :) */
#define CTABFMT_RGB8 2
#endif



#define ADVANCE_DATA(x) data += (x); len -= (x)

static void pngio_read_tags(APTR obj, UBYTE *data, ULONG len)
{
	struct TagItem *tag;
	ULONG did_setpos = FALSE;
	ULONG did_setdrawer = FALSE;
	/* XXX: we don't check for duplicate tags.. is it needed ? */

	while (len >= sizeof(struct TagItem *))
	{
		tag = (struct TagItem *)data;

		ADVANCE_DATA(sizeof(struct TagItem *));

		//dprintf("tag: %p\n", tag->ti_Tag);

		switch (tag->ti_Tag)
		{
			case PNGICON_LocationX:
				set(obj,MA_Icon_X, tag->ti_Data);
				if (!did_setpos)
				{
					set(obj,MA_Icon_HasPos, TRUE );
					did_setpos = TRUE;
				}
				ADVANCE_DATA(sizeof(ULONG));
				break;

			case PNGICON_LocationY:
				set(obj,MA_Icon_Y, tag->ti_Data);
				if (!did_setpos)
				{
					set(obj,MA_Icon_HasPos, TRUE);
					did_setpos = TRUE;
				}
				ADVANCE_DATA(sizeof(ULONG));
				break;

			case PNGICON_DrawerLeft:
				set(obj,MA_Icon_WindowLeft, tag->ti_Data);
				if (!did_setdrawer)
				{
					set(obj,MA_Icon_HasDrawerData, TRUE);
					did_setdrawer = TRUE;
				}
				ADVANCE_DATA(sizeof(ULONG));
				break;

			case PNGICON_DrawerTop:
				set(obj,MA_Icon_WindowTop, tag->ti_Data);
				if (!did_setdrawer)
				{
					set(obj,MA_Icon_HasDrawerData, TRUE);
					did_setdrawer = TRUE;
				}
				ADVANCE_DATA(sizeof(ULONG));
				break;

			case PNGICON_DrawerWidth:
				set(obj,MA_Icon_WindowWidth, tag->ti_Data);
				if (!did_setdrawer)
				{
					set(obj,MA_Icon_HasDrawerData, TRUE);
					did_setdrawer = TRUE;
				}
				ADVANCE_DATA(sizeof(ULONG));
				break;

			case PNGICON_DrawerHeight:
				set(obj,MA_Icon_WindowHeight, tag->ti_Data);
				if (!did_setdrawer)
				{
					set(obj,MA_Icon_HasDrawerData, TRUE);
					did_setdrawer = TRUE;
				}
				ADVANCE_DATA(sizeof(ULONG));
				break;

			case PNGICON_DrawerViewMode:
				set(obj,MA_Icon_ViewMode, tag->ti_Data);
				if (!did_setdrawer)
				{
					set(obj,MA_Icon_HasDrawerData, TRUE);
					did_setdrawer = TRUE;
				}
				ADVANCE_DATA(sizeof(ULONG));
				break;

			case PNGICON_DrawerSortMode:
				set(obj,MA_Icon_SortMode, tag->ti_Data);
				if (!did_setdrawer)
				{
					set(obj,MA_Icon_HasDrawerData, TRUE);
					did_setdrawer = TRUE;
				}
				ADVANCE_DATA(sizeof(ULONG));
				break;

			case PNGICON_StackSize:
				set(obj,MA_Icon_StackSize, tag->ti_Data);
				ADVANCE_DATA(sizeof(ULONG));
				break;

			/* XXX: we should have a special strlen() that stops after a certain size or so.. */
			case PNGICON_DefaultTool:
				{
					ULONG size;
					size = strlen((STRPTR)&tag->ti_Data) + 1;
					ADVANCE_DATA(size);
				}
				break;

			case PNGICON_ToolType:
				{
					ULONG size;
					size = strlen((STRPTR)&tag->ti_Data) + 1;
				//	PDB(("inserting PNGicon tooltype <%s>, size: %ld\n", &tag->ti_Data, size));
				/*	methodstack_push_sync(obj, 3,
						MM_Icon_InsertToolType, size, &tag->ti_Data
					); *//* XXX: check retval? hm.. */
					ADVANCE_DATA(size);
				}
				break;
		}
	
	}
	
}


/*
 * Handle our private chunk.
 */
static int read_chunk_call_back(png_structp png_ptr, png_unknown_chunkp chunk)
{
	if (chunk->name[0] == 'i'
		&& chunk->name[1] == 'c'
		&& chunk->name[2] == 'O'
		&& chunk->name[3] == 'n'
	)
	{
		pngio_read_tags(png_get_user_chunk_ptr(png_ptr), chunk->data, chunk->size);
		
		return (1); /* chunk was ok */
	}
	else
	{
		return (0);
	}
}


static ULONG pngicon_readimage(png_structp png_ptr, png_infop info_ptr, APTR obj, ULONG width, ULONG height, ULONG imagetype)
{
	ULONG **row_ptr;
	APTR bm;
	ULONG is32;
	
	is32 = (info_ptr->pixel_depth == 32);

	row_ptr = (ULONG **)png_get_rows(png_ptr, info_ptr);

	if ( (bm = gfx_bitmap_create(width, height, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)) )
	{
		ULONG i;

		/*
		 * This sucks but it looks like libpng doesn't set bytesperrow properly
		 * and adds some padding anyway.
		 */
		for (i = 0; i < info_ptr->height; i++)
		{
			gfx_blit(row_ptr[i], bm,
				BLITTAG_SrcType, BLITVAL_SrcType_Array,
				BLITTAG_DstY, i,
				BLITTAG_DstHeight, 1,
				BLITTAG_SrcFormat, is32 ? BLITVAL_SrcFormat_ARGB : BLITVAL_SrcFormat_RGB,
			TAG_DONE);
		}

		
		DoMethod(obj,
			MM_Icon_AddBitMap, bm, MV_Icon_BitMap_PNGicon, imagetype == MV_Icon_BitMap_Normal ? MV_Icon_BitMap_Normal : MV_Icon_BitMap_Selected
		);
		return (TRUE);
	}
	return (FALSE);
}

static ULONG pngicon_read2(APTR fh, APTR obj, ULONG end, ULONG getimage, ULONG imagetype)

{
	volatile ULONG retval = FALSE;
	png_structp png_ptr;

	if ( (png_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL, NULL, png_icon_malloc, png_icon_free)) ) /* XXX: set error funcptrs */
	//if (png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) // keep that line when I decide to fix the memtracker one day.. see ambient.todo
	{
		png_infop info_ptr;

		if ( (info_ptr = png_create_info_struct(png_ptr)) )
		{
			png_infop  end_info;

			if ( (end_info = png_create_info_struct(png_ptr)) )
			{

#if USE_SHARED_LIBPNG //&& _JBLEN != 59
				/* Call compatible setjmp function with shared libpng (png.library) */
				#warning warning
			if (setjmp59(png_jmpbuf(png_ptr)))
#else
			//	if (setjmp(png_jmpbuf(png_ptr)))
		}} 
		#warning
#endif
				{
					png_destroy_read_struct((APTR)&png_ptr, &info_ptr, &end_info);
						
					/* XXX: this is tricky.. beware */

					return (FALSE);
				}
				png_set_read_fn(png_ptr, fh, (png_rw_ptr)user_read_data);

				png_set_sig_bytes(png_ptr, 8);

				if (imagetype == MV_Icon_BitMap_Normal)
				{
					png_set_read_user_chunk_fn(png_ptr, obj, read_chunk_call_back);
				}

				png_set_keep_unknown_chunks(png_ptr, 2, 0, 0); /* libpng sucks.. that's the only way I can get it to work */
			
				if (getimage)
				{
					png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_SWAP_ALPHA | PNG_TRANSFORM_EXPAND, NULL);

					/*
					 * We want RGB or ARGB only.
					 */
					if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE
						|| info_ptr->color_type == PNG_COLOR_TYPE_RGB
						|| info_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA
						|| info_ptr->color_type == PNG_COLOR_TYPE_GRAY_ALPHA /* XXX: not sure about that grey alpha one.. */
					)
					{
					
						pngicon_readimage(png_ptr, info_ptr, obj, info_ptr->width, info_ptr->height, imagetype); /* XXX: check retcode */
						
						retval = TRUE;
					}
					
				}
				else
				{
					png_read_png(png_ptr, info_ptr, 0, NULL);
					retval = TRUE;
				}
				

				png_destroy_read_struct((APTR)&png_ptr, &info_ptr, &end_info);
			}
			else
			{
				png_destroy_read_struct((APTR)&png_ptr, &info_ptr, NULL);
			}
		}
		else
		{
			png_destroy_read_struct((APTR)&png_ptr, NULL, NULL);
		}
	}
	return (retval);
}

static ULONG png_icon_read(APTR fh, APTR obj, ULONG end, ULONG getimage)
{
	ULONG retval = pngicon_read2(fh, obj, FALSE, getimage, MV_Icon_BitMap_Normal);

	if (retval)
	{
		if (getimage)// && _conf(icon_dualpng))
		{
			UBYTE buf[8];

			if (file_read(fh, buf, 8) && pngio_sig_valid(buf))
			{
				pngicon_read2(fh, obj, FALSE, getimage, MV_Icon_BitMap_Selected);
			}
		}

	//	if (end)
	//		methodstack_push_sync(obj, 1, MM_Icon_End);
	}

	return retval;
}


#pragma pack(2)
struct chunk_header {
	ULONG id;
	ULONG len;
};

struct form_chunk_header {
	struct chunk_header header;
	ULONG type;
};


struct face_chunk {
	UBYTE width;          /* icon width - 1 */
	UBYTE height;         /* icon height - 1 */
	UBYTE flags;          /* see below */
	UBYTE aspect;         /* aspect ratio: x is upper nibble and y is lower nibble */
	UWORD maxpalettesize; /* this value is completely useless and unreliable. I have no idea of what it can be used for */
};

#define FCF_FRAMELESS (1UL << 0) /* icon has no frame */

struct image_chunk {
	UBYTE transparentcolornum; /* number of the transparent color */
	UBYTE colornum;            /* number of colors in the palette - 1 */
	UBYTE flags;               /* see below */
	UBYTE imageformat;         /* image storage format */
	UBYTE paletteformat;       /* palette storage format */
	UBYTE depth;               /* image depth */
	UWORD imagesize;           /* in bytes - 1 */
	UWORD palettesize;         /* in bytes - 1 */

	/* image and palette data follows */
};
#pragma pack()



#define GETBITS(x) \
	if (bitshift < depth) \
	{ \
		if (--srclen < 0) \
		{ \
			return (FALSE); \
		} \
		shift |= (UWORD)*in++ << (8 - bitshift); \
		bitshift += 8; \
	} \
	(x) = shift >> (16 - depth); \
	shift <<= depth; \
	bitshift -= depth; \

#define GETBYTE(x) \
	if (bitshift < 8) \
	{ \
		if (--srclen < 0) \
		{ \
			return (FALSE); \
		} \
		shift |= (UWORD)*in++ << (8 - bitshift); \
		bitshift += 8; \
	} \
	(x) = (WORD)shift >> 8; \
	shift <<= 8; \
	bitshift -= 8; \

static ULONG byte_run1_decode(UBYTE *in, LONG srclen, ULONG depth, UBYTE *out, LONG dstlen)
{
	LONG n, bitshift = 0;
	UWORD shift = 0;
	UBYTE b;

	

	while (dstlen > 0)
	{
		GETBYTE(n);
		if (n < 0)
		{
			/*
			 * Copy the next *in + 1 bytes literally.
			 */
			n = -n + 1;
			dstlen -= n;

			if (dstlen < 0)
			{
				return (FALSE);
			}
			GETBITS(b);
			memset(out, b, n);
			out	+= n;
		}
		else
		{
			/*
			 * Replicate the next byte 256 - *in + 1 times.
			 * *in as 255 should be a NOP but PhotoShop didn't
			 * care and no known app uses 255 as NOP so there we go.
			 */
			n += 1;
			dstlen -= n;

			if (dstlen < 0)
			{
				return (FALSE);
			}

			while (n-- > 0)
			{
				GETBITS(b);
				*out++ = b;
			}
		}
	}
	return (TRUE);
}




#include <proto/asyncio.h>
//#if !BUILD_ICONLIB && USE_SHARED_LIBZ
#include <proto/z.h>
//#endif
#include <libraries/iffparse.h>

/* private */
#include "gfx_mask.h"
#include "gfx_alpha.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"


static void *zalloc(void *p, int items, int size)
{
	return AllocVecTaskPooled(items * size);
}

static void zfree(void *p, void *addr)
{
	if (addr)
		FreeVecTaskPooled(addr);
} 
struct face_chunk_sane {
	ULONG width;
	ULONG height;
	ULONG flags;
	ULONG aspect;
};

struct image_chunk_sane {
	ULONG transparentcolornum;
	ULONG colornum;
	ULONG flags;
	ULONG imageformat;
	ULONG paletteformat;
	ULONG depth;
	ULONG imagesize;
	ULONG palettesize;
};

#define ICF_TRANSPARENT (1UL << 0) /* the image has a transparent color */
#define ICF_PALETTE     (1UL << 1) /* the image has palettedata following imagedata */

#define ICFORMAT_NORMAL   0 /* no compression */
#define ICFORMAT_BYTERUN1 1 /* byterun1 compression */


#define ID_ICON MAKE_ID('I','C','O','N')
#define ID_FACE MAKE_ID('F','A','C','E')
#define ID_IMAG MAKE_ID('I','M','A','G')
#define ID_ARGB MAKE_ID('A','R','G','B')

/*
 * Brr, braindead format can have the palette anywhere.
 */
struct glowicon {
	struct face_chunk_sane fc;
	UBYTE *imagedec;   /* decompression buffer */
	UBYTE *palettedec; /* decompression buffer */
	ULONG colornum;
};


static APTR gfx_mask_create_chunky_8(UBYTE * chunky, ULONG width, ULONG height, ULONG bgvalue)
{
	APTR mbm;


	if ((mbm = gfx_bitmap_create(width, height, 1, BITMAPTAG_Clear, TRUE, TAG_DONE)))
	{
		ULONG i, j;
		UBYTE shift;
		UBYTE *p = gfx_bitmap_array(mbm);
		UBYTE *maskline;

		maskline = p;

		for (j = 0; j < height; j++)
		{
			p = maskline;
			shift = (1L << 7);

			for (i = 0; i < width; i++)
			{
				if (!shift)
				{
					shift = (1L << 7);
					p++;
				}

				if (*chunky++ != bgvalue)
				{
					*p |= shift;
				}
				shift >>= 1;
			}
			maskline += gfx_bitmap_modulo(mbm);
		}
	}
	return (mbm);
}

#define gfx__bitmap_modulo(_ctx) (((struct bitmap_ctx *)_ctx)->modulo)
#define gfx__bitmap_array(_ctx) (((struct bitmap_ctx *)_ctx)->bm->Planes[0])

static void gfx__alpha_set(APTR bm, ULONG x, ULONG y, ULONG width, ULONG height, UBYTE val)
{
	{
		ULONG i, j;
		ULONG rw;

		rw = gfx__bitmap_modulo(bm);

		for (i = y; i < y + height; i++)
		{
			UBYTE *array = gfx__bitmap_array(bm) + ( i * rw + x ) * 4;

			j = width;
			do
			{

				*array = val;
				array += 4;

			} while( --j );
		}
	}
}


static ULONG glowicon_read_image(APTR fh, APTR obj, struct glowicon *gi, ULONG mode)
{
	struct chunk_header ch;
	ULONG retval = FALSE;
	/*
	 * Reading chunk header.
	 */
	if (file_read(fh, &ch, sizeof(ch)))
	{
		struct image_chunk icl;
			if (ch.len > sizeof(icl))
		{
			if (file_read(fh, &icl, sizeof(icl)))
			{
				
				APTR bm = NULL;
				if (ch.id == ID_IMAG)
				{
					struct image_chunk_sane ic;
					/*
					 * Correct the braindead fields.
					 */
					ic.transparentcolornum = icl.transparentcolornum;
					ic.colornum            = icl.colornum + 1;
					ic.flags               = icl.flags;
					ic.imageformat         = icl.imageformat;
					ic.paletteformat       = icl.paletteformat;
					ic.depth               = icl.depth;
					ic.imagesize           = icl.imagesize + 1;
					ic.palettesize		   = 0; // bitRocky: set it to 0, because its nowhere initialized!?

					if (ic.flags & ICF_PALETTE)
					{
						ic.palettesize = icl.palettesize + 1;
					}
	
					
					if ( ((ic.imageformat  == ICFORMAT_NORMAL || ic.imageformat   == ICFORMAT_BYTERUN1) &&
						  (ic.paletteformat == ICFORMAT_NORMAL || ic.paletteformat == ICFORMAT_BYTERUN1) ) )
					{
						ULONG calcsize;
						calcsize = sizeof(icl) + ic.imagesize;
			
						if (ic.flags & ICF_PALETTE)
						{
								calcsize += ic.palettesize;
						}

						if (calcsize <= ch.len) /* should be == but some icons are buggy */
						{
							APTR mbm;
							if (ic.flags & ICF_PALETTE)
							{
								gi->colornum = ic.colornum;
							}
						

							if (ic.imageformat == ICFORMAT_NORMAL)
							{
								if (file_read(fh, gi->imagedec, ic.imagesize))
								{
									
								//	D(ICONIO, bug("read properly in NORMAL mode\n"));
								}
							}
							else
							{
								UBYTE *buf;

								/*
								 * Unpack.
								 */
								if ( (buf = icon_malloc(ic.imagesize)) )
								{
									if (file_read(fh, buf, ic.imagesize))
									{
										byte_run1_decode(buf, ic.imagesize, ic.depth, gi->imagedec, gi->fc.width * gi->fc.height * sizeof(UBYTE)); /* XXX: check retcode */
									}
									icon_free(buf);
								}
							
							}

							if (ic.flags & ICF_PALETTE)
							{
								if (gi->palettedec)
								{
									icon_free(gi->palettedec);
								}

								if ( (gi->palettedec = icon_malloc(gi->colornum * 3 * sizeof(UBYTE))) )
								{
									if (ic.paletteformat == ICFORMAT_NORMAL)
									{
										if (ic.palettesize >= gi->colornum * 3 * sizeof(UBYTE))
										{
											if (file_read(fh, gi->palettedec, ic.palettesize))
											{
												//D(ICONIO, bug("palette read properly in normal mode\n"));
											}
											
										}
										
									}
									else
									{
										UBYTE *buf;

										/*
										 * Unpack.
										 */
										if ( (buf = icon_malloc(ic.palettesize)) )
										{
											if (file_read(fh, buf, ic.palettesize))
											{
												byte_run1_decode(buf, ic.palettesize, 8, gi->palettedec, gi->colornum * 3 * sizeof(UBYTE));
												#ifdef DEBUG
											/*	if (db_a[DB_DUMPIMAGE].active)
												{
													dump_image(gi->palettedec, gi->colornum * 3 * sizeof(UBYTE), 3);
												}*/
												#endif /* DEBUG */
											}
											icon_free(buf);
										}
										
									}
								}
							
							}

							/*
							 * Read 1 more byte to pad if needed as
							 * chunks need to have a 2-byte alignement.
							 */
							if ((ic.imagesize + ic.palettesize) & 1)
							{
								file_seek(fh, 1, MODE_CURRENT); /* XXX */
							}

							if ( (bm = gfx_bitmap_create(gi->fc.width, gi->fc.height, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)) )
							{
								gfx_blit(gi->imagedec, bm,
									BLITTAG_SrcType, BLITVAL_SrcType_Array,
									BLITTAG_CMAP, gi->palettedec,
									BLITTAG_CMAPFormat, BLITVAL_CMAPFormat_RGB8,
								TAG_DONE);
								/* XXX: wrong, change if we are the 2nd imagery using palette from the first one */
							}

							if (ic.flags & ICF_TRANSPARENT)
							{
								if ( (mbm = gfx_mask_create_chunky_8(gi->imagedec, gi->fc.width, gi->fc.height, ic.transparentcolornum)) )
								{
									gfx_alpha_set_mask(bm, 0, 0, gi->fc.width, gi->fc.height, mbm, 0xff, TRUE);
									gfx_bitmap_delete(mbm);
								}
								/* XXX */
							}
							else
							{
								gfx__alpha_set(bm, 0, 0, gi->fc.width, gi->fc.height, 0xff);
							}

							retval = TRUE;
						}
					}
				
				}
				else if (ch.id == ID_ARGB)
				{
					ULONG imagesize, readsize;
					UBYTE *buf;
					
					imagesize = icl.imagesize + 1;
					readsize = imagesize & 1 ? imagesize + 1 : imagesize;

					/*
					 * Unpack.
					 */
					
					if ((buf = icon_malloc(readsize)))
					{
					
						if (file_read(fh, buf, readsize))
						{
							z_stream stream;

							stream.next_in = buf;
							stream.avail_in = imagesize;
							stream.next_out = gi->imagedec;
							stream.avail_out = gi->fc.width * gi->fc.height * sizeof(ULONG);
							stream.zalloc = (alloc_func)&zalloc;
							stream.zfree = (free_func)&zfree;

							if (inflateInit(&stream) == Z_OK)
							{
								inflate(&stream, Z_FINISH);
								inflateEnd(&stream);
							}
						}
						icon_free(buf);
					}
					
					if ((bm = gfx_bitmap_create(gi->fc.width, gi->fc.height, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)) )
					{
						gfx_blit(gi->imagedec, bm,
							BLITTAG_SrcType, BLITVAL_SrcType_Array,
							BLITTAG_SrcFormat, BLITVAL_SrcFormat_ARGB,
							TAG_DONE);
					}

				

					retval = TRUE;
				}
	

				
				if (bm)
				{
					DoMethod(obj,MM_Icon_AddBitMap, bm, MV_Icon_BitMap_Glowicon, mode
					);
				}
			
			}
		}
	}
	return (retval); /* XXX: but we should tell that the chunk is just not recognized.. */
}



static ULONG glow_icon_read(APTR fh, APTR obj)
{
	struct glowicon *gi;
	ULONG retval = FALSE;

	if ( (gi = icon_malloc(sizeof(struct glowicon))) )
	{
		struct form_chunk_header fch;
		ULONG xx[40];
		memset(gi, 0, sizeof(*gi));
		/*
		 * Search for 'FORM' chunk header with type 'ICON'.
		 */
		if (file_read(fh, &fch, sizeof(fch)))
		{
			if (fch.header.id == ID_FORM && fch.type == ID_ICON && fch.header.len)
			{
				struct chunk_header ch;
				/*
				 * We must have a FACE chunk.
				 */
				if (file_read(fh, &ch, sizeof(ch)))
				{
					if (ch.id == ID_FACE && ch.len == sizeof(struct face_chunk))
					{
						struct face_chunk fc;
						/*
						 * Found, now read it.
						 */
						if (file_read(fh, &fc, sizeof(fc)))
						{
							/*
							 * Correct the fields. Enough of this lame storage method.
							 */
							gi->fc.width  = fc.width + 1;
							gi->fc.height = fc.height + 1;
							gi->fc.flags  = fc.flags;
							gi->fc.aspect = fc.aspect;
							/*
							 * Allocate a decompression buffer. If no decompression is
							 * needed, the buffer is used anyway. Since both image have
							 * the same resulting size, the allocation is done here.
							 */
							if ( (gi->imagedec = icon_malloc(gi->fc.width * gi->fc.height * sizeof(ULONG))) )
							{
								if (glowicon_read_image(fh, obj, gi, MV_Icon_BitMap_Normal))
								{
									glowicon_read_image(fh, obj, gi, MV_Icon_BitMap_Selected); /* XXX: check retval */
									retval = TRUE;
								}
								
								if (gi->palettedec)
								{
									icon_free(gi->palettedec);
								}
								icon_free(gi->imagedec);
							}
							

						}
					}
				}
			} /* no error */
		}
		icon_free(gi);
	}
	
	return (retval);
}



static BPTR lock_icon(CONST_STRPTR filename )
{
	BPTR l;

	
	{
		l = Lock(filename, ACCESS_READ);
	}

	return (l);
}


static APTR open_icon(CONST_STRPTR filename, ULONG mode)
{
	APTR fh;


	
	{
		fh = file_open(filename, mode);
	}

	return (fh);
}




/*
 * Reads an Icon and sets all the attributes of
 * the IconObject. Is designed to be called from
 * another task.
 *
 * - filename: filename of the file, with .info at the end
 * - obj: object to send methods to (iconclass)
 */
#define ICF_ANCILLARY (1 << 0UL)
#define ICF_POSITION  (1 << 1UL)
#define ICF_END       (1 << 2UL)
#define ICF_INFOWIN   (1 << 3UL)
#define ICF_DEFICON   (1 << 4UL)
#define ICF_ASSIGN    (1 << 5UL)
#define ICF_GETIMAGE  (1 << 6UL)

#define IMAGE_MAX_X 1024
#define IMAGE_MAX_Y 1024
#define IMAGE_MAX_DEPTH 32


//#include "gfx_cmap.h"
//#include "colormap.h"
#include <proto/cgxdither.h>





/*
 * Allocates a MagicWB v2 (with RomIcons/TomIcons extension) ColorMap.
 * Original MagicWB v2 is 8 colors only.
 * We also support MagicWB v1 then.. colors from 16 to 251 will use an
 * arbitrary gradient (better than random colors)
 */

#if 1

static const ULONG magicwb2[] = {
	/* MagicWB v2 */
	0x959595,
	0x000000,
	0xffffff,
	0x3b67a2,
	0x7b7b7b,
	0xafafaf,
	0xaa907c,
	0xffa997
};
static const int magicwb2cnt = sizeof(magicwb2) / sizeof(magicwb2[0]);

static const ULONG romicons[] = {
	/* RomIcons/TomIcons */
	0x0000ff,
	0x283e5b,
	0x608060,
	0xe2d177,
	0xffd4cb,
	0x7a6048,
	0xd2d2d2,
	0xe55d5d
};
static const int romiconscnt = sizeof(romicons) / sizeof(romicons[0]);

static const ULONG magicwb1[] = {
	/* MagicWB v1 */
	0x7b7b7b,
	0xafafaf,
	0xaa907c,
	0xffa997
};
static const int magicwb1cnt = sizeof(magicwb1) / sizeof(magicwb1[0]);


#define UNPACK_32(x) (((x) << 24) | ((x) << 16) | ((x) << 8) | (x))

static inline void LoadRGBArray(const ULONG *from, int start, int cnt, ULONG *array)
{
	const int end = start + cnt;
	int i;

	for (i = start; i < end; i++)
	{
		ULONG rgb;
		UBYTE r, g, b;

		rgb = *from++;

		r = rgb >> 16;
		g = rgb >> 8;
		b = rgb >> 0;

		array[1 + i * 3 + 0] = UNPACK_32(r);
		array[1 + i * 3 + 1] = UNPACK_32(g);
		array[1 + i * 3 + 2] = UNPACK_32(b);
		
	}
}


#define NUMCOLOURS 256

ULONG magic_wb_cm_init(void)
{
	if ((magic_wb_cm = GetColorMap(NUMCOLOURS)))
	{
		ULONG *array;

		array = malloc(sizeof(*array) * (NUMCOLOURS * 3 + 2));
		if (array)
		{
			struct ViewPort fakevp;
			ULONG i;

			array[0] = (NUMCOLOURS << 16) | 0;
			
			LoadRGBArray(magicwb2, 0, magicwb2cnt, array);
			LoadRGBArray(romicons, magicwb2cnt, romiconscnt, array);

			/* Nothing there.. let's put a B&W gradient */
			for (i = magicwb2cnt + romiconscnt;
			     i < (NUMCOLOURS - magicwb1cnt);
			     i++)
			{
				array[1 + i * 3 + 0] = UNPACK_32(i);
				array[1 + i * 3 + 1] = UNPACK_32(i);
				array[1 + i * 3 + 2] = UNPACK_32(i);
			}

			/* MagicWB v1 */
			LoadRGBArray(magicwb1, NUMCOLOURS - magicwb1cnt, magicwb1cnt, array);

			array[1 + NUMCOLOURS * 3] = (0 << 16) | 0;

			/* Neat trick to avoid using SetRGB32CM */
			InitVPort(&fakevp);
			fakevp.ColorMap = magic_wb_cm;
			LoadRGB32(&fakevp, array);

			free(array);
	
			return TRUE;
		}
	}

	return FALSE;
}

#else

#define UNPACK_32(x) (((x << 24) & 0xff000000) | ((x << 16) & 0xff0000) | ((x << 8) & 0xff00) | (x & 0xff))

ULONG magic_wb_cm_init(void)
{
	if ((magic_wb_cm = GetColorMap(256)))
	{
		ULONG i;

		/* MagicWB v2 */
		SetRGB32CM(magic_wb_cm,   0, 0x95959595, 0x95959595, 0x95959595);
		SetRGB32CM(magic_wb_cm,   1, 0x00000000, 0x00000000, 0x00000000);
		SetRGB32CM(magic_wb_cm,   2, 0xffffffff, 0xffffffff, 0xffffffff);
		SetRGB32CM(magic_wb_cm,   3, 0x3b3b3b3b, 0x67676767, 0xa2a2a2a2);
		SetRGB32CM(magic_wb_cm,   4, 0x7b7b7b7b, 0x7b7b7b7b, 0x7b7b7b7b);
		SetRGB32CM(magic_wb_cm,   5, 0xafafafaf, 0xafafafaf, 0xafafafaf);
		SetRGB32CM(magic_wb_cm,   6, 0xaaaaaaaa, 0x90909090, 0x7c7c7c7c);
		SetRGB32CM(magic_wb_cm,   7, 0xffffffff, 0xa9a9a9a9, 0x97979797);

		/* RomIcons/TomIcons */
		SetRGB32CM(magic_wb_cm,   8, 0x00000000, 0x00000000, 0xffffffff);
		SetRGB32CM(magic_wb_cm,   9, 0x28282828, 0x3e3e3e3e, 0x5b5b5b5b);
		SetRGB32CM(magic_wb_cm,  10, 0x60606060, 0x80808080, 0x60606060);
		SetRGB32CM(magic_wb_cm,  11, 0xe2e2e2e2, 0xd1d1d1d1, 0x77777777);
		SetRGB32CM(magic_wb_cm,  12, 0xffffffff, 0xd4d4d4d4, 0xcbcbcbcb);
		SetRGB32CM(magic_wb_cm,  13, 0x7a7a7a7a, 0x60606060, 0x48484848);
		SetRGB32CM(magic_wb_cm,  14, 0xd2d2d2d2, 0xd2d2d2d2, 0xd2d2d2d2);
		SetRGB32CM(magic_wb_cm,  15, 0xe5e5e5e5, 0x5d5d5d5d, 0x5d5d5d5d);

		/* Nothing there.. let's put a B&W gradient */
		for (i = 16; i < 252; i++)
		{
			SetRGB32CM(magic_wb_cm, i, UNPACK_32(i), UNPACK_32(i), UNPACK_32(i));
		}

		/* MagicWB v1 */
		SetRGB32CM(magic_wb_cm, 252, 0x7b7b7b7b, 0x7b7b7b7b, 0x7b7b7b7b);
		SetRGB32CM(magic_wb_cm, 253, 0xafafafaf, 0xafafafaf, 0xafafafaf);
		SetRGB32CM(magic_wb_cm, 254, 0xaaaaaaaa, 0x90909090, 0x7c7c7c7c);
		SetRGB32CM(magic_wb_cm, 255, 0xffffffff, 0xa9a9a9a9, 0x97979797);
	
		return (TRUE);
	}
	return (FALSE);
}

#endif

/*
 * Frees a MagicWB v2 ColorMap.
 */
static void magic_wb_cm_cleanup(void)
{
	if (magic_wb_cm)
	{
		FreeColorMap(magic_wb_cm);
	}
}


STATIC ULONG gfx_cmap_remap(APTR sbm, APTR tbm, APTR cm)
{
	

	RemapMapColours(gfx_bitmap_bm(sbm), gfx_bitmap_bm(tbm), 0, 0, (struct ColorMap *)cm, 0);

	return (TRUE);
}


/*
 * Sets the alpha channel values within a bitmap using a mask.
 * Clears it to 0 otherwise and if clear is TRUE.
 */

void gfx_alpha_set_mask(APTR bm, ULONG x, ULONG y, ULONG width, ULONG height, APTR mask, UBYTE val, ULONG mode)
{
	ULONG i, j, jm, im;
	UBYTE *mp;
	UBYTE m;
	ULONG rw;

	

	rw = gfx_bitmap_modulo(bm);

	for (i = y, im = 0; i < y + height; i++, im++)
	{
		mp = gfx_bitmap_array(mask) + im * gfx_bitmap_modulo(mask);
		m = 1 << 7;

		for (j = x, jm = 0; j < x + width; j++, jm++, (m == 1) ? (m = 1 << 7) : (m >>= 1))
		{
			if (*(mp + ((jm & ~7) >> 3)) & m)
			{
				if (mode == ACM_COMPOSE)
				{
					*(gfx_bitmap_array(bm) + (i * rw + j) * 4) += val;
				}
				else
				{
					*(gfx_bitmap_array(bm) + (i * rw + j) * 4) = val;
				}
			}
			else
			{
				if (mode == ACM_SET_AND_CLEAR || mode == ACM_COMPOSE_AND_CLEAR)
				{
					*(gfx_bitmap_array(bm) + (i * rw + j) * 4) = 0x0;
				}
			}
		}
	}
}




ULONG remap_image(APTR obj, ULONG state, struct Image *img, ULONG imgwidth, APTR imgdata)
{
	LONG i;
	struct BitMap sbm;
	APTR bm;
	APTR tbm;
	APTR mbm;
	UBYTE *planeptr;
	APTR p;

	/*
	 * We remap, although it would be smarter to know if it's a newicon to
	 * avoid that step, so XXX
	 */
	InitBitMap(&sbm, img->Depth, img->Width, img->Height);

	planeptr = (UBYTE *)imgdata;
	
	for (i = 0; i < img->Depth; i++)
	{
		if (img->PlanePick & (1 << i))
		{
			/* normal plane */
			sbm.Planes[i] = (APTR)planeptr;
			planeptr += imgwidth * img->Height;
		}
		else
		{
			if ( (p = icon_malloc(imgwidth * img->Height)) )
			{
				if (img->PlaneOnOff & (1 << i))
				{
					/* full 1 plane */
					memset(p, 0xff, imgwidth * img->Height);
				}
				else
				{
					/* full 0 plane */
					memset(p, 0, imgwidth * img->Height);
				}
				sbm.Planes[i] = p;
			}
			else
			{
				/* XXX */
			}
		}
	}

	if ( (bm = gfx_bitmap_create_from_native(&sbm, img->Width, img->Height)) )
	{
		if ( (mbm = gfx_mask_create_planar(bm, img->Width, img->Height, img->Depth)) )
		{
			if ( (tbm = gfx_bitmap_create(img->Width, img->Height, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)) )
			{
				gfx_cmap_remap(bm, tbm, magic_wb_cm);
				
				gfx_alpha_set_mask(tbm, 0, 0, img->Width, img->Height, mbm, 0xff, TRUE);
				DoMethod(obj,MM_Icon_AddBitMap, tbm, MV_Icon_BitMap_Standard, state);
			}
			gfx_bitmap_delete(mbm);
		}
		gfx_bitmap_delete(bm);
	}

	for (i = 0 ; i < img->Depth ; i++)
	{
		if (!(img->PlanePick & (1 << i)))
		{
			icon_free(sbm.Planes[i]);
		}
	}
	return (TRUE); /* XXX: bogus.. */
}


ULONG read_image(APTR fh, APTR obj, ULONG state, ULONG ancillary)
{
	struct Image img;
	ULONG retval = FALSE;
	ULONG val;
	UWORD xx[100];
	if (file_read(fh, &img, sizeof(img)))
	{
		APTR imgdata;
		ULONG imgwidth = RASSIZE(img.Width, 1);
		ULONG imgsize = imgwidth * img.Height * img.Depth;
		val = sizeof(struct Image);	
		if ( (imgwidth && imgwidth <= IMAGE_MAX_X && img.Height && img.Height <= IMAGE_MAX_Y && img.Depth && img.Depth <= IMAGE_MAX_DEPTH) )
		{	
			if ( (imgdata = icon_malloc(imgsize)) )
			{
				if (file_read(fh, imgdata, imgsize))
				{
					img.ImageData = imgdata;
					val += imgsize;
					retval = val;
				
					if (ancillary)
					{
						DoMethod(obj,MM_Icon_AddAncillary, (state == MV_Icon_BitMap_Normal) ? MV_Icon_Ancillary_ImageNormal : MV_Icon_Ancillary_ImageSelected, imgsize, &img
						);
					}
					remap_image(obj, state, &img, imgwidth, imgdata); /* XXX: check retcode.. */
					
				}
				
				icon_free(imgdata);
			}
		
		}
	}
	
	
	return (retval);
}

static STRPTR icon_read_infostring(APTR fh, ULONG *size)
{
	ULONG len;

	/*
	 * Read ULONG specifying the string size including NULL terminator,
	 * and force that NULL termination.
	 */
	if (file_read(fh, &len, sizeof(len)))
	{
		if (len && (len < 256))
		{
			STRPTR p;

			if ( (p = icon_malloc(len)) )
			{
				if (file_read(fh, p, len))
				{
					p[len - 1] = '\0';

					if (size)
					{
						*size = len;
					}

					return (p);
				}

				//icon_free(p);
			}
		}
	}
	//errormsg(ERR_READERROR); /* XXX: find something better.. */
	return (0);
}


static const TEXT id1[] = "IM1=";
static const TEXT id2[] = "IM2=";
const TEXT newiconstart[41] = "*** DON'T EDIT THE FOLLOWING LINES!! ***";


/*
 * Detects a newicon icon.
 * tt - tooltype string without front ULONG
 */
static LONG newicon_find(APTR fh, UBYTE *tt)
{
	
	/*
	 * Looks for the following 2 tooltypes:
	 * " "
	 * "*** DON'T EDIT THE FOLLOWING LINES!! ***"
	 */
	if (*tt == ' ' && !*(tt + 1))
	{
		STRPTR buf = icon_read_infostring(fh, NULL);

		if (buf)
		{
			ULONG ret = FALSE;
			
			if (!stricmp(newiconstart, buf))
			{
				ret = TRUE;
			}
			icon_free(buf);
			return (ret);
		}
	}
	return (FALSE);
}


/*
 * Finds the next tooltype with the right ID.
 * p - tooltype string with front ULONG size specifier
 * id - ID to look for (4 bytes)
 */
static UBYTE * next_tooltype(APTR fh, CONST_STRPTR id, ULONG *len)
{
	STRPTR buf;

	while (1)
	{
		buf = icon_read_infostring(fh, len);
		
		if (buf)
		{
			if (!strncmp(buf, id, 4))
			{
				break;
			}
			icon_free(buf);
			buf = NULL;
		}
		else
		{
			break;
		}
	}
	return ((UBYTE *)buf);
}



/*
 * Decodes a newicon line. Taken from the newicon sources. This manages to trash
 * the frontwall from time to time. Why ? No idea.. frontwall byte -1 trashed with val 0x0
 */
static LONG newicon_decodeline(CONST UBYTE *from, UBYTE *to, LONG bitsperbyte, LONG maxlen)
{
	WORD inbits = 0;
	LONG inbound = 0;
	UBYTE *origto = to;
	UBYTE mask = (1 << bitsperbyte) - 1;
	LONG zerocount = 0;

	while (zerocount > 0 || *from)
	{
		LONG newbits;

		inbound <<= 7;
		if (!zerocount)
		{
			UBYTE i;

			i = *(from++) - 0x20;
			if (i >= 0x50)
			{
				i -= 0x31;
				if (i >= 0x80)
				{
					zerocount = i - 0x80;
					i = 0;
				}
				else inbound |= i;
			}
			else inbound |= i;
		}
		else zerocount--;

		inbits += 7;

		while ((newbits = inbits - bitsperbyte) >= 0)
		{
			*(to++) = (inbound >> newbits) & mask;
			if (to - origto == maxlen) return (maxlen);
			inbits = newbits;
		}
	}
	return (to - origto);
}


/*
 * Reads a newicon image.
 */
static ULONG newicon_read_image(APTR fh, APTR obj, ULONG mode, ULONG ancillary)
{
	CONST_STRPTR id;
	STRPTR ptr;
	ULONG retval = FALSE;
	ULONG sz;

	

	if (mode == MV_Icon_BitMap_Normal)
	{
		id = id1;
	}
	else
	{
		id = id2;
	}

	/* find image */
	ptr = next_tooltype(fh, id, &sz);

	if (ptr) /* do not set an error if there's no image */
	{
		if (*ptr && (sz - 1) > 8 && (*(ptr + 4) == 'B' || *(ptr + 4) == 'C'))
		{
			ULONG colornum, width, height, len;
			UBYTE *palette;

			if (ancillary)
			{
			/*	methodstack_push_sync(obj, 4,
					MM_Icon_AddAncillary, (mode == MV_Icon_BitMap_Normal) ? MV_Icon_Ancillary_NewiconNormalTT : MV_Icon_Ancillary_NewiconSelectedTT, NULL, ptr
				);*/
			}

			colornum = (*(ptr + 7) - 0x21) * 64 + (*(ptr + 8) - 0x21);

			width = *(ptr + 5) - 0x21;
			height = *(ptr + 6) - 0x21;

			//D(ICONIO, bug("colornum: %ld, width: %ld, height: %ld\n", colornum, width, height));

			len = 3 * colornum * sizeof(UBYTE);

			if (width && height && colornum && (colornum <= 256))
			{
				palette = icon_malloc(len + 7); /* 7: safety margin for newicon_decodeline(), maybe not needed */

				if (palette) 
				{
					ULONG trans;
					UBYTE *chunky;
					ULONG read;
					UBYTE *p = palette;

					/* transparency */
					if (*(ptr + 4) == 'B')
					{
						trans = TRUE; /* color 0 is transparent */
					//	D(ICONIO, bug("transparent\n"));
					}
					else
					{
						trans = FALSE;
					//	D(ICONIO, bug("non transparent\n")); /* XXX: take that into account */
					}

					/* read the palette which starts at byte 9  (first line) */
					read = newicon_decodeline(ptr + 9, p, 8, len);
				
					p += read;
					len -= read;

					icon_free(ptr);

					/* continue reading the next lines */
					while (len > 0)
					{
						ptr = next_tooltype(fh, id, NULL);
						if (ptr && *ptr)
						{
							if (ancillary)
							{
							}
							read = newicon_decodeline(ptr + 4, p, 8, len);
							p += read;
							len -= read;
							icon_free(ptr);
						}
						else
						{
							read = 0; /* marker */
							break;
						}
					}

					if (read)
					{
						/*
						 * Reading the chunky data.
						 */
						len    = width * height * sizeof(UBYTE);
						chunky = icon_malloc(len+7);              /* 7: safety margin too, maybe not needed */

						if (chunky) 
						{
							APTR bm;
							APTR mbm;
							ULONG depth = 1;
							ULONG cn = colornum - 1; /* XXX: maybe remove and process colornum directly */
							p = chunky;

							while (cn >>= 1)
							{
								depth++;
							}

							while (len > 0)
							{
								ptr = next_tooltype(fh, id, NULL);
								if (ptr && *ptr)
								{
									
									read = newicon_decodeline(ptr + 4, p, depth, len);
									p += read;
									len -= read;
									icon_free(ptr);
								}
								else
								{
									
									read = 0; /* abusing var again :) */
									break;
								}
							}

							if (read)
							{
								bm = gfx_bitmap_create(width, height, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE);
								
								if (bm)
								{
									gfx_blit(chunky, bm,
										BLITTAG_SrcType, BLITVAL_SrcType_Array,
										BLITTAG_CMAP, palette,
										BLITTAG_CMAPFormat, BLITVAL_CMAPFormat_RGB8,
									TAG_DONE);

									/* mark success */
								}
								else
								{
									
									read = 0;
								}

								if (read)
								{
									if (trans)
									{
										mbm = gfx_mask_create_chunky8(chunky, width, height, 0);

										if (mbm)
										{
											gfx_alpha_set_mask(bm, 0, 0, width, height, mbm, 0xff, TRUE);
											gfx_bitmap_delete(mbm);
										}
										else
										{
										
											read = FALSE;
										}
									}
									else
									{
										#warning
										//gfx_alpha_set(bm, 0, 0, width, height, 0xff);
									}

									if (read)
									{
										
										retval = TRUE;
									}
								}
							}
							icon_free(palette);
							icon_free(chunky);
						}
						else
						{
						
						}
					}
				}
				else
				{
				
				}
			}
			else
			{
			
			}
		}
		else
		{
			
		}
	}
	return (retval);
}



ULONG icon_read(STRPTR filename, APTR obj)
{
	APTR fh;
	ULONG retval = FALSE;
	BPTR l = 0;
	
	ULONG len = 0;
	ULONG filetype = 0;
	ULONG flags = (ICF_POSITION | ICF_GETIMAGE | ICF_END);
	ULONG icontype = MV_Icon_Type_None;
	
	flags &= ~ICF_POSITION;
	flags |= ICF_DEFICON;

	if ( (l = lock_icon(filename)) )
	{
		
	}
	if (!len)
	{
		D_S(struct FileInfoBlock, fib);
		if (Examine(l, fib))
		{
			len = fib->fib_Size;  /* icon files are supposed to stay <2GB */
		}
	}

	if (!len)
	{
		/*
		 * Bah, can happen with buggy filesystems like
		 * Olsen's smbfs.
		 */
	/*	methodstack_push(obj, 2,
			MM_Icon_ErrorString, "Filesystem error: no file length"
		);*/
		if (flags & ICF_DEFICON)
		{
#warning no default icon
		//	PDB(("%d %s\n",icontype,filename));
		//	deficonpool_apply_default_icon(obj, icontype, filename, NULL);
		}
		UnLock(l);
		return (FALSE);
	}

	if ( (fh = open_icon(filename, MODE_OLDFILE)) )
	{
		struct DiskObject diskobj;
		/*
		 * Read the first 8 bytes to find out the format.
		 */
			
		if (file_read(fh, &diskobj, 8))
		{
			if (diskobj.do_Magic == WB_DISKMAGIC && diskobj.do_Version == WB_DISKVERSION)
			{
				/*
				 * Old icon
				 */
				if (file_read(fh, (UBYTE *)&diskobj + 8, sizeof(diskobj) - 8))
				{
					LONG rev = TRUE; /* that var is abused twice :) */
					if (diskobj.do_DrawerData)
					{
						struct OldDrawerData dd;

						/*
						 * Relic from the past. We have an OldDrawerData structure
						 * here.
						 */
						if (file_read(fh, &dd, sizeof(struct OldDrawerData)))
						{
							SetAttrs(obj,MA_Icon_WindowTop, (ULONG)dd.dd_NewWindow.TopEdge,
								MA_Icon_WindowLeft,	(ULONG)dd.dd_NewWindow.LeftEdge,
								MA_Icon_WindowHeight, (ULONG)dd.dd_NewWindow.Height,
								MA_Icon_WindowWidth, (ULONG)dd.dd_NewWindow.Width,
								MA_Icon_OffsetY, dd.dd_CurrentY,
								MA_Icon_OffsetX, dd.dd_CurrentX,TAG_DONE);

						/*	methodstack_push(obj, 3,
								MUIM_Set,
								MA_Icon_WindowHeight, (ULONG)dd.dd_NewWindow.Height);

							methodstack_push(obj, 3,
								MUIM_Set,
								MA_Icon_WindowWidth, (ULONG)dd.dd_NewWindow.Width);

							methodstack_push(obj, 3,
								MUIM_Set,
								MA_Icon_OffsetY, dd.dd_CurrentY);

							methodstack_push(obj, 3,
								MUIM_Set,
								MA_Icon_OffsetX, dd.dd_CurrentX
							);*/
						}
					}
					if (read_image(fh, obj, MV_Icon_BitMap_Normal, flags & ICF_ANCILLARY))
					{
						if (diskobj.do_Gadget.SelectRender)
						{
							/*
							 * Read in second icon image (selected state).
							 */	
					
							if (!read_image(fh, obj, MV_Icon_BitMap_Selected, flags & ICF_ANCILLARY))
							{
								rev = FALSE;
							}
						}
						
						
							if (rev)
							{
								if (diskobj.do_DefaultTool)
								{
								//	PDB(("reading DefaultTool.., offset 0x%llx\n", file_seek(fh, 0, OFFSET_CURRENT)));
									
									if ( (diskobj.do_DefaultTool = icon_read_infostring(fh, NULL)) )
									
									{
										if (diskobj.do_DefaultTool[0])
										{
									//		PDB(("do_DefaultTool: <%s>\n", diskobj.do_DefaultTool));
											/*methodstack_push(obj, 3,
												MUIM_Set,
												MA_Icon_DefaultTool, diskobj.do_DefaultTool
											);*/
										}
										else
										{
											/*
											 * Bah.. discard those braindead empty strings.
											 */
										//	icon_free(diskobj.do_DefaultTool);
											diskobj.do_DefaultTool = NULL;
										}
									}
								}
							
	
								/*
								 * Tooltypes
								 */
								if (diskobj.do_ToolTypes)
								{
									ULONG ttnum;
									ULONG *ttmem = 0;

									//PDB(("tooltype mess, offset 0x%llx\n", file_seek(fh, 0, OFFSET_CURRENT)));
									/*
									 * XXX: We should protect ourself from buggy icons with
									 * a wrong array size here..
									 */
									if (file_read(fh, &ttnum, sizeof(ttnum))) /* number of *LONGs* to allocate */
									{
										//PDB(("processing %ld of them (array size: %ld)\n", ttnum >> 2, ttnum));
											

											if ( (ttmem = icon_malloc(ttnum)) )
											{
												ULONG idx = 0;
												ULONG sz;
												STRPTR buf;
												ULONG i;
												memset(ttmem, 0, ttnum);

												for (i = 0; i < (ttnum >> 2) - 1; i++) /* there's one tooltype more, even if it doesn't exist physically.. who knows what they had in mind.. */
												{
													if ( (buf = icon_read_infostring(fh, &sz)) )
													{
														ttmem[idx++] = (ULONG)buf;

														
														if (newicon_find(fh, buf)) /* XXX: length.. I think :) */
														{
															
															if (flags & ICF_GETIMAGE)
															
															{
															//	D(ICONIO, bug("reading first newicon image..\n"));
																if (newicon_read_image(fh, obj, MV_Icon_BitMap_Normal, flags & ICF_ANCILLARY))
																{
																//	D(ICONIO, bug("reading second newicon image..\n"));
																	if (!newicon_read_image(fh, obj, MV_Icon_BitMap_Selected, flags & ICF_ANCILLARY))
																	{
																		/*
																		 * That's ok.. 2nd image is optional.
																		 * XXX: detect if there was a real failure
																		 */
																		//D(ICONIO, bug("failed\n"));
																		//rev = 0;
																	}
																}
																
																else
																{
																//	D(ICONIO, bug("failed\n"));
																	rev = 0;
																}
															}
															break; /* out of the loop */
														}
														
														
														
														
														
														
													}
													else
													{
															continue;
													}
												}

											}
											
										}
										else
										{
													ttnum = 0;
										}
									}
									
										
								//	}					
						}
						if (rev)
						{
							QUAD seekpos = 0;
							ULONG has_glow = FALSE;
							APTR glowbuf = NULL;
								
							rev = ((LONG)diskobj.do_Gadget.UserData) & WB_DISKREVISIONMASK;
							/*
							 * And now the IFF extensions.
							*/
							has_glow = flags & ICF_GETIMAGE ? glow_icon_read(fh, obj) : 0;
						
							retval = TRUE;
#if 0					
							if (glowbuf)
							{
								icon_free(glowbuf);
							}
							if (diskobj.do_ToolWindow)
							{
								icon_free(diskobj.do_ToolWindow);
							}
							if (diskobj.do_DefaultTool)
							{
								icon_free(diskobj.do_DefaultTool);
							}
#endif
						}
					}		
				}
			}
			else
			{
				/*
				 * PNG icon
				 */
				if (pngio_sig_valid((UBYTE *)&diskobj))
				{
					retval = png_icon_read(fh, obj, flags & ICF_END, flags & ICF_GETIMAGE);
					if (!retval)
					{
						STRPTR realfile;

						// Slow but you get icon right away
						if ((realfile = name_build_noinfo(filename)) )
						{
#warning no def icon					
					//		retval = deficonpool_apply_default_icon(obj, icontype, realfile, (APTR)DEFICON_MIMETYPE_RECOGNIZE);
					//		name_delete(realfile);
						}	
					}
				}
			//	PDB(("Testing for svg icon %s\n", filename));
				if(!retval && svg_signature(filename))
				{
					
					/* Vector icons in SVG format */
					retval = (ULONG)svg_icon_read(filename, obj, flags & ICF_END, 0);
				}
			}
			
		}
		file_close(fh);
	}
	UnLock(l);
	return (retval);
}
