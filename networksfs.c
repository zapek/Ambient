#include "networksfs.h"
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <exec/rawfmt.h>
#include <string.h>

BOOL is_networksfs(CONST_STRPTR path)
{
	if (FindPort("NETBIOS"))
		return Strnicmp(path, "networks:", 9) == 0;// && !strstr(path, "/");
	return FALSE;
}

void networksfs_settings(CONST_STRPTR path)
{
	ULONG len = strlen(path);
	if (len > 512)
		return;
	char buffer[len + 20];
	strcpy(buffer, path);
	if (AddPart(buffer, ".settings", sizeof(buffer)))
	{
		struct DateStamp ds = { 0 };
		SetFileDate(buffer, &ds);
	}
}

void networksfs_connect(void)
{
	struct DateStamp ds = { 0 };
	SetFileDate("NETBIOS:.connect", &ds);
}
