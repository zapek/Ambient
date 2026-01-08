#include "ambient.h"

/* public */
#include <exec/system.h>
#include <exec/resident.h>
#include <proto/openurl.h>
#include <proto/exec.h>

/* private */
#include "ambient_cat.h"
#include "methodstack.h"
#include "mui_func.h"
#include "copyright.h"
#include "screen.h"
#include "crypto.h"
#include "smartreq.h"
#include "modules.h"
#include "modules/about/libraries/about.h"
#include "modules/about/ppcinline/about.h"
#include "ambient_altivec.h"
#include "input.h"

struct Data
{
	ULONG mimetype;
	ULONG resolved;
};

DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_Mimetype_Type:
			*msg->opg_Storage = data->mimetype;
			return (TRUE);

		case MA_Mimetype_TypeResolved:
			*msg->opg_Storage = (ULONG) data->resolved;
			return (TRUE);
	}

	return (DOSUPER);
}

DEFSET
{
	GETDATA;

	FORTAG(INITTAGS)
	{
		case MA_Mimetype_Type:
			data->mimetype = tag->ti_Data;
			break;
		
		case MA_Mimetype_TypeResolved:
			data->resolved = tag->ti_Data ? TRUE : FALSE;
			break;
	}
	NEXTTAG

	return (DOSUPER);
}

BEGINMTABLE
DECSET
DECGET
ENDMTABLE

DECSUBCLASS_NC(MUIC_Notify, mimetypeclass)
