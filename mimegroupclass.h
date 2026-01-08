#ifndef AMBIENT_MIMEGROUPCLASS_H
#define AMBIENT_MIMEGROUPCLASS_H

#define NODEFLAG_MIMETYPE   1
#define NODEFLAG_MEDIATYPE  2
#define NODEFLAG_OVERLOADED 4

struct treedata
{
	ULONG  flags;
	STRPTR longname;
	LONG   priority;
	
	TEXT   buffer_name[256];
	TEXT   buffer_pri[32];
};

#endif

