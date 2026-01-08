#ifndef AMBIENT_REXX_H
#define AMBIENT_REXX_H
/*
 * $Id: rexx.h,v 1.4 2006/08/08 13:31:36 fab Exp $
 */

struct TagItem;

/*
 * Name of the ARexx port and extentions
 */
#define REXXPORT "AMBIENT"
#define REXXEXT ".ARX"

/*
 * Maximum length of an ARexx command including the arguments
 */
#define REXX_MAXLENGTH 2048 /* should be enough for everyone (tm) */

/*
 * Ambient ARexx errors
 *
 * - success: 0 (RC_OK)
 * - specific command warnings: 5 to 9
 * - specific command errors: 10 to 19
 * - specific command fatals: 20 to 30 or so
 * - general warnings use negative values similar to MUI, like below:
 */
#define AMBIENT_RXERR_BADDEFINITION  -1L
#define AMBIENT_RXERR_OUTOFMEMORY    -2L
#define AMBIENT_RXERR_UNKNOWNCOMMAND -3L
#define AMBIENT_RXERR_BADSYNTAX      -4L
#define AMBIENT_RXERR_NYI            -5L /* blame me for that :) */

#if USE_REXX
extern ULONG rexxsig;

ULONG rexx_init(void);
void rexx_cleanup(void);

void rexx_handle(void);

void rx_set_result(ULONG id, LONG val, CONST_STRPTR str);
void rx_reply_id(ULONG id);
void rx_setrexxvar_id(ULONG id, CONST_STRPTR name, CONST_STRPTR val);
#endif

struct RDArgs *readargsstring(CONST_STRPTR source, CONST_STRPTR templ, ULONG *array);
void freeargsstring(struct RDArgs *rda);

//int match_command(int type, STRPTR c, STRPTR s); /* XXX: add back I think.. */
void execute_command(APTR obj, ULONG type, CONST_STRPTR str, APTR *array);
void execute_command_objarray(APTR obj, ULONG type, CONST_STRPTR s);

#endif /* AMBIENT_REXX_H */
