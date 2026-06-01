#ifndef AMBIENT_PANELLIB_H
#define AMBIENT_PANELLIB_H
/*
 * $Id: panellib.h,v 1.1 2026/01/25 17:39:38 kronos Exp $
 */
 
#define PANEL_LIB_VERSION 1
#define PANEL_LIB_REVISION 3
 
ULONG panellib_init(void);
void panellib_cleanup(void);

ULONG preclose_panellib(void);


#endif /* AMBIENT_PANELLIB_H */
