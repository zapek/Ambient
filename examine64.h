#ifndef AMBIENT_EXAMINE64_H
#define AMBIENT_EXAMINE64_H
/*
 * $Id: examine64.h,v 1.1 2006/08/08 13:19:51 fab Exp $
 */

#include <dos/dos.h>

struct fileinfo64 {
	LONG             fi_Type;
	UBYTE            fi_FileName[108];
	ULONG            fi_Protection;
	UQUAD            fi_Size;
	UQUAD            fi_NumBlocks;
	struct DateStamp fi_Date;
	UBYTE            fi_Comment[80];
	UWORD            fi_OwnerUID;
	UWORD            fi_OwnerGID;
};

ULONG examine64(CONST_STRPTR name, struct fileinfo64 *fi);
ULONG examinefh64(BPTR fh, struct fileinfo64 *fi);

#endif /* EXAMINE64_H */
