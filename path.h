#ifndef AMBIENT_PATH_H
#define AMBIENT_PATH_H
/*
 * $Id: path.h,v 1.5 2006/08/08 13:31:35 fab Exp $
 */

struct ipc_newpath;
struct ipc_clonepath;

STRPTR path_build(CONST_STRPTR filename);
void path_free(STRPTR filename);
void path_update(struct ipc_newpath *np);
void path_clone(struct ipc_clonepath *cp);

#endif /* AMBIENT_PATH_H */
