#ifndef AMBIENT_DEVICE_FUNC_H
#define AMBIENT_DEVICE_FUNC_H
/*
 * $Id: device_func.h,v 1.4 2023/01/11 15:14:30 jacadcaps Exp $
 */

struct device_info {
	STRPTR name;          /* DH0:, etc.. */
	STRPTR devname;       /* device name, this and later fields only filled if fssmLookup is TRUE */
	ULONG unit;
	ULONG flags;          /* flags to OpenDevice() with */
	ULONG lowcyl;         /* starting cylinder of the drive, usually 0 */
	ULONG highcyl;        /* highest cylinder */
	ULONG surfaces;       /* number of heads, drive specific */
	ULONG blocksize;      /* in bytes */
	ULONG blockspertrack; /* blocks per track, drive specific */
	ULONG bufmemtype;     /* type of memory buffer */
	ULONG dostype;        /* dostype */
};


struct device_info * deviceinfo_build(STRPTR volumename, BOOL fssmLookup);
void deviceinfo_delete(struct device_info *di);
ULONG isndos(STRPTR devname);
ULONG isfssm(APTR fssm);

#endif /* AMBIENT_DEVICE_FUNC_H */
