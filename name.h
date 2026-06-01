#ifndef AMBIENT_NAME_H
#define AMBIENT_NAME_H
/*
 * $Id: name.h,v 1.12 2025/08/12 17:32:17 kronos Exp $
 */

ULONG name_isinfo(CONST_STRPTR name);
STRPTR name_build(CONST_STRPTR name);
STRPTR name_build_sysify(CONST_STRPTR name);
STRPTR name_replace(STRPTR oldname, CONST_STRPTR newname);
STRPTR name_build_info(CONST_STRPTR name);
STRPTR name_build_noinfo(CONST_STRPTR name);
STRPTR name_build_quoted(CONST_STRPTR name);
STRPTR name_build_unquoted(CONST_STRPTR name);
void name_delete(STRPTR name);
ULONG name_check_quoted(CONST_STRPTR name);
ULONG name_match(CONST_STRPTR name, CONST_STRPTR pattern);
STRPTR name_quotepattern(CONST_STRPTR path, STRPTR newpath, ULONG buflen);
STRPTR name_build_readargs_quoted(CONST_STRPTR name, STRPTR newname, ULONG buflen);
STRPTR name_build_colon(CONST_STRPTR name);
int name_build_wintitle(STRPTR wintitle, int length, STRPTR p);

APTR name_truncateinfo(STRPTR name);
void name_restoreinfo(STRPTR name, APTR truncation);

APTR name_truncateprefs(STRPTR name);
void name_restoreprefs(STRPTR name, APTR truncation);

#define NAME_ELLIPSIS_START  -1
#define NAME_ELLIPSIS_MIDDLE 0
#define NAME_ELLIPSIS_END    1

STRPTR name_shorten_ellipsis(CONST_STRPTR src, STRPTR dest, ULONG buflen, LONG style);

#endif /* AMBIENT_NAME_H */
