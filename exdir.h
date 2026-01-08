#ifndef AMBIENT_EXDIR_H
#define AMBIENT_EXDIR_H
/*
 * $Id: exdir.h,v 1.4 2006/08/08 13:31:34 fab Exp $
 */

struct dirinfo
{
	ULONG files;
	ULONG dirs;
	ULONG icons;
	ULONG links;
	QUAD  diskusage;
	QUAD  selecteddiskusage;
};

ULONG exdir(APTR obj, CONST_STRPTR path, CONST_STRPTR pattern, ULONG max_ed, LONG type, ULONG (*matchfunc)(APTR obj, CONST_STRPTR path, struct ExAllData *ead, APTR userdata), ULONG (*notmatchfunc)(APTR obj, CONST_STRPTR path, struct ExAllData *ead, APTR userdata), struct dirinfo *di, APTR userdata);

#endif /* AMBIENT_EXDIR_H */
