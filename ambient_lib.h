#ifndef AMBIENT_AMBIENT_LIB_H
#define AMBIENT_AMBIENT_LIB_H



ULONG ambientlib_init(void);
void ambientlib_cleanup(void);

ULONG preclose_ambientlib(void);

 
#define AMBIENT_LIB_VERSION 1
#define AMBIENT_LIB_REVISION 3

#endif /* AMBIENT_AMBIENT_LIB_H */
