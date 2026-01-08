#ifndef AMBIENT_RECOG_H
#define AMBIENT_RECOG_H
/*
 * $Id: recog.h,v 1.8 2007/05/08 19:27:09 fab Exp $
 */

struct recognode {
	struct MinNode n;
	ULONG cmd;
	UBYTE data[0];
};

struct recog_ctx {
	struct MinList l;
	STRPTR pattern;     /* this is a pattern string (for MatchPattern function) which helps to prioritize recognition routines */
	STRPTR parsedpattern;
};


enum {
	RECOGCMD_NONE,
	RECOGCMD_OR,         /* joins commands, data is 0                                          */
	RECOGCMD_AND,        /* joins commands, data is 0                                          */
	RECOGCMD_BLOCKIN,    /* block delimiter, data is 0                                         */
	RECOGCMD_BLOCKOUT,   /* block delimited, data is 0                                         */
	RECOGCMD_MATCH,      /* matches against a pattern, see dopus 5.5, page 146                 */
	RECOGCMD_MATCHHUNK,  /* matches against a pattern, in amiga executable file hunks          */
	RECOGCMD_FILENAME,   /* matches against a filename pattern                                 */
	RECOGCMD_DATATYPES,  /* asks the datatypes subsystem                                       */
	RECOGCMD_MULTIMEDIA, /* asks the multimedia subsystem                                      */
	RECOGCMD_FINDSTRING, /* scans full file for string occurence at random position            */
	RECOGCMD_PROTECTION, /* matches protection bits                                            */
	RECOGCMD_DEVICE,     /* matches device type                                                */
	RECOGCMD_DIRECTORY,  /* matches directory type                                             */
	RECOGCMD_FILE,       /* matches directory type                                             */
	RECOGCMD_FILESIZE,   /* matches exact filesize or if filesize is dividable by fixed number */
	RECOGCMD_CONTENT,    /* guess if file is TEXT or BINARY                                    */
	RECOGCMD_COMMENT,    /* matches content of comment field                                   */
};


/*  recog_file() retvals 
 */
enum {
	RECOG_FALSE,
	RECOG_TRUE,
	RECOG_WRONGINPUT,    /* parse error or so */
	RECOG_OUTOFMEM,      /* out of memory     */
	RECOG_IOERROR,       /* I/O error         */
};

/*  recog_getfhattr() values
 */
enum {
	RECOGFHATTR_USERDATA = 1,
};


APTR  recog_open(CONST_STRPTR name);
void  recog_close(APTR fctx);
ULONG recog_prod(APTR fctx, APTR ctx, ULONG fileio);

APTR  recog_create(void);
void  recog_delete(APTR ctx);

ULONG recog_add(APTR ctx, ULONG cmd, CONST_STRPTR data);
APTR  recog_duplicate(APTR ctx);

void  recog_sethintpattern( APTR ctx, CONST_STRPTR pattern );
ULONG recog_checkhint( APTR ctx, CONST_STRPTR name );

ULONG recog_getfhattr(APTR fctx, ULONG val);

#endif /* AMBIENT_RECOG_H */
