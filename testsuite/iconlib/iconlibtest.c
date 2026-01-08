/*
 * Tests icon.library.
 *
 * $Id: iconlibtest.c,v 1.1 2016/07/18 08:20:48 itix Exp $
 */

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/icon.h>

static int GetSize(CONST_STRPTR name)
{
	BPTR lock = Lock(name, ACCESS_READ);

	if (lock)
	{
		struct FileInfoBlock fib;

		Examine64(lock, &fib, NULL);
		UnLock(lock);

		return fib.fib_Size64;
	}
	else
	{
		Printf("Failed to open file %s:\nDOS error %ld\n", name, IoErr());
	}

	return 0;
}

static int GetPutObject()
{
	APTR dobj = GetDiskObject("ENVARC:sys/def_disk");
	int rc = 0;

	if (!dobj)
	{
		PutStr("Failed to read icon!\n");
		return rc;
	}

	if (dobj)
	{
		TEXT buf[200], buf2[200];
		ULONG i;

		for (i = 0; i < 3; i++)
		{
			int size;

			NewRawDoFmt("T:iconlibtest_%ld", NULL, buf, i);
			NewRawDoFmt("T:iconlibtest_%ld.info", NULL, buf2, i);
			rc = PutDiskObject(buf, dobj);

			if (!rc)
			{
				Printf("Failed to write icon %s:\nError %ld.\n", buf, IoErr());
				//break;
			}

			if ((size = GetSize(buf2)) == 0)
			{
				Printf("Failed to write icon %s:\n%ld bytes.\n", buf2, size);
				rc = FALSE;
				break;
			}
		}
	}


	return rc;
}

int main()
{
	if (GetPutObject())
	{
		return RETURN_OK;
	}

	return RETURN_FAIL;
}
