/*
 * Tests Ambient's wbstart.library.
 * Highly inspired from Stefan Becker's testsuite
 *
 * $Id: wbtest.c,v 1.2 2005/07/04 23:06:14 laire Exp $
 */

#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/wbstart.h>


void execute(STRPTR info, STRPTR dir, STRPTR file)
{
	Printf("%s: ", info);

	if (WBStartTags(WBStart_DirectoryName, dir,
					WBStart_Name, file,
		TAG_DONE) == RETURN_OK)
	{
		Printf("ok\n");
	}
	else
	{
		Printf("failed\n");
	}
}


void assign(STRPTR name, STRPTR path)
{
	BPTR l;

	if (path)
	{
		if (l = Lock(path, ACCESS_READ))
		{
			if (!AssignLock(name, l))
			{
				UnLock(l);
			}
		}
	}
	else
	{
		AssignLock(name, NULL);
	}
}


int main(void)
{
	assign("WBSTART", "");
	assign("WBSTART1", "wbparam");
	assign("WBSTART-MULTI", "SYS:");
	assign("WBSTART-MULTI", "");

	execute("01: tool           (normal, no path)    ", "WBSTART:wbparam",   "wbparam");
	execute("02: tool           (normal, only assign)", NULL,                "WBSTART1:wbparam");
	execute("03: tool           (normal, with path)  ", "WBSTART:",          "wbparam/wbparam");
	execute("04: tool           (normal, relative)   ", "WBSTART:",          "/wbstartlib/wbparam/wbparam");
	execute("05: project nofile (normal, no path)    ", "WBSTART:",          "Project-NoFile1");
	execute("06: project file   (normal, only assign)", NULL,                "WBSTART:Project-File1");
	execute("07: project nofile (normal, only assign)", NULL,                "WBSTART:Project-NoFile1");
	execute("08: project file   (normal, with path)  ", "WBSTART:",          "wbparam/Project-File2");
	execute("09: project nofile (normal, with path)  ", "WBSTART:",          "wbparam/Project-NoFile2");
	execute("10: project file   (multi,  only assign)", NULL,                "WBSTART-MULTI:Project-File1");
	execute("11: project nofile (multi,  only assign)", NULL,                "WBSTART-MULTI:Project-NoFile1");
	execute("12: project file   (multi,  with path)  ", NULL,                "WBSTART-MULTI:wbparam/Project-File2");
	execute("13: project nofile (multi,  with path)  ", NULL,                "WBSTART-MULTI:wbparam/Project-NoFile2");
	execute("14: tool softlink  (normal, no path)    ", "WBSTART:",          "Tool-Softlink1");
	execute("15: tool softlink  (normal, only assign)", NULL,                "WBSTART:Tool-Softlink1");
	execute("16: tool softlink  (normal, with path)  ", "WBSTART:",          "softlink/Tool-Softlink2");
	execute("17: tool softlink  (normal, relative)   ", "WBSTART:",          "/wbstartlib/Tool-Softlink1");
	execute("18: tool softlink  (multi,  only assign)", NULL,                "WBSTART-MULTI:Tool-Softlink1");
	execute("19: tool softlink  (multi,  with path)  ", NULL,                "WBSTART-MULTI:softlink/Tool-Softlink2");

	assign("WBSTART-MULTI", NULL);
	assign("WBSTART1", NULL);
	assign("WBSTART", NULL);

	return (0);
}
