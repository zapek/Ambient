#ifndef AMBIENT_LOCALE_H
#define AMBIENT_LOCALE_H
/*
 * $Id: locale.h,v 1.7 2016/03/19 16:31:13 itix Exp $
 */

extern struct Locale *locale;

ULONG locale_init(void);
void  locale_cleanup(void);

void CreateDateString(CONST_STRPTR template, struct DateStamp *ds, STRPTR buf);
BOOL ParseDateString(CONST_STRPTR template, struct DateStamp *ds, CONST_STRPTR datestr);

#endif /* AMBIENT_LOCALE_H */
