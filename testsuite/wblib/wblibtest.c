/*
 * Tests Ambient's workbench.library.
 *
 * $Id: wblibtest.c,v 1.5 2016/08/28 14:14:15 itix Exp $
 */

#define USE_INLINE_STDARG

#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/wb.h>


struct pathentry {
	BPTR pe_next;
	BPTR pe_lock;
};


static void wbcontrol_searchpath(void)
{
	BPTR path = 0;

	PutStr("Testing WorkbenchControl(NULL, WBCTRLA_DuplicateSearchPath, ...)\n");

	if (WorkbenchControl(NULL, WBCTRLA_DuplicateSearchPath, (IPTR)&path, TAG_DONE))
	{
		struct pathentry *pe = BADDR(path), *peo;

		while (pe)
		{
			TEXT path[1024];

			peo = pe;
			pe = BADDR(pe->pe_next);
			NameFromLock(peo->pe_lock, path, sizeof(path));
			Printf("%s\n", (IPTR)path);
		}
	}
	else
	{
		PutStr("WorkbenchControl() returned FALSE\n");
	}
}


static void openwbobject(void)
{
	PutStr("\nTesting OpenWorkbenchObjectA()\n");

	if (OpenWorkbenchObject("RAM:", WBOPENA_Show, DDFLAGS_SHOWALL, WBOPENA_ViewBy, DDVM_BYNAME, TAG_DONE))
	{
		PutStr("Succeeded!\n");
	}
	else
	{
		PutStr("Failed!\n");
	}
}


static void wbinfo(void)
{
	BPTR lock;

	PutStr("\nTesting WBInfo()\n");

	lock = Lock("SYS:", ACCESS_READ);

	if (lock)
	{
		WBInfo(lock, "Prefs.info", NULL);
		UnLock(lock);
	}
}


static void drawer()
{
	PutStr("\nTesting CreateDrawerA()\n");

	CreateDrawerA   ("RAM:wblibtest/dummy1", NULL);
	CreateDrawerA   ("RAM:wblibtest/dummy2/icons", NULL);
	CreateDrawerTags("RAM:wblibtest/dummy3/noicons", WBCREATEDRAWER_NoIcons, TRUE, TAG_DONE);
}


static void writefile(STRPTR name, STRPTR data)
{
	BPTR fh = Open(name, MODE_NEWFILE);

	if (fh)
	{
		Write(fh, data, strlen(data));
		Close(fh);
	}
}

static void icons()
{
	CreateDrawerTags("RAM:wblibtest/iconless", WBCREATEDRAWER_CreateIcon, FALSE, TAG_DONE);
	CreateIconA("RAM:wblibtest/iconless", NULL);
	CreateDrawerTags("RAM:wblibtest/iconless2", WBCREATEDRAWER_CreateIcon, FALSE, TAG_DONE);
	CreateIconTags("RAM:wblibtest/iconless2", WBCREATEICON_DefIcon, FALSE, TAG_DONE);

	writefile("RAM:wblibtest/document.txt", "a\nb\nc\n");
	writefile("RAM:wblibtest/document", "a\nb\nc\n");
	writefile("RAM:wblibtest/document.c", "#include <stdio.h>\n\nint main()\n{\n\tprintf(\"Hello world!\");\n\treturn 0;\n}\n");
	writefile("RAM:wblibtest/document2", "#include <stdio.h>\n\nint main()\n{\n\tprintf(\"Hello world!\");\n\treturn 0;\n}\n");

	CreateIconTags("RAM:wblibtest/document.txt.info", TAG_DONE);
	CreateIconTags("RAM:wblibtest/document.info", WBCREATEICON_DefIcon, FALSE, WBCREATEICON_MimeType, "text/html", TAG_DONE);
	CreateIconTags("RAM:wblibtest/document.c.info", WBCREATEICON_DefIcon, FALSE, TAG_DONE);

	// This should give PNG icon
	CreateIconTags("RAM:wblibtest/document2.info", WBCREATEICON_DefIcon, FALSE, WBCREATEICON_File, "MOSSYS:Prefs/Gfx/Audio/Channels.png", TAG_DONE);
}


int main(void)
{
	wbcontrol_searchpath();
	wbinfo();
	drawer();
	openwbobject();
	icons();

	return (0);
}
