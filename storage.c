#include "ambient.h"

/* public */
#include <proto/exec.h>
#include <proto/iffparse.h>
#include <proto/dos.h>

/* private */
#include "mui_func.h"
#include "storage.h"


static APTR dataspace = NULL;
static APTR memdataspace = NULL;
static struct SignalSemaphore storagesem;
static ULONG pushid = 0;

/* private */

static void set_generic(APTR ds, ULONG id, CONST_APTR data, ULONG len)
{
	ULONG res;

	res = DoMethod(ds, MUIM_Dataspace_Add, data, len, id);

	if (!res)
	{
		DB(("failed to store object into dataspace"));
	}
}

static void set_strarray(APTR ds, ULONG id, CONST_APTR data)
{
	ULONG res = 0;
	ULONG i;
	ULONG size = sizeof(APTR);
	CONST UBYTE **array = (CONST UBYTE **)data;

	for (i = 0;; i++)
	{
		if (array[i] == NULL)
			break;

		size += strlen(array[i]) + 1;
	}

	if (i > 0)
	{
		UBYTE *a;

		a = malloc(size);
		if (a)
		{
			UBYTE *p = a;

			for (i = 0;; i++)
			{
				int len;

				if (array[i] == NULL)
					break;

				len = strlen(array[i]) + 1;
				memcpy(p, array[i], len);
				p += len;
			}
			((UBYTE **)p)[0] = NULL; /* XXX: potentially unaligned access */

			res = DoMethod(ds, MUIM_Dataspace_Add, a, size, id);

			free(a);
		}
	}
	else
	{
		/* Remove the entry when saving empty array */
		DoMethod(ds, MUIM_Dataspace_Remove, id);
		res = TRUE;
	}

	if (!res)
	{
		DB(("failed to store object into dataspace"));
	}
}


static APTR get_generic(APTR ds, ULONG id)
{
	ULONG res;

	res = DoMethod(ds, MUIM_Dataspace_Find, id);

	return (APTR)res;
}

static APTR get_strarray(APTR ds, ULONG id)
{
	APTR res;

	res = (APTR) DoMethod(ds, MUIM_Dataspace_Find, id);


	if (res)
	{
		ULONG i;
		ULONG size = sizeof(APTR);
		UBYTE *a = res;

		for (i = 0;; i++)
		{
			if (*(UBYTE **)a == NULL) /* XXX: potentially unaligned access */
				break;
			else
			{
				int len = strlen(a) + 1;

				size += len + sizeof(APTR);
				a += len;
			}
		}

		if (i)
		{
			UBYTE **data;

			data = malloc(size);
			if (data)
			{
				ULONG ptrsize = (i + 1) * sizeof(APTR);
				ULONG j;

				a = (UBYTE *)((ULONG)data + ptrsize);

				memcpy(a, res, size - ptrsize);

				for (j = 0; j < i; j++)
				{
					data[j] = a;

					a += strlen(a) + 1;
				}

				data[i] = NULL;

				return data;
			}
		}
	}

	return NULL;
}

/* API */


ULONG storage_init(void)
{
	if ((dataspace = MUI_NewObject(MUIC_Dataspace, TAG_DONE)))
	{
		if ((memdataspace = MUI_NewObject(MUIC_Dataspace, TAG_DONE)))
		{
			struct IFFHandle *iff = AllocIFF();

			if (iff)
			{
				InitSemaphore(&storagesem);

				InitIFFasDOS(iff);

				if ((iff->iff_Stream = Open("ENVARC:ambient.data", MODE_OLDFILE)))
				{
					if (!OpenIFF(iff, IFFF_READ))
					{
						if (!ParseIFF(iff,IFFPARSE_STEP))
						{
							struct ContextNode *cn;

							if ((cn=CurrentChunk(iff)))
							{
								DoMethod(dataspace, MUIM_Dataspace_ReadIFF, iff);
							}

						}

						CloseIFF(iff);
						Close(iff->iff_Stream);
						FreeIFF(iff);

						return TRUE;

					}
					Close(iff->iff_Stream);
				}
				FreeIFF(iff);

				return TRUE;
			}
			MUI_DisposeObject(memdataspace);
			memdataspace = NULL;
		}
		MUI_DisposeObject(dataspace);
		dataspace = NULL;
	}

	return FALSE;
}

void storage_cleanup(void)
{
	if (dataspace)
	{
		storage_commit();

		MUI_DisposeObject(dataspace);
	}

	if (memdataspace)
	{
		MUI_DisposeObject(memdataspace);
	}
}

void storage_commit(void)
{
	DB(("storage commit...\n"));

	if (dataspace)
	{
		struct IFFHandle *iff = AllocIFF();

		if (iff)
		{
			InitIFFasDOS(iff);

			ObtainSemaphore(&storagesem);

			if ((iff->iff_Stream = Open("ENVARC:ambient.data", MODE_NEWFILE)))
			{
				if (!OpenIFF(iff, IFFF_WRITE))
				{
					LONG rc UNUSED = DoMethod(dataspace, MUIM_Dataspace_WriteIFF, iff, MAKE_ID('D','A','T','A'), ID_FORM);
					CloseIFF(iff);
					DB(("...commited. rc = %d\n", rc));
				}

				Close(iff->iff_Stream);
			}

			ReleaseSemaphore(&storagesem);

			FreeIFF(iff);
		}
	}
}

void storage_set(ULONG id, ULONG type, CONST_APTR data)
{
	ObtainSemaphore(&storagesem);

	switch (type)
	{
		case STORAGE_STRING:
			set_generic(dataspace, id, data, strlen(data) + 1);
			break;

		case STORAGE_STRARRAY:
			set_strarray(dataspace, id, data);
			break;

		case STORAGE_MSTRING:
			set_generic(memdataspace, id, data, strlen(data) + 1);
			break;

		case STORAGE_MSTRARRAY:
			set_strarray(memdataspace, id, data);
			break;
	}

	if (type < STORAGE_MSTRING)
	{
		if (pushid)
			DoMethod(app, MUIM_Application_KillPushMethod, app, pushid);

		pushid = DoMethod(app, MUIM_Application_PushMethod, app, 2 | MUIV_PushMethod_Delay(10000), MM_Application_CommitStorage);
	}

	ReleaseSemaphore(&storagesem);
}

void storage_get(ULONG id, ULONG type, APTR *data)
{
	ObtainSemaphore(&storagesem);

	switch (type)
	{
		case STORAGE_STRING:
			*data = get_generic(dataspace, id);
			break;

		case STORAGE_STRARRAY:
			*data = get_strarray(dataspace, id);
			break;

		case STORAGE_MSTRING:
			*data = get_generic(memdataspace, id);
			break;

		case STORAGE_MSTRARRAY:
			*data = get_strarray(memdataspace, id);
			break;
	}

	ReleaseSemaphore(&storagesem);
}

void storage_delete(ULONG id)
{
	ObtainSemaphore(&storagesem);

	DoMethod(dataspace, MUIM_Dataspace_Remove, id);

	ReleaseSemaphore(&storagesem);
}
