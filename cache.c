/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2007 Ambient Open Source Team
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * $Id: cache.c,v 1.13 2022/01/06 18:11:52 piru Exp $
 */


#include "ambient.h"

/* public */

#include <exec/interrupts.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <emul/emulinterface.h>
#include <proto/exec.h>
#include <proto/btree.h>

/* private */

#include "cache.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "gfx_alpha.h"
#include "time_func.h"

/* unused cacheobject maximum lifetime (seconds) */

#if MORPHOS_BETABUILD
#define CLEANUP_THRESHOLD ( 1 * 60 )
#else
#define CLEANUP_THRESHOLD ( 30 * 60 )
#endif

/* lowmem handler */

static LONG memhandlerfunc(void);

static struct IntData {
	struct EmulLibEntry InterruptFunc;
	struct Interrupt Interrupt;
	struct ExecBase *SysBase;
} mosintdata;

static struct SignalSemaphore cachesem;
static ULONG memhandler_cache_active;


/*  */

static APTR tree;
static APTR pool;
static ULONG global_stamp = 1;

struct cachenode
{
	ULONG stamp;                            /* last usage counter for LRU flushing. */
	ULONG flag;                             /* which value entries are used. we can't just check for 0 there. */
	APTR value[ CACHETAG_MAX ];
};

/* resource duplication and releasing callbacks */

typedef LONG ( *allocResource_t ) ( APTR val, APTR *copy );
typedef void ( *freeResource_t ) ( APTR val );

static LONG allocCopyPtr( APTR val, APTR *copy )
{
	*copy = val;
	return TRUE;
}

static LONG allocCopyBitMap( APTR val, APTR *copy )
{
	APTR sb = val;
	UBYTE *spixels = gfx_bitmap_array( sb );
	ULONG w = gfx_bitmap_width( sb );
	ULONG h = gfx_bitmap_height( sb );
	ULONG bpp = gfx_alpha_ispresent( sb ) ? 4 : 3;

	APTR darray = AllocPooled(pool, w * h * bpp + 12 );

	if ( darray )
	{
		ULONG *header = darray;
		UBYTE *dpixels = (UBYTE*)darray + 12;
		ULONG y, rm = gfx_bitmap_modulo( sb );

		header[ 0 ] = w;
		header[ 1 ] = h;
		header[ 2 ] = bpp;


		for( y=0; y<h; y++)
		{
			if ( bpp == 4 )
			{
				memcpy( dpixels + y * w * 4, spixels + y * rm * 4, w * 4 );
			}
			else
			{
				ULONG x;

				for( x=0; x<w; x++)
				{
					dpixels[ ( y * w + x ) * 3 + 0 ] = spixels[ ( y * rm + x ) * 4 + 1 ];
					dpixels[ ( y * w + x ) * 3 + 1 ] = spixels[ ( y * rm + x ) * 4 + 2 ];
					dpixels[ ( y * w + x ) * 3 + 2 ] = spixels[ ( y * rm + x ) * 4 + 3 ];
				}
			}
		}

		*copy = darray;

		return TRUE;
	}

	return FALSE;
}

static void freeDummy( APTR val UNUSED )
{
}

static void freeBitMap( APTR val )
{
	if ( val )
	{
		const ULONG *header = val;
		const ULONG size = header[0] * header[1] * header[2] + 12;
		FreePooled( pool, val, size );
	}
}

static allocResource_t allocator[ CACHETAG_MAX ] = {
	allocCopyPtr,                               /* CACHETAG_MIMETYPE - just copy pointer */
	allocCopyBitMap,                            /* CACHETAG_THUMBNAIL - copy passed bitmap to buffer */
};

static freeResource_t deallocator[ CACHETAG_MAX ] = {
	freeDummy,                                  /* CACHETAG_MIMETYPE - nothing to free */
	freeBitMap,                                 /* CACHETAG_THUMBNAIL - delete bitmap */
};

/* internal functions */

static void cnTouch( struct cachenode *cn )
{
	if ( cn != NULL )
	{
		cn->stamp = timev();
	}
}

static struct cachenode *cnCreate( ULONG hash UNUSED )
{
	struct cachenode *cn = AllocPooled( pool, sizeof( *cn ) );

	if ( cn != NULL )
	{
		cn->flag = 0;
		cnTouch( cn );
	}

	return cn;
}

