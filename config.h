#ifndef AMBIENT_CONFIG_H
#define AMBIENT_CONFIG_H
/*
 * $Id: config.h,v 1.44 2026/05/14 22:40:54 geit Exp $
 */

/*  comment out to compile on 1.5 systems. Do *NOT* do
 *  other fancy stuff here and keep the '1' (it's used in the
 *  makefile)
 */
//#define USE_LEGACY 1

#ifndef BUILD_ICONLIB

#ifdef BETA_RELEASE
#define DEBUG 1
//#undef DEBUG
#endif

/*
 * Do *NOT* comment out, indent or
 * do fancy stuff here. Just set the
 * number to '1' or '0' and nothing
 * else. You've been warned.
 */
#define USE_64BIT_WRITES         1
#if USE_LEGACY
#define USE_ALTIVEC              0
#else
#define USE_ALTIVEC              1
#endif
#define USE_ASYNC_IO             1
#define USE_ASYNC_PUSHMETHOD     1
#define USE_CPU_CACHEHINTS       1
#define USE_CRAWLER              1
#define USE_DATATYPES_SOUND      1
#define USE_DEFICONS             1
#define USE_DOSNOTIFY            1
#define USE_GLOWICONS            1
#define USE_GLOWICONS32          1
#define USE_DBUF                 1
#define USE_DOTBACKDROP          0
#define USE_DTICONS              0
#define USE_DTFRIENDFORMAT       0
#define USE_FPU                  1
#define USE_IFF_IO               0
#define USE_INLINE_NEXTOBJECT    1
#define USE_INLINE_NEXTTAGITEM   1
#define USE_INTERNAL_READARGS    1
#define USE_INTERNAL_SCALER      1
#define USE_VECTOR_SCALER        1
#define USE_INTUITON_TIMEOUT     1
#define USE_ISFILESYSTEM_TIMEOUT 1
#if USE_LEGACY
#define USE_LOGOFLASH            0
#else
#define USE_LOGOFLASH            1
#endif
#define USE_THREADPOOL           0
#define USE_MPEGA                1
#define USE_MUICLASSES_NAMEHACK  1
#define USE_STUNTZIHACK          1
#if USE_LEGACY
#define USE_MULTIMEDIA           0
#else
#define USE_MULTIMEDIA           1
#endif
#define USE_NEWICONS             1
#define USE_OS4                  1
#define USE_PNGICONS             1
#define USE_SVGICONS             1
#if USE_LEGACY
#define USE_POINTERS             0
#else
#define USE_POINTERS             1
#endif
#define USE_POINTER_HOVERING     0
#define USE_PREFSCRC32           1
#if USE_LEGACY
#define USE_RANDOM_LIB           0
#else
#define USE_RANDOM_LIB           1
#endif
#define USE_REXX                 1
#define USE_SAFE_CONFIG          1
#define USE_SCREENBUFFER         0
#if USE_LEGACY
#define USE_SHARED_LIBPNG        0
#define USE_SHARED_LIBZ          0
#else
#define USE_SHARED_LIBPNG        1
#define USE_SHARED_LIBZ          1
#endif
#define USE_SHORTCUTS            1
#define USE_SOLIDDRAG            0
#define USE_SOUND                1
#define USE_SOUNDLIB             1
#if USE_LEGACY
#define USE_STRONG_RANDOM        0
#else
#define USE_STRONG_RANDOM        1
#endif
#define USE_THUMBS               1
#define USE_VIEW_IMAGE           1
#define USE_VIEW_TEXT            1
#define USE_VIEW_HEX             1
#define USE_VORBIS               1
#define USE_WBSTARTLIB           1
#define USE_ICON_SCALING         1
#define USE_DROP_EFFECT_PREFS    0
#define USE_LESS_MEMORY          1
#define USE_SILLY_ICONPREFS      0
#define USE_EXTRA_LISTERPREFS    0
#define USE_INSTANTAPPLY_PREFS   1
#if USE_LEGACY
#define USE_AVCODEC              0
#else
#define USE_AVCODEC              1
#endif
#define USE_AMBIENT_LIB          1
#define ENABLE_METHODSTACK_PUSHSYNCSAFE	1
#define USE_INTERNAL_PANELS      1
#define USE_EXTERNAL_PANELS      1
/*
 * Debugging stuff
 */
