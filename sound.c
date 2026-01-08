/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2007 Ambient Open Source Team
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
 * $Id: sound.c,v 1.9 2012/09/03 05:07:58 geit Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "sound.h"
#include "snddrv_ahi.h"
#include "snddrv_audiodev.h"
#include "tags.h"
#include "snd_codecs.h"
#include "vars.h"

/************************************************************************/

#define USE_AUDIODEV 0

/************************************************************************/

struct sound_ctx {
	APTR drvctx;
	struct TagItem *tags;
	APTR (*snddrv_open)        ( const struct TagItem *tags );
	void (*snddrv_close)       ( APTR drvctx );
	LONG (*snddrv_write_async) ( APTR drvctx, CONST_APTR buf, ULONG len );
	void (*snddrv_abort)       ( APTR drvctx );
	ULONG (*snddrv_wait)       ( APTR drvctx );
	ULONG (*snddrv_check)      ( APTR drvctx );
};

/************************************************************************/

ULONG sound_init( void )
{
	if( snd_codecs_init( ) )
	{
		//if (init_snd_drivers())
		{
			return( TRUE );
		}
	}
	return( FALSE ); /* XXX: we could handle things a bit better, sound is not *necessary* */
}

/************************************************************************/

void sound_cleanup( void )
{
	snd_codecs_cleanup();
}

/************************************************************************/

APTR v_sound_create( ULONG mode UNUSED, const struct TagItem *tags )
{
	struct sound_ctx *ct;

	if( ( ct = malloc( sizeof( *ct ) ) ) )
	{
		memset( ct, 0, sizeof( *ct ) );

		/* default for now */
		#if USE_AUDIODEV
		if( _var( audiodev ) )
		{
			ct->snddrv_open        = snddrv_audiodev_open;
			ct->snddrv_close       = snddrv_audiodev_close;
			ct->snddrv_write_async = snddrv_audiodev_write_async;
			ct->snddrv_abort       = snddrv_audiodev_abort;
			ct->snddrv_wait        = snddrv_audiodev_wait;
			ct->snddrv_check       = snddrv_audiodev_check;
		}
		else
		#endif
		{
			ct->snddrv_open        = snddrv_ahi_open;
			ct->snddrv_close       = snddrv_ahi_close;
			ct->snddrv_write_async = snddrv_ahi_write_async;
			ct->snddrv_abort       = snddrv_ahi_abort;
			ct->snddrv_wait        = snddrv_ahi_wait;
			ct->snddrv_check       = snddrv_ahi_check;
		}

		if( ( ct->tags = tags_clone( tags ) ) )
		{
			if( ( ct->drvctx = ct->snddrv_open( ct->tags ) ) )
			{
				return( ct );
			}
			tags_free( ct->tags );
		}
		free( ct );
	}
	return( NULL );
}

/************************************************************************/

void sound_delete( APTR ctx )
{
	struct sound_ctx *ct = ctx;

	ASSERT( ct );

	ct->snddrv_close( ct->drvctx );

	tags_free( ct->tags );
	
	free( ct );
}

/************************************************************************/

ULONG sound_play_sync( APTR ctx, CONST_APTR buf, ULONG len )
{
	struct sound_ctx *ct = ctx;
	ULONG rc = FALSE;

	ASSERT( ct );

	if( ( rc = ct->snddrv_write_async( ct->drvctx, buf, len ) ) == TRUE )
	{
		ct->snddrv_wait( ct->drvctx );
	}
	return( rc );
}

/************************************************************************/

ULONG sound_play_async( APTR ctx, CONST_APTR buf, ULONG len )
{
	struct sound_ctx *ct = ctx;

	ASSERT( ct );

	return( ct->snddrv_write_async( ct->drvctx, buf, len ) );
}

/************************************************************************/

ULONG sound_wait( APTR ctx )
{
	struct sound_ctx *ct = ctx;

	ASSERT( ct );

	return( ct->snddrv_wait( ct->drvctx ) );
}

/************************************************************************/

void sound_abort( APTR ctx )
{
	struct sound_ctx *ct = ctx;

	ASSERT( ct );

	ct->snddrv_abort( ct->drvctx );
}

/************************************************************************/

APTR sound_getattr( APTR ctx, ULONG attr )
{
	ASSERT( ctx );

	if( ctx == (APTR) SOUNDCONTEXT_DEFAULT )
	{
		switch( attr )
		{
			case SOUNDATTR_DriverName:
				#if USE_AUDIODEV
				if( _var( audiodev ) )
				{
					return( (APTR) "audiodev" );
				} else
				#endif
				{
					return( (APTR) "AHI" );
				}
				break;

			#ifdef DEBUG
			default:
				PDB(("unknown attr %ld\n", attr));
				break;
			#endif
		}
	} else {
		struct sound_ctx *ct = ctx;

		switch( attr )
		{
			case SOUNDATTR_BufferFillState:
				{
					return( (APTR) ct->snddrv_check( ct->drvctx ) );
				}
				break;

			#ifdef DEBUG
			default:
				PDB(("unknown attr %ld\n", attr));
				break;
			#endif
		}
	}
	return( NULL );
}
