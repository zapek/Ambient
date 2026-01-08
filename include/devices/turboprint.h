#ifndef DEVICES_TURBOPRINT_H
#define DEVICES_TURBOPRINT_H

#ifndef DEVICES_PRINTER_H
#include <devices/printer.h>
#endif

#define TPFMT_BitPlanes    0x00
#define TPFMT_Chunky8      0x01
#define TPFMT_RGB15        0x12
#define TPFMT_RGB16        0x13
#define TPFMT_RGB24        0x14
#define TPFMT_BGR15        0x02
#define TPFMT_BGR16        0x03
#define TPFMT_BGR24        0x04

#define TPFMT_HAM          0x800
#define TPFMT_EHB          0x080
#define TPFMT_CyberGraphX  0x400

#define TPMATCHWORD 0xf10a57ef

#define PRD_TPEXTDUMPRPORT (PRD_DUMPRPORT | 0x80)

#pragma pack(2)
struct TPExtIODRP
{
	UWORD PixAspX;
	UWORD PixAspY;
	UWORD Mode;
	ULONG Reserved[9];
};
#pragma pack()

#endif /* DEVICES_TURBOPRINT_H */
