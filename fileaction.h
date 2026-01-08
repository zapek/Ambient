#ifndef AMBIENT_FILEACTIONREQ_H
#define AMBIENT_FILEACTIONREQ_H
/*
 * $Id: fileaction.h,v 1.1 2006/09/18 23:17:28 fab Exp $
 */

ULONG fileaction_unprotect(CONST_STRPTR path, ULONG mode, ULONG *flags);
ULONG fileaction_replace(CONST_STRPTR from, CONST_STRPTR to, ULONG mode, ULONG *flags);

enum
{
	FILEACTION_CANCEL = 0,
	FILEACTION_SKIP,
	FILEACTION_PROCEED,
};

enum
{
	FILEACTION_UNPROTECT_FROM_DELETION = 0,
	FILEACTION_UNPROTECT_FROM_OVERWRITE,
};

enum
{
	FILEACTION_REPLACE_COPY = 0,
	FILEACTION_REPLACE_MOVE,
};

/* Unprotect files from deletion */
#define FILEACTION_UNPROTECT_DELETION_ALL       (1L<<0)
#define FILEACTION_UNPROTECT_DELETION_SKIP_ALL  (1L<<1)

/* Unprotect files from overwrite */
#define FILEACTION_UNPROTECT_OVERWRITE_ALL      (1L<<2)
#define FILEACTION_UNPROTECT_OVERWRITE_SKIP_ALL (1L<<3)

/* Replace files */
#define FILEACTION_REPLACE_ALL                  (1L<<4)
#define FILEACTION_REPLACE_SKIP_ALL             (1L<<5)

#define FILEACTION_REQUEST_CANCEL               (1L<<31)

#endif /* AMBIENT_FILEACTIONREQ_H */