static void cnFlush( struct cachenode *cn )
{
	if ( cn != NULL )
	{
		LONG i;

		for(i=0; i<CACHETAG_MAX; i++)
		{
			if ( cn->flag & ( 1<<i ) )
			{
				/* delete contents of each tag using specific routine */

				if ( deallocator[ i ] != NULL )
					deallocator[ i ]( cn->value[ i ] );
			}
		}

		cn->flag = 0;
	}
}

static void cnDelete( struct cachenode *cn )
{
	if ( cn != NULL )
	{
		cnFlush( cn );
		FreePooled( pool, cn, sizeof( *cn ) );
	}
}

static struct cachenode *cnLookup( ULONG hash )
{
	APTR n = FindTreeNodeByKey( tree, (APTR)hash);
	if ( n != NULL )
	{
		struct cachenode *cn = ( struct cachenode* )GetTreeNodeData( tree, n );
		cnTouch( cn );
		return cn;
	}

	return NULL;

}

/* API */

LONG cache_get( ULONG hash, LONG tag, APTR *val )
{
	LONG rc = FALSE;

	if (AttemptSemaphore(&cachesem)) /* we really don't want to be blocked*/
	{
		struct cachenode *cn = cnLookup( hash );

		if ( cn != NULL )
		{
			if ( cn->flag & ( 1<<tag ) )
			{
				/* we only update stamp on read access */

#warning "This makes little sense. The value set by cnLookup is trashed by this walking counter. WTF? - Piru"
				cn->stamp = global_stamp++;

				/* */

				*val = cn->value[ tag ];
				rc = TRUE;

				/*
				 * keep the cache locked for CACHETAG_THUMBNAIL. caller
				 * must unlock after done by calling cache_unlock(). this
				 * avoids a race when called from multiple threads. - Piru
				 */
				if (tag == CACHETAG_THUMBNAIL)
					return rc;
			}
		}

		ReleaseSemaphore(&cachesem);

	}

	return rc;
}

void cache_unlock( void )
{
	ReleaseSemaphore(&cachesem);
}

void cache_set( ULONG hash, LONG tag, APTR val )
{
	if (AttemptSemaphore(&cachesem)) /* we really don't want to be blocked*/
	{
		struct cachenode *cn = cnLookup( hash );

		if ( !cn )
		{
			/* node not in cache. allocate new one and insert */

			cn = cnCreate( hash );

			if ( cn )
			{
				InsertTreeNode( tree, (APTR)hash, (APTR)cn );
			}

		}

		if ( cn )
		{
			if ( cn->flag & ( 1<<tag ) )
			{
				/* delete previous one first */

				if ( deallocator[ tag ] )
					deallocator[ tag ]( cn->value[ tag ] );

				cn->flag &= ~(1<<tag);
			}

			if ( allocator[ tag ] )
			{
				APTR copy;

				if ( allocator[ tag ]( val, &copy ) )
				{
					cn->value[ tag ] = copy;
					cn->flag |= 1<<tag;
				}
			}
		}

		ReleaseSemaphore(&cachesem);

	}
}

static void invalidateFunc(APTR tagp, const APTR n)
{
	struct cachenode *cn = NULL;
	LONG tag = (LONG)tagp;

	if ( !n )
		return;

	cn = ( struct cachenode * )GetTreeNodeData( tree, n );

	if ( cn )
	{
		if ( tag == CACHETAG_ALL )
		{
			/* invalidate all tags = delete key from tree */

			cnFlush( cn );
		}
		else
		{
			if ( cn->flag & ( 1<<tag ) )
			{
				/* delete previous one */

				if ( deallocator[ tag ] )
					deallocator[ tag ]( cn->value[ tag ] );

				cn->flag &= ~(1<<tag);

			}
		}
	}
}

struct disposal_state
{
	ULONG stamp_threshold;
	ULONG counter;
	APTR *node;
};

static void deleteunusedCountFunc(APTR disposal_state_p, const APTR n)
{
	struct cachenode *cn;
	struct disposal_state *ds = ( struct disposal_state* )disposal_state_p;

	if ( !n )
		return;

	cn = ( struct cachenode * )GetTreeNodeData( tree, n );

	if ( cn != NULL && ( cn->stamp <= ds->stamp_threshold || cn->flag == 0 ) )
	{
		ds->counter++;
	}
}

static void deleteunusedMarkFunc(APTR disposal_state_p, const APTR n)
{
	struct cachenode *cn;
	struct disposal_state *ds = ( struct disposal_state* )disposal_state_p;

	if ( !n )
		return;

	cn = ( struct cachenode * )GetTreeNodeData( tree, n );

	if ( cn != NULL && ( cn->stamp <= ds->stamp_threshold || cn->flag == 0 ) )
	{
		*ds->node++ = n;
	}
}