#ifdef DEBUG
#define USE_INTERNAL_DOMETHOD 0 /* do NOT enable this. some stuff (eg. datatypes) currently use it */
#define USE_MEMTRACK          1
#define USE_MEMTRACK_MEMLIST  1
#define USE_MEMTRACK_RECORD   1
#endif

#endif /* !BUILD_ICONLIB */

#define USE_RECOGTRANSLATION     1

/*
 * Constants
 */
#define PATH_SIZE 256          /* maximum path size (including filename + NULL) */
#define NAME_SIZE 108          /* maximum filename size (+ NULL) */
#define VOLUME_SIZE 31         /* maximum volume size (+ NULL) */
#define M68K_STACKSIZE 4096    /* minimum 68k stacksize */
#define STACKSIZE 32768        /* stacksize of the main task */
#define STACKSIZE_THREAD 16384 /* stacksize of the threads */
#define STATUSBARTEXTSIZE 1024 /* max lenght of the text in statusbar */
#define MINIMUM_TRANSITIONS_VMEM (96 * 1024 * 1024) /* do NOT set this to 64 or lower, just don't */
/*
 * I/O buffers for copy
 */
#define COPYIO_BUFFERSIZE (8* 1024 * 1024) /* bytes */
#define COPYIO_READAHEAD_BUFFERSIZE (16 * 512 * 1024) /* bytes */
#define COPYIO_USE_ASYNCIO 1


/*
 * Animation speed in miliseconds (panels)
 */
#define ANIM_REFRESH_MS 20 /* 50 hz */

/*
 * Progress update refresh speed (gauges, etc..)
 */
#define PROGRESS_REFRESH_MS 40 /* 25 hz */

/*
 * Accuracy of graphs. (CPU history, etc..)
 */
#define GRAPHCLASS_ACCURACY 256

/*
 * Features
 */

/*
 * Uses the float unit to output a 64-bit
 * "burst". Works best with memory set in
 * non coherency modes. Always works on
 * PCI memory areas. Doesn't seems worth it.
 */
#if !USE_64BIT_WRITES
#else
#if !USE_FPU
#error "USE_64BIT_WRITES requires USE_FPU"
#endif
#endif

/*
 * Use the altivec engine if present.
 */
#if !USE_ALTIVEC
#undef USE_ALTIVEC
#endif

/*
 * Enable the use of asynchronous I/O
 * (requires asyncio.library)
 */
#if !USE_ASYNC_IO
#undef USE_ASYNC_IO
#endif

/*
 * Enable the use of asynchronous pushmethods.
 * If not set, all the methods are synchronous.
 * It's better to not set it for debugging otherwise
 * set it to gain speed.
 */
#if !USE_ASYNC_PUSHMETHOD
#undef USE_ASYNC_PUSHMETHOD
#endif

/*
 * Enables CPU cache hints.
 */
#if !USE_CPU_CACHEHINTS
#undef USE_CPU_CACHEHINTS
#endif

/*
 * Enables custom crawling scrolling in the
 * About window.
 */
#if !USE_CRAWLER
#undef USE_CRAWLER
#endif

/*
 * Enables datatypes to play sound.
 */
#if !USE_DATATYPES_SOUND
#undef USE_DATATYPES_SOUND
#endif

/*
 * Enable to use deficons emulation.
 */
#if !USE_DEFICONS
#undef USE_DEFICONS
#endif

/*
 * Enables DOS notifications.
 */
#if !USE_DOSNOTIFY
#undef USE_DOSNOTIFY
#endif

/*
 * Tries to use double buffering
 * where possible.
 */
#if !USE_DBUF
#undef USE_DBUF
#endif

/*
 * Enable Glowicons support.
 */
#if !USE_GLOWICONS
#undef USE_GLOWICONS
#endif

#if !USE_GLOWICONS32 || !USE_GLOWICONS
#undef USE_GLOWICONS32
#endif

/*
 * Enables .backdrop support.
 */
#if !USE_DOTBACKDROP
#undef USE_DOTBACKDROP
#endif

/*
 * Fallsback to datatypes to read
 * icons. Basically allows any image
 * to be loaded as icon as long as there's
 * a datatype for it.
 */
#if !USE_DTICONS
#undef USE_DTICONS
#endif

/*
 * Tries to trick picture.datatype to allocate
 * the format we need by passing it a small
 * bitmap of the desired format as friend. If it
 * doesn't work we fallback to the previous
 * method anyway. Beware, it appears picture.datatype
 * sucks so in case of failure it would just free
 * that bitmap itself. Moreover, picture.datatype
 * internally remaps by allocating another bitmap,
 * which is exactly what we do otherwise. Don't
 * enable that define.
 */
