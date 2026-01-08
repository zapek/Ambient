#ifndef AMBIENT_CX_H
#define AMBIENT_CX_H
/*
 * $Id: cx.h,v 1.5 2006/04/12 14:01:53 fab Exp $
 */

#include <libraries/commodities.h>

struct PrivateCxObj {
   struct Node     mco_Node;
   UBYTE           mco_Flags;
   UBYTE           mco_dummy1;
   struct MinList  mco_SubList;
   APTR            mco_dummy2;
   TEXT            mco_Name[24];
   TEXT            mco_Title[40];
   TEXT            mco_Descr[40];
   struct Task *   mco_Task;
   struct MsgPort *mco_Port;
   ULONG           mco_dummy3;
   WORD            mco_dummy4;
};


struct InputEvent;

#if 0
/* that one is the aros version.. */
struct PrivateCxMsg {
	struct Message    cxm_Message;
	APTR              *cxm_Routing;           /* Next destination */
	LONG              cxm_ID;
	UBYTE             cxm_Type;
	UBYTE             cxm_Level;
	APTR              *cxm_retObj[32];
	struct InputEvent *cxm_Data;
};
#else
struct PrivateCxMsg {
	struct Message    cxm_Message;
	APTR              *cxm_Routing;           /* Next destination */
	LONG              cxm_ID;
	UWORD             cxm_dummy; /* <- ? what is that.. */
	UBYTE             cxm_Type;
	UBYTE             cxm_Level;
	APTR              *cxm_retObj[32];
	struct InputEvent *cxm_Data;
};
#endif


extern struct MsgPort *cxport;
extern CxObj *exbroker;
extern ULONG cxsig;
extern ULONG exsig;

ULONG cx_init(void);
void cx_cleanup(void);
void cx_handle(void);

enum {
	CE_FAILED,
	CE_OK,
	CE_DUP,
};

ULONG exchange_create(void);
void exchange_delete(void);
void exchange_handle(void);

#endif /* AMBIENT_CX_H */
