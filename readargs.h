#ifndef AMBIENT_READARGS_H
#define AMBIENT_READARGS_H
/*
 * $Id: readargs.h,v 1.4 2006/08/08 13:31:36 fab Exp $
 */

#ifdef USE_INTERNAL_READARGS
struct RDArgs *readargs(CONST_STRPTR templ, LONG *array, struct RDArgs *rda);
void freeargs(struct RDArgs *args);
#else
#warning "original ReadArgs() gives problems with URI parsing"
#define readargs ReadArgs
#define freeargs FreeArgs
#endif

#endif /* AMBIENT_READARGS_H */