#if !USE_DTFRIENDFORMAT
#undef USE_DTFRIENDFORMAT
#endif

/*
 * Allows computations using the FPU.
 */
#if !USE_FPU
#undef USE_FPU
#endif

/*
 * Enables IFF read/write routines
 * support. Not always needed.
 */
#if !USE_IFF_IO
#undef USE_IFF_IO
#endif

/*
 * Inlines the NextObject() calls for
 * greater speed.
 */
#if !USE_INLINE_NEXTOBJECT
#undef USE_INLINE_NEXTOBJECT
#endif

/*
 * Inlines NextTagItem() within the FORTAG/NEXTTAG
 * taglist walking macros for greater speed. Needs
 * to be changed/removed if new tag types are
 * added.
 */
#if !USE_INLINE_NEXTTAGITEM
#undef USE_INLINE_NEXTOBJECT
#endif

/*
 * Uses an internal ReadArgs(). The original
 * one barfs when an URI (LoadURI) contains
 * some '=' in it.
 */
#if !USE_INTERNAL_READARGS
#undef USE_INTERNAL_READARGS
#endif

/*
 * Tries to use the internal scaling
 * routines whenever possible (ARGB32).
 */
#if !USE_INTERNAL_SCALER
#undef USE_INTERNAL_SCALER
#endif

/*
 * Tries to detect when intuition is waiting
 * for us to release a lock on layers. If we don't
 * do that the system gets a total lockup anyway.
 */
#if !USE_INTUITON_TIMEOUT
#undef USE_INTUITON_TIMEOUT
#endif

/*
 * Tries to timeout IsFileSystem()
 * if a buggy handler locked.
 */
#if !USE_ISFILESYSTEM_TIMEOUT
#undef USE_ISFILESYSTEM_TIMEOUT
#endif

/*
 * Enables flashing logos in the about
 * window with alpha fading with the
 * background.
 */
#if !USE_LOGOFLASH
#undef USE_LOGOFLASH
#endif

/*
 * Applies a lowpass filtering when
 * resizing icons at less than 0.5x the
 * original size (NYI).
 */
#if !USE_LOWPASS_FILTER
#undef USE_LOWPASS_FILTER
#else
#if !USE_FPU
#error "USE_LOWPASS_FILTER requires USE_FPU"
#endif
#endif

/*
 * Enables internal DoMethod() with
 * consistency checks. Recommended during
 * debugging. Not fully working yet.
 */
#if USE_INTERNAL_DOMETHOD
#ifndef DEBUG
#error "USE_INTERNAL_DOMETHOD needs DEBUG"
#endif
#else
#undef USE_INTERNAL_DOMETHOD
#endif

/*
 * Adds a way to dump states of memory
 * allocations.
 */
#if USE_MEMTRACK_MEMLIST
#if !USE_MEMTRACK
#error "USE_MEMTRACK_MEMLIST needs USE_MEMTRACK"
#endif
#else
#undef USE_MEMTRACK
#endif

/*
 * Allows to record allocations from one point
 * then dump what was allocated at another
 * point.
 */
#if USE_MEMTRACK_RECORD
#if !USE_MEMTRACK
#error "USE_MEMTRACK_RECORD needs USE_MEMTRACK"
#endif
#else
#undef USE_MEMTRACK_RECORD
#endif

/*
 * Enables internal memory allocation tracking
 * and debugging.
 */
#if USE_MEMTRACK
#ifndef DEBUG
#error "USE_MEMTRACK needs DEBUG"
#endif
#else
#undef USE_MEMTRACK
#endif

/*
 * Uses mpega.library.
 */
#if !USE_MPEGA
#undef USE_MPEGA
#endif

/*
 * Saves a bit of memory by using common strings for
 * class names.
 */
#if !USE_MUICLASSES_NAMEHACK
#undef USE_MUICLASSES_NAMEHACK
#endif

/*
 * Uses Krashan's multimedia class.
 */
#if USE_MULTIMEDIA
#ifndef USE_FPU
#error USE_MULTIMEDIA needs USE_FPU
#endif
#else
#undef USE_MULTIMEDIA
#endif

/*
 * Enables hobby support.
 */
#if !USE_OS4
#undef USE_OS4
#endif

/*
 * Enables Newicons support.
 */