void cache_invalidate( ULONG hash, LONG tag )
{
	/*
	 * Here we want to wait even if someone else has the semaphore. This
	 * ensures that invalidate will work properly every time. - Piru
	 */
	ObtainSemaphore(&cachesem);

	if ( hash )
	{
		/* single tree node */

		APTR n = FindTreeNodeByKey( tree, (APTR)hash);

		if ( n )
			invalidateFunc( (APTR)tag, n );

	}
	else
	{
		/* all tree nodes */

		ForTreeNodes( tree, invalidateFunc, (APTR)tag );

	}

	ReleaseSemaphore(&cachesem);
}

void cache_deleteunused( void )
{
	if (AttemptSemaphore(&cachesem))
	{
		struct disposal_state ds;

		ds.stamp_threshold = timev() - CLEANUP_THRESHOLD;
		ds.counter = 0;

		DB(("Cache cleanup initialized...\n"));

		ForTreeNodes( tree, deleteunusedCountFunc, (APTR)&ds );

		if ( ds.counter > 0 )
		{
			APTR *nodes = AllocPooled( pool, ds.counter * sizeof( APTR ) );
			if ( nodes != NULL )
			{
				LONG i;

				DB(("%d nodes marked for deleting...\n", ds.counter));

				ds.node = nodes;
				ForTreeNodes( tree, deleteunusedMarkFunc, (APTR)&ds );

				for(i=0; i<ds.counter; i++)
				{
					DeleteTreeNode( tree, nodes[ i ] );
				}

				FreePooled( pool, nodes, ds.counter * sizeof( APTR ) );
			}

		}

		DB(("Finished.\n"));
		ReleaseSemaphore(&cachesem);
	}

}

/*        */

static APTR btAlloc( APTR userdata UNUSED, ULONG size )
{
	return AllocPooled( pool, size );
}

static void btFree( APTR userdata UNUSED, APTR mem, ULONG size )
{
	FreePooled( pool, mem, size );
}

static LONG btCompare( APTR userdata UNUSED, APTR keya, APTR keyb )
{
	if ( (ULONG)keya > (ULONG)keyb )
		return 1;
	else if ( (ULONG)keya < (ULONG)keyb )
		return -1;
	else
		return 0;

}

static void btDestroyKey( APTR userdata UNUSED, APTR key UNUSED )
{
}

static void btDestroyData( APTR userdata UNUSED, APTR data )
{
	cnDelete( data );
}

struct BTArgArray argarray = {
	btAlloc,
	btFree,
	btCompare,
	btDestroyKey,
	btDestroyData,
	NULL,
};

ULONG cache_init( void )
{
	/* memory management */
	
	pool = CreatePool( MEMF_ANY, 65536, 1024 );

	if ( pool )
	{
		/* initialize tree */

		tree = CreateTree( BT_DEFAULT, &argarray );

		if ( tree )
		{
			/* install lowmem handler */

			InitSemaphore(&cachesem);

			mosintdata.SysBase = SysBase;
			mosintdata.InterruptFunc.Trap = TRAP_LIB;
			mosintdata.InterruptFunc.Extension = 0;
			mosintdata.InterruptFunc.Func = (void (*)(void))memhandlerfunc;
			mosintdata.Interrupt.is_Node.ln_Type = NT_INTERRUPT;
			mosintdata.Interrupt.is_Node.ln_Pri = 1;
			mosintdata.Interrupt.is_Node.ln_Name = "Ambient's Cache MemHandler";
			mosintdata.Interrupt.is_Data = &mosintdata;
			mosintdata.Interrupt.is_Code = (void(*)(void))&mosintdata.InterruptFunc;

			AddMemHandler(&mosintdata.Interrupt);
			memhandler_cache_active = TRUE;

			/* initialize disposal list */

			/* and we are done */

			return TRUE;
		}

		DeletePool( pool );
	}

	return FALSE;
}

void cache_cleanup( void )
{
	if (memhandler_cache_active)
	{
		RemMemHandler(&mosintdata.Interrupt);
	}

	DeleteTree( tree );
	DeletePool( pool );
}


static LONG memhandlerfunc(void)
{
	struct ExecBase *SysBase = mosintdata.SysBase;

	if (AttemptSemaphore(&cachesem))
	{

		/*
		 * If this is lowmem situation, then we better free whatever we can.
		 */

		FlushTree( tree );

		ReleaseSemaphore(&cachesem);

		return (MEM_ALL_DONE);
	}

	return (MEM_DID_NOTHING);
}

