

/* public */
#include <libraries/locale.h>
#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/locale.h>

/* private */
#include "locale.h"
#include "panelprefs_cat.h"




#warning lcale pfusch
/* catmaker */
#define CATCOMP_NUMBERS
extern const char * const __stringtable[];

#ifdef __SASC
#define GSI(x) __stringtable[x]
#else
#define GSI(x) ( STRPTR )__stringtable[x]
#endif
//#include "debug.h"
#include <clib/debug_protos.h>


static struct Catalog *catalog;

struct Locale *locale;

LONG   locale_timezone_offset;

ULONG locale_init(void)
{
	/* Safe with NULL argument */
#if 1
	locale = OpenLocale(NULL);

	locale_timezone_offset = -locale->loc_GMTOffset * 60;
	catalog = OpenCatalog(
		NULL, "panelprefs.catalog",
		OC_BuiltInLanguage, "english",
		OC_Version, 1,  /* required catalog version */
		TAG_DONE
	);

	if (catalog)
	{
		int c;
		for (c = 0; c < NUMCATSTRING; c++)
		{
			((char**)__stringtable)[ c ] = GetCatalogStr(catalog, c, (char*)__stringtable[c]);
		}
		return c;
	}
#endif
	return (TRUE);
}

void locale_cleanup(void)
{
	/* Both safe with NULL arguments */
	CloseLocale(locale);
	CloseCatalog(catalog);
}


