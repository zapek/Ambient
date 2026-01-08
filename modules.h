#ifndef AMBIENT_MODULE_H
#define AMBIENT_MODULE_H
/*
 * $Id: modules.h,v 1.4 2006/08/08 13:31:35 fab Exp $
 */

struct Library *module_open(CONST_STRPTR name, ULONG version);
void module_close(struct Library *module);

ULONG modules_init(void);
void modules_cleanup(void);

#endif /* AMBIENT_MODULE_H */
