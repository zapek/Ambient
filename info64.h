#ifndef AMBIENT_INFO64_H
#define AMBIENT_INFO64_H
/*
 * $Id: info64.h,v 1.2 2007/11/11 17:56:58 piru Exp $
 */

#include <exec/types.h>

struct devinfo64 {
	LONG  di_NumSoftErrors;
	LONG  di_UnitNumber;
	LONG  di_DiskState;
	UQUAD di_NumBlocks;
	UQUAD di_NumBlocksUsed;
	ULONG di_BytesPerBlock;
	LONG  di_DiskType;
	BPTR  di_VolumeNode;
	LONG  di_InUse;
	LONG  di_DeviceType;
	ULONG di_Flags;
};

#define DIF_64BIT (1UL << 0)
#define DIF_CASE  (1UL << 1)

ULONG info64(CONST_STRPTR name, struct devinfo64 *fi);

#endif /* INFO64_H */
