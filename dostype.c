/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006 Ambient Open Source Team
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
 * $Id: dostype.c,v 1.4 2006/08/08 13:31:33 fab Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "dostype.h"


/*
 * Partition type table.
 */
struct parttype {
	ULONG dostype;
	CONST_STRPTR name;
};

static const struct parttype pt[] = {
	/* BSD disklabel in MSDOS partitions */
	{FS_BSD_DISKLABEL_MSDOS_SWAP   , "BSD swap"},        /* "BSD\1" */
	{FS_BSD_DISKLABEL_MSDOS_V6     , "BSD version 6"},   /* "BSD\2" */
	{FS_BSD_DISKLABEL_MSDOS_V7     , "BSD version 7"},   /* "BSD\3" */
	{FS_BSD_DISKLABEL_MSDOS_SYSV   , "BSD System V"},    /* "BSD\4" */
	{FS_BSD_DISKLABEL_MSDOS_41     , "4.1 BSD"},         /* "BSD\5" */
	{FS_BSD_DISKLABEL_MSDOS_8TH    , "8th edition BSD"}, /* "BSD\6" */
	{FS_BSD_DISKLABEL_MSDOS_42     , "4.2 BSD"},         /* "BSD\7" */
	{FS_BSD_DISKLABEL_MSDOS_MSDOS  , "BSD MSDOS"},       /* "BSD\8" */
	{FS_BSD_DISKLABEL_MSDOS_44LFS  , "4.4 BSD LFS"},     /* "BSD\9" */
	{FS_BSD_DISKLABEL_MSDOS_UNKNOWN, "BSD unknown"},     /* "BSD\a" */
	{FS_BSD_DISKLABEL_MSDOS_HPFS   , "BSD OS/2 HPFS"},   /* "BSD\b" */
	{FS_BSD_DISKLABEL_MSDOS_ISO9660, "BSD ISO9660"},     /* "BSD\c" */
	{FS_BSD_DISKLABEL_MSDOS_BOOT   , "BSD boot"},        /* "BSD\d" */
	{FS_BSD_DISKLABEL_MSDOS_AFFS   , "BSD Amiga AFFS"},  /* "BSD\e" */
	{FS_BSD_DISKLABEL_MSDOS_HFS    , "BSD Apple HFS"},   /* "BSD\f" */

	/* OSF (alpha) */
	{FS_OFS_LINUX_SWAP  , "OSF BSD/Linux swap"},  /* "OFS\1" */
	{FS_OFS_BSD_V6      , "OSF BSD version 6"},   /* "OFS\2" */
	{FS_OFS_BSD_V7      , "OSF BSD version 7"},   /* "OFS\3" */
	{FS_OFS_BSD_SYSV    , "OSF BSD System V"},    /* "OFS\4" */
	{FS_OFS_BSD_41      , "OSF 4.1 BSD"},         /* "OFS\5" */
	{FS_OFS_BSD_8TH     , "OSF 8th edition BSD"}, /* "OFS\6" */
	{FS_OFS_BSD_42      , "OSF 4.2 BSD"},         /* "OFS\7" */
	{FS_OFS_LINUX_NATIVE, "OSF Linux native"},    /* "OFS\8" */
	{FS_OFS_BSD_44LFS   , "OSF 4.4 BSD LFS"},     /* "OFS\9" */
	{FS_OFS_UNKNOWN     , "OSF unknown"},         /* "OFS\a" */
	{FS_OFS_HPFS        , "OSF OS/2 HPFS"},       /* "OFS\b" */
	{FS_OFS_ISO9660     , "OSF ISO9660"},         /* "OFS\c" */
	{FS_OFS_BOOT        , "OSF boot"},            /* "OFS\d" */
	{FS_OFS_AFFS        , "OSF Amiga AFFS"},      /* "OFS\e" */
	{FS_OFS_HFS         , "OSF Apple HFS"},       /* "OFS\f" */
	{FS_OFS_ADVFS       , "OSF Digital AdvFS"},   /* "OFS\1\0" */

	/* SUN */
	{FS_SUN_EMPTY       , "Sun empty"},        /* "SUN\0" */
	{FS_SUN_BOOT        , "Sun boot"},         /* "SUN\1" */
	{FS_SUN_SUNOS_ROOT  , "SunOS root"},       /* "SUN\2" */
	{FS_SUN_SUNOS_SWAP  , "SunOS swap"},       /* "SUN\3" */
	{FS_SUN_SUNOS_USR   , "SunOS usr"},        /* "SUN\4" */
	{FS_SUN_WHOLE       , "Sun whole disk"},   /* "SUN\5" */
	{FS_SUN_SUNOS_STAND , "SunOS stand"},      /* "SUN\6" */
	{FS_SUN_SUNOS_VAR   , "SunOS var"},        /* "SUN\7" */
	{FS_SUN_SUNOS_HOME  , "SunOS home"},       /* "SUN\8" */
	{FS_SUN_LINUX_MINIX , "Sun Linux minix"},  /* "SUN\8\1" */
	{FS_SUN_LINUX_SWAP  , "Sun Linux swap"},   /* "SUN\8\2" */
	{FS_SUN_LINUX_NATIVE, "Sun Linux native"}, /* "SUN\8\3" */

	/* Amiga */
	{FS_AMIGA_GENERIC_BOOT  , "Amiga generic boot"},         /* "BOOU" */
	{FS_AMIGA_OFS           , "OFS"},                        /* "DOS\0" */
	{FS_AMIGA_FFS           , "FFS"},                        /* "DOS\1" */
	{FS_AMIGA_OFS_INTL      , "OFS Intl."},                  /* "DOS\2" */
	{FS_AMIGA_FFS_INTL      , "FFS Intl."},                  /* "DOS\3" */
	{FS_AMIGA_OFS_DC_INTL   , "OFS DC Intl."},               /* "DOS\4" */
	{FS_AMIGA_FFS_DC_INTL   , "FFS DC Intl."},               /* "DOS\5" */
	{FS_AMIGA_OFS_LNFS      , "OFS LNFS"},                   /* "DOS\6" */
	{FS_AMIGA_FFS_LNFS      , "FFS LNFS"},                   /* "DOS\7" */
	{FS_AMIGA_MUFS_FFS_INTL , "muFS FFS Intl."},             /* "muFS" */
	{FS_AMIGA_MUFS_OFS      , "muFS OFS"},                   /* "muF\0" */
	{FS_AMIGA_MUFS_FFS      , "muFS FFS"},                   /* "muF\1" */
	{FS_AMIGA_MUFS_OFS_INTL , "muFS OFS Intl"},              /* "muF\2" */
	{FS_AMIGA_MUFS_FFS_INTL2, "muFS FFS Intl."},             /* "muF\3", same as muFS */
	{FS_AMIGA_MUFS_OFS_DC   , "muFS OFS DC"},                /* "muF\4" */
	{FS_AMIGA_MUFS_FFS_DC   , "muFS FFS DC"},                /* "muF\5" */
	{FS_AMIGA_LINUX_NATIVE  , "Amiga Linux native"},         /* "LNX\0" */
	{FS_AMIGA_LINUX_EXT2    , "Amiga Linux ext2"},           /* "EXT2" */
	{FS_AMIGA_LINUX_SWAP    , "Amiga Linux swap"},           /* "SWAP" */
	{FS_AMIGA_LINUX_SWAP2   , "Amiga Linux swap"},           /* "SWP\0", same as SWAP */
	{FS_AMIGA_LINUX_MINIX   , "Amiga Linux minix"},          /* "MNX\0" */
	{FS_AMIGA_AMIX_0        , "Amix 0"},                     /* "UNI\0" */
	{FS_AMIGA_AMIX_1        , "Amix 1"},                     /* "UNI\1" */
	{FS_AMIGA_NETBSD_ROOT   , "Amiga NetBSD root"},          /* "NBR\7" */
	{FS_AMIGA_NETBSD_SWAP   , "Amiga NetBSD swap"},          /* "NBS\1" */
	{FS_AMIGA_NETBSD_OTHER  , "Amiga NetBSD other"},         /* "NBU\7" */
	{FS_AMIGA_PFS0          , "PFS 0"},                      /* "PFS\0" */ /* XXX: not sure about the PFS ones.. ask.. */
	{FS_AMIGA_PFS1          , "PFS 1"},                      /* "PFS\1" */ /* XXX: not sure about the PFS ones.. ask.. */
	{FS_AMIGA_PFS2          , "PFS 2"},                      /* "PFS\2" */
	{FS_AMIGA_PDS2          , "PFS 2 SCSIdirect"},           /* "PDS\0" */
	{FS_AMIGA_PFS3          , "PFS 3"},                      /* "PFS\3" */
	{FS_AMIGA_PDS3          , "PFS 3 SCSIdirect"},           /* "PDS\3" */
	{FS_AMIGA_MUPFS         , "PFS Multiuser"},              /* "muPF"  */
	{FS_AMIGA_AFS           , "AFS"},                        /* "AFS\0" */
	{FS_AMIGA_AFS_EXP       , "AFS (experimental)"},         /* "AFS\1" */
	{FS_AMIGA_CDISO         , "CDROM ISO"},                  /* "CD01" can be AmiCDFS, CBM's FS or CDrive in ISO mode */
	{FS_AMIGA_CDHSF         , "CDROM HSF"},                  /* "CD00" HighSierra */
	{FS_AMIGA_CDDA          , "CDROM CDDA"},                 /* "CDDA" audio CD */
	{FS_AMIGA_CDRIVE        , "CDrive or AmiCDFS"},          /* "CDFS" used by AmiCDFS and CDrive, CDrive changes the dostype to ISO, HighSierra or CDDA then */
	{FS_AMIGA_ASIMCDFS      , "AsimCDFS"},                   /* <meaningless> */
	{FS_AMIGA_HFS           , "Macintosh HFS"},              /* "MAC\0" */
	{FS_AMIGA_MSDOS         , "MSDOS disk"},                 /* "MSD\0" */
	{FS_AMIGA_MSDOS_HF      , "MSDOS PC-Task hardfile"},     /* "MSH\0" */
	{FS_AMIGA_BFFS          , "BFFS"},                       /* "BFFS" */
	{FS_AMIGA_SFS           , "SFS"},                        /* "SFS\0 */

	/* Those are special amiga stuff */
	{FS_AMIGA_BAD , "Unreadable disk"},            /* "BAD\0" */
	{FS_AMIGA_NDOS, "Not really dos"},             /* "NDOS" */
	{FS_AMIGA_KICK, "Kickstart disk"},             /* "KICK" */

	/* Atari */
	{FS_ATARI_GEMDOS      , "Atari GEMDOS (<32MB)"}, /* "AGEM" */
	{FS_ATARI_GEMDOSBIG   , "Atari GEMDOS (>32MB)"}, /* "ABGM" */
	{FS_ATARI_LINUX       , "Atari Linux"},          /* LNX */
	{FS_ATARI_LINUX_SWAP  , "Atari Linux swap"},     /* SWP */
	{FS_ATARI_LINUX_MINIX , "Atari Linux minix"},    /* MIX */
	{FS_ATARI_LINUX_MINIX2, "Atari Linux minix"},    /* MNX */
	{FS_ATARI_HFS         , "Atari HFS"},            /* MAC */
	{FS_ATARI_SYSV_UNIX   , "Atari SysV unix"},      /* UNX */
	{FS_ATARI_RAW         , "Atari raw"},            /* RAW */
	{FS_ATARI_EXTENDED    , "Atari extended"},       /* XGM */

	/* Macintosh */
	{FS_MAC_PARTITION_MAP   , "Mac partition map"},      /* "MAC\0" */
	{FS_MAC_MACOS_DRIVER    , "MacOS driver"},           /* "MAC\1" */
	{FS_MAC_MACOS_DRIVER_43 , "MacOS driver 4.3"},       /* "MAC\2" */
	{FS_MAC_MACOS_HFS       , "MacOS HFS"},              /* "MAC\4" */
	{FS_MAC_MACOS_MFS       , "MacOS MFS"},              /* "MAC\5" */
	{FS_MAC_SCRATCH         , "Mac scratch"},            /* "MAC\6" */
	{FS_MAC_PRODOS          , "Mac ProDOS"},             /* "MAC\7" */
	{FS_MAC_FREE            , "Mac free"},               /* "MAC\8" */
	{FS_MAC_LINUX_SWAP      , "Mac Linux swap"},         /* "MAC\9" */
	{FS_MAC_AUX             , "Mac A/UX"},               /* "MAC\a" */
	{FS_MAC_MSDOS           , "Mac MSDOS"},              /* "MAC\b" */
	{FS_MAC_MINIX           , "Mac minix"},              /* "MAC\c" */
	{FS_MAC_AFFS            , "Mac Amiga AFFS"},         /* "MAC\d" */
	{FS_MAC_LINUX_NATIVE    , "Mac Linux native"},       /* "MAC\e" */
	{FS_MAC_NEWWORLD        , "Mac NewWorld bootblock"}, /* "MAC\f" */
	{FS_MAC_MACOS_ATA       , "MacOS ATA driver"},       /* "MAC\1\0" */
	{FS_MAC_MACOS_FW_DRIVER , "MacOS FW driver"},        /* "MAC\1\1" */
	{FS_MAC_MACOS_IOKIT     , "MacOS IOKit"},            /* "MAC\1\2" */
	{FS_MAC_MACOS_PATCHES   , "MacOS patches"},          /* "MAC\1\3" */
	{FS_MAC_MACOSX_BOOT     , "MacOSX bootloader"},      /* "MAC\1\4" */
	{FS_MAC_MACOSX_LOADER   , "MacOSX loader"},          /* "MAC\1\5" */
	{FS_MAC_UFS             , "Mac UFS"},                /* "MAC\1\6" */

	/* Acorn */
	{FS_ACORN_ADFS          , "Acorn ADFS"},                  /* "ARM\1" */
	{FS_ACORN_LINUX_MAP     , "Acorn Linux partitionmap"},    /* "ARM\2" */
	{FS_ACORN_LINUX_EXT2    , "Acorn Linux ext2"},            /* "ARM\3" */
	{FS_ACORN_LINUX_SWAP    , "Acorn Linux swap"},            /* "ARM\4" */
	{FS_ACORN_ADFS_ICS      , "Acorn ADFS (ICS/APDL)"},       /* "ARM\5" */
	{FS_ACORN_LINUX_EXT2_ICS, "Acorn Linux ext2 (ICS/APDL)"}, /* "ARM\6" */
	{FS_ACORN_LINUX_SWAP_ICS, "Acorn Linux swap (ICS/APDL)"}, /* "ARM\7" */

	/* Sinclair QL */
	{FS_SINCLAIR_QL5A, "Sinclair QL 720k"},  /* "QL5A" */
	{FS_SINCLAIR_QL5B, "Sinclair QL 1440k"}, /* "QL5B" */

	/* Spectrum */
	{FS_SPECTRUM_DISCIPLE, "Spectrum Disciple"},   /* "ZXS\0" */
	{FS_SPECTRUM_UNIDOS  , "Spectrum UniDos"},     /* "ZXS\1" */
	{FS_SPECTRUM_SAMDOS  , "Spectrum SamDos"},     /* "ZXS\2" */
	{FS_SPECTRUM_OPUS    , "Spectrum Opus (180k)"}, /* "ZXS\3" */

	/* Archimedes */
	{FS_ARCHIMEDES_D, "Archimedes (D)"},  /* "ARMD" */
	{FS_ARCHIMEDES_E, "Archimedes (E)"}, /* "ARME" */

	/* CP/M */
	{FS_CPM, "CP/M"}, /* "CPM\2" */

	/* C64 */
	{FS_C64, "C64"}, /* "1541" */

	{NULL, NULL},
};


CONST_STRPTR dostype_get(ULONG id)
{
	ULONG i = 0;

	while (pt[i].dostype)
	{
		if (pt[i].dostype == id)
		{
			return (pt[i].name);
		}
		i++;
	}
	return (NULL);
}
