
#include <dos/exall.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/query.h>
#include <exec/execbase.h>
/* private */
#include "ambient.h"
#include "mui_func.h"
#include "paneltags.h"
#include "panelitem.h"
#include "panelclasslist.h"
#include "classes.h"

#include "file_func.h" // bitRocky: just for some tests in ScanDirectory
/************************************************************************/

struct Data {
	struct List classlist;
};

/************************************************************************/

static struct List *panelclasslist;

struct List *GetPanelclassList()
{
	return( panelclasslist );
}

/************************************************************************/

DEFNEW
{
	if( ( obj  = DoSuperNew( cl, obj, TAG_DONE ) ) )
	{
		struct Data *data = INST_DATA( cl, obj );

		NEWLIST( &data->classlist );

		panelclasslist = &data->classlist;

		DoMethod( obj, MM_Panelclasslist_RefreshClasslist );
	}
	return( (ULONG) obj );
}

/************************************************************************/

DEFDISP
{
	DoMethod( obj, MM_Panelclasslist_FreeClasslist );

	return( DOSUPER );
}

/************************************************************************/

DEFGET
{
	return( DOSUPER );
}

/************************************************************************/

DEFSMETHOD(Panelclasslist_ScanDirectory)
{
	GETDATA;
	struct Node *node;
	APTR PathNode = 0;
	#define SCANFILENAME_SIZEOF 0x200
	TEXT filename[ SCANFILENAME_SIZEOF ];

	ULONG QueryTags[] =
	{
		QUERYFINDATTR_CLASS     , QUERYCLASS_AMBIENT,
		QUERYFINDATTR_SUBCLASS  , QUERYSUBCLASS_AMBIENT_PANEL,
		TAG_DONE
	};

	void *queryinfo = NULL;

PDB(("msg->path = '%s'\n", msg->path));
#ifdef DEBUG
	if (exists("ENV:NoPanelClassScan")) { PDB(("skipit!\n")); return 0;}; // bitRocky
#endif
PDB(("vor QueryCreatePathNode(%s)\n", msg->path));
	if( ( PathNode = QueryCreatePathNode( msg->path, 0, QUERYPATHFLAGF_ALL ) ) )
	{
PDB(("vor while()\n"));// Delay(50);
		while( ( queryinfo = QueryObtainTagList( queryinfo, (struct TagItem *) QueryTags ) ) )
		{
			char  *class_name;
PDB(("vor QueryGetAttr()\n")); // Delay(50);
			if( QueryGetAttr( queryinfo, (ULONG *) &class_name, QUERYINFOATTR_NAME ) )
			{
PDB(("class_name = '%s'\n", class_name));
				if( class_name && !( strcmp( &class_name[ strlen( class_name ) - 5 ],".pobj" ) ) )
				{
					strcpy( filename, msg->path );
					AddPart(filename, class_name, SCANFILENAME_SIZEOF );

					if( !( FindName( &data->classlist, class_name ) ) )
					{
						if( ( node = malloc( sizeof( struct Node ) ) ) )
						{
							if( ( node->ln_Name = malloc( strlen( class_name ) + 1 ) ) )
							{
								strcpy( node->ln_Name, class_name );
								ADDTAIL( &data->classlist, node );
							} else {
								free( node );
							}
						}
					}
				}
			}
		}
PDB(("vor QueryDeletePathNode()\n"));
		QueryDeletePathNode( PathNode );
	}
PDB(("vor return(0)\n"));
	return( 0 );
}

/************************************************************************/

DEFTMETHOD(Panelclasslist_RefreshClasslist)
{
	DoMethod( obj, MM_Panelclasslist_FreeClasslist );

	DoMethod( obj, MM_Panelclasslist_ScanDirectory, PANEL_DISKPATHMOSSYS );
//	DoMethod( obj, MM_Panelclasslist_ScanDirectory, PANEL_OLDDISKPATHSYS );
	DoMethod( obj, MM_Panelclasslist_ScanDirectory, PANEL_DISKPATHSYS    );

	return( 0 );
}

/************************************************************************/

DEFTMETHOD(Panelclasslist_FreeClasslist)
{
	GETDATA;
	struct Node *node, *nnode;

	ITERATELISTSAFE( node, nnode, &data->classlist )
	{
		free( node->ln_Name ); /* o.k. than remove from list*/
		REMOVE( node );
	}

	return( 0 );
}

/************************************************************************/

BEGINMTABLE

DECNEW
DECGET
DECDISP
DECSMETHOD(Panelclasslist_ScanDirectory)
DECTMETHOD(Panelclasslist_RefreshClasslist)
DECTMETHOD(Panelclasslist_FreeClasslist)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Notify, panelclasslistclass)