#if !USE_NEWICONS
#undef USE_NEWICONS
#endif

/*
 * Enables PNG icon support.
 */
#if !USE_PNGICONS
#undef USE_PNGICONS
#endif

/*
 * Enables the changing of the mouse pointer
 * depending on what it points to, current
 * action, etc..
 */
#if !USE_POINTERS
#undef USE_POINTERS
#endif

/*
 * Changes the pointer when hovering
 * over icons.
 */
#if USE_POINTER_HOVERING
#if !USE_POINTERS && !defined(BUILD_ICONLIB)
#error "you need USE_POINTERS for USE_POINTER_HOVERING"
#endif
#undef USE_POINTER_HOVERING
#endif

/*
 * Adds a crc32 checksum to
 * preference files.
 */
#if !USE_PREFSCRC32
#undef USE_PREFSCRC32
#endif

/*
 * Uses random.library
 */
#if !USE_RANDOM_LIB
#undef USE_RANDOM_LIB
#else
#ifndef USE_STRONG_RANDOM
#error USE_RANDOM_LIB needs USE_STRONG_RANDOM
#endif
#endif

/*
 * Enables the ARexx interface.
 */
#if !USE_REXX
#undef USE_REXX
#endif

/*
 * When saving config files, rename the previous
 * file as <filename>.bak then write <filename>.
 */
#if !USE_SAFE_CONFIG
#define USE_SAFE_CONFIG
#endif

/*
 * Copies the whole screen to a buffer in main
 * memory for faster reading. Unfortunately the
 * first copy is slow and would have to be done
 * by a thread then used once it's available as
 * the current way gives a 0.5 second penality
 * to the user. Once copied, it's not much faster
 * than without the buffer so alpha functions
 * are the current bottleneck.
 */
#if !USE_SCREENBUFFER
#undef USE_SCREENBUFFER
#endif

/*
 * Uses png.library
 */
#if !USE_SHARED_LIBPNG
#undef USE_SHARED_LIBPNG
#endif

/*
 * Uses z.library
 */
#if !USE_SHARED_LIBZ
#undef USE_SHARED_LIBZ
#endif

/*
 * Enables icon shortcuts support.
 */
#if !USE_SHORTCUTS
#undef USE_SHORTCUTS
#endif

/*
 * Enable solid and checkered dragging modes. Mostly
 * useful on very low end configs, so low end that
 * this define doesn't work properly so don't
 * enable it.
 */
#if !USE_SOLIDDRAG
#undef USE_SOLIDDRAG
#endif

/*
 * Enables the sound system.
 */
#if !USE_SOUND
#undef USE_SOUND
#endif

/*
 * Enables ambient_sound.library API.
 */
#if !USE_SOUNDLIB
#undef USE_SOUNDLIB
#endif

/*
 * Uses cryptographically strong random number
 * generator. Based on Tatu Ylonen's code.
 */
#if !USE_STRONG_RANDOM
#undef USE_STRONG_RANDOM
#endif

/*
 * stuntzi requested this so that he can flush
 * the MUI libs. Always disabled for release
 * builds. Ambient will just leak the memory
 * used for the wb start port and exit without
 * warnings.
 */
#if !USE_STUNTZIHACK
#undef USE_STUNTZIHACK
#else
#ifndef DEBUG
#undef USE_STUNTZIHACK
#endif
#endif

/*
 * Adds thumbnails support.
 */
#if !USE_THUMBS
#undef USE_THUMBS
#endif

/*
 * Enables imageview.
 */
#if !USE_VIEW_IMAGE
#undef USE_VIEW_IMAGE
#endif

/*
 * Enables textview.
 */
#if !USE_VIEW_TEXT
#undef USE_VIEW_TEXT
#endif

#if !USE_VIEW_HEX
#undef USE_VIEW_HEX
#endif

/*
 * Enables Ogg Vorbis decoding by
 * using vorbisfile.library.
 */
#if !USE_VORBIS
#undef USE_VORBIS
#endif

/*
 * Creates a wbstart.library.
 */
#if !USE_WBSTARTLIB
#undef USE_WBSTARTLIB
#endif

/*
 * support for avcodec.library to retrieve
 * thumbnails for movie files 
 */
#if !USE_AVCODEC
#undef USE_AVCODEC
#else
#ifndef USE_THUMBS
#error USE_AVCODEC needs USE_THUMBS
#endif
#endif

#endif /* AMBIENT_CONFIG_H */
