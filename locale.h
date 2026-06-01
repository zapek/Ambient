#ifndef AMBIENT_LOCALE_H
#define AMBIENT_LOCALE_H
/*
 * $Id: locale.h,v 1.9 2026/05/15 12:15:50 geit Exp $
 */

extern struct Locale *locale;

ULONG locale_init(void);
void  locale_cleanup(void);

/* functions to translate Recognition.db */
char *locale_translationgetenglish( char *str );
char *locale_translationget( char *english );
char *locale_translationfillpattern( char *oldstr, int oldstrlen );

/* date functons */
void CreateDateString(CONST_STRPTR template, struct DateStamp *ds, STRPTR buf);
BOOL ParseDateString(CONST_STRPTR template, struct DateStamp *ds, CONST_STRPTR datestr);

#endif /* AMBIENT_LOCALE_H */
