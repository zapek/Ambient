#ifndef AMBIENT_EJECT_H
#define AMBIENT_EJECT_H

#include <exec/types.h>


/*
** eject modes available
*/
enum
{
	EJECT_LOAD,
	EJECT_EJECT,
	EJECT_TOGGLE
};

#define EJECT_JIFFIES 10    /* tolerance for tray state detection */

/* Some standard scsi definitions */

#define SSUB_ON      0
#define SSUF_ON      (1L<<SSUB_ON)
#define SSUB_EJECT   1
#define SSUF_EJECT   (1L<<SSUB_EJECT)

#define SENSE_SIZEOF 252

#define SCSI_CMD_SSU                 0x1B   /* 6B: Start/Stop Unit */


/* Prototypes */

void eject( STRPTR devname, ULONG unit, ULONG flags, ULONG mode );
void unmount( STRPTR devname );

#endif /* AMBIENT_EJECT_H */


