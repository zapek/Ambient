/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2016 Ambient Open Source Team
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * $Id: snapshot.c,v 1.19 2024/01/23 22:03:07 piru Exp $
 */

#include "ambient.h"

/* public */
#include <workbench/workbench.h>
#include <stddef.h> /* for offsetoff() */
#include <proto/dos.h>

/* private */
#include "snapshot.h"
#include "file_io.h"
#include "pngicon.h"
#include "pngio.h"
#include "iconview.h" /* XXX: not needed later */
#include "name.h"
#include "notify.h"
#include "methodstack.h"
#include "mui_func.h"
#include "prefs.h"
#include "iconio.h"
#include "deficonpool.h"
#include <clib/debug_protos.h>
#include "sxmlc.h"
#include "sxmlsearch.h"
#include "sxmlhelp.h"
#include "svgicon.h"

#define IOBUFFERSIZE (1024)

/*
 * Snapshots an icon X/Y position. If X/Y is NO_ICON_POSITION
 * then the snapshot is reseted.
 * XXX: add also window size, fix read_icon() to be able
 * to be called with obj == NULL meaning it should do a kind
 * of "sanity check". Current snapshot_icon() will kill a file
 * which is not an icon..
 * XXX: retval isn't checked by anything atm..
 */
ULONG tr_snapshot_icon(CONST_STRPTR filename, LONG x, LONG y)
{
	APTR fh;
	ULONG retval = FALSE;
	BOOL svg = FALSE;

	THREAD;
	ASSERT(filename);

	D(ICONIO,bug("trying to snapshot <%s>\n", filename));

	/* to not cause contents of window reload */

	methodstack_push_sync( app, 3, MM_Application_EnableDOSNotify, filename , FALSE );

	/* */

	if ( (fh = file_open(filename, MODE_OLDFILE)) )
	{
		struct DiskObject diskobj;

		/*
		 * Check what kind of icon it is.. We cannot use
		 * MODE_READWRITE here as asyncio writes when reading
		 * in that mode.
		 */
		if (file_read(fh, &diskobj, 8))
		{
			D(ICONIO,bug("first bytes: 0x%lx, 0x%lx\n", (ULONG)diskobj.do_Magic, (ULONG)diskobj.do_Version));
			if (diskobj.do_Magic == WB_DISKMAGIC && diskobj.do_Version == WB_DISKVERSION)
			{
				D(ICONIO,bug("that one is an old icon\n"));
				file_close(fh);

				if ( (fh = file_open(filename, MODE_READWRITE)) )
				{
					if (file_seek(fh, offsetof(struct DiskObject, do_CurrentX), OFFSET_BEGINNING) != -1LL)
					{
						D(ICONIO,bug("ok, writing the new positions (x: %ld, y: %ld)\n", x, y));
						if (file_write(fh, &x, sizeof(x)) && file_write(fh, &y, sizeof(y)))
						{
							retval = TRUE;
						}
					}

					file_close(fh);

					/* Enable DOS Notify for source and destination views */

					methodstack_push_sync( app, 3, MM_Application_EnableDOSNotify, filename, TRUE );

					return (retval);
				}
			}
			#if USE_SVGICONS
			else if(svg_signature_buffer(&diskobj, 8))
			{
				file_close(fh);
				svg = TRUE;
				APTR obj = NewObject(geticonclass(), NULL, TAG_DONE);
				
				if(obj)
				{
					XMLDoc *svgdoc = read_svgtags((char*)filename, obj);
					if(svgdoc)
					{
						XMLNode *svgnode = XMLDoc_root(svgdoc);
						if(svgnode)
						{
							XMLNode *metadata = getnode_xp(svgnode, "svg/metadata", FALSE);

							XMLNode *morphosicon = NULL;

							if(strcmp(svgnode->tag, "svg") == 0)
							{
								if(metadata)
								{
									D(ICONIO,bug("found metadata node\n"));
									morphosicon = getnode_xp(metadata, "ambient:icondata", FALSE);
									if(morphosicon)
									{
										int nodenr = getnodenum(metadata, morphosicon);
										XMLNode_remove_child(metadata, nodenr, true);
									}
									else
									{
										D(ICONIO,bug("did not find a morphos icon node\n"));
									}
								}
								else
								{
									D(ICONIO,bug("did not find a metadata node, creating one\n"));
									metadata = XMLNode_alloc();
									if (!XMLNode_set_tag(metadata, C2SX("metadata")) ||
									    !XMLNode_set_type(metadata, XTAG_FATHER) ||
									    !XMLNode_insert_child(svgnode, XINSERT_TOP, metadata))
									{
										XMLNode_free(metadata);
										metadata = NULL;
									}
								}

								D(ICONIO,bug("creating a new morphos icon node\n"));
								morphosicon = XMLNode_alloc();
								
								if (x == NO_ICON_POSITION || y == NO_ICON_POSITION)
								{
									methodstack_push(obj, 3, MUIM_Set, MA_Icon_HasPos, FALSE);
								}
								else
								{
									methodstack_push(obj, 3, MUIM_Set, MA_Icon_X, x);
									methodstack_push(obj, 3, MUIM_Set, MA_Icon_Y, y);
									methodstack_push(obj, 3, MUIM_Set, MA_Icon_HasPos, TRUE);
								}

								if (SetSVGIconContents(obj, morphosicon) &&
								    XMLNode_add_child(metadata, morphosicon))
								{
									retval = save_xmldoc(svgdoc, filename);
								}
								else
								{
									XMLNode_free(morphosicon);
								}
							}
							else
							{
								D(ICONIO,bug("this does not appear to be an SVG file\n"));
							}
						}
						else
						{
							D(ICONIO,bug("Could not get the the root node for the document\n"));
						}
						
						XMLDoc_free(svgdoc);
						FreeMem(svgdoc, sizeof(XMLDoc));
					}
					
					DisposeObject(obj);
				}
			}
			#endif
			else
			{
				#if USE_PNGICONS
				APTR ctx;

				file_close(fh);

				D(ICONIO,bug("that one is a PNGicon\n"));

				if ( (ctx = pngio_create(filename, FALSE)) )
				{
					ULONG rc;
					ULONG ctx_saved = FALSE;

					if (x == NO_ICON_POSITION || y == NO_ICON_POSITION)
					{
						pngio_tag_delete(ctx, PNGICON_LocationX);
						pngio_tag_delete(ctx, PNGICON_LocationY);
						rc = TRUE;
					}
					else
					{
						rc = (pngio_tag_add(ctx, PNGICON_LocationX, &x) && pngio_tag_add(ctx, PNGICON_LocationY, &y));
					}

					if (rc)
					{
						D(ICONIO,bug("chunk added\n"));
						retval = TRUE;
						ctx_saved = TRUE;
					}

					if (ctx_saved)
					{
						APTR bm = NULL;
						APTR iconobj = NULL;

						/* Get second image, sigh, isn't there a better way? */
						if(_conf(icon_dualpng))
						{
							if ((iconobj = (APTR)methodstack_push_sync(app, 3, MM_Application_CreateIcon, TRUE, MV_ViewID_Unknown)))
							{
								if(icon_read((STRPTR) filename, iconobj, ICONTAG_Deficon, FALSE, TAG_DONE))
								{
									methodstack_push_sync(iconobj, 3, OM_GET, MA_Icon_ImageSelected, &bm);
								}
							}
						}

						if ( (fh = file_open(filename, MODE_NEWFILE)) )
						{
							if(bm)
							{
								pngio_add_bitmap(ctx, bm, 1);
							}

							if (pngio_save(filename, ctx, fh))
							{
								retval = TRUE;
							}

							file_close(fh);
						}

						if(iconobj)
						{
							methodstack_push_sync(app, 2, MM_Application_DisposeObject, iconobj);
						}
					}
					pngio_delete(ctx);
				}

				/* Enable DOS Notify for source and destination views */

				methodstack_push_sync( app, 3, MM_Application_EnableDOSNotify, filename, TRUE );

				return (retval);
				#endif
			}
		}
		if(!svg)
			file_close(fh);
	}

	/* Enable DOS Notify for source and destination views */

	methodstack_push_sync( app, 3, MM_Application_EnableDOSNotify, filename, TRUE );

	return (retval);
}


ULONG tr_snapshot_window(CONST_STRPTR origfilename, LONG x, LONG y, ULONG xs, ULONG ys, ULONG flags) /* XXX: add the viewmode */
{
	APTR fh;
	ULONG retval = FALSE;
	STRPTR filename;
	STRPTR defname = NULL;
	BOOL svg = FALSE;

	THREAD;
	ASSERT(origfilename);

	D(ICONIO,bug("trying to snapshot window, file is <%s>..\n", origfilename));

	if ( (filename = name_build_info(origfilename)) )
	{
		/* to not cause contents of window reload */
		methodstack_push_sync( app, 3, MM_Application_EnableDOSNotify, filename , FALSE );

		fh = file_open(filename, MODE_OLDFILE);
		
		if (NULL == fh)
		{
			/* magic deficon fallback */
			ULONG is_default = FALSE;
			
			defname = deficonpool_get_icon_name(MV_ViewID_Unknown, (STRPTR)origfilename, &is_default);
			
			if (NULL != defname)
			{
				D(ICONIO,bug("falling back to deficon <%s>..\n", defname));

				fh = file_open(defname, MODE_OLDFILE);
				
				if (NULL == fh)
				{
					name_delete(defname);
					defname = NULL;
				}
			}
		}

		if (NULL != fh)
		{
			struct DiskObject diskobj;

			/*
			 * Check what kind of icon it is.. We cannot use
			 * MODE_READWRITE here as asyncio writes when reading
			 * in that mode.
			 */
			if (file_read(fh, &diskobj, 8))
			{
				D(ICONIO,bug("first bytes: 0x%lx, 0x%lx\n", (ULONG)diskobj.do_Magic, (ULONG)diskobj.do_Version));
				if (diskobj.do_Magic == WB_DISKMAGIC && diskobj.do_Version == WB_DISKVERSION)
				{
					if (NULL == defname) // won't work for this...
					{
						D(ICONIO,bug("that one is an old icon\n"));
						file_close(fh);

						if ( (fh = file_open(filename, MODE_READWRITE)) )
						{
							if (file_seek(fh, sizeof(struct DiskObject), OFFSET_BEGINNING) != -1LL)
							{

								if (x == NO_ICON_POSITION || y == NO_ICON_POSITION)
								{
									PDB(("window unsnapshoting NYI\n"));
									/* XXX: there I need to set dd_DrawerData to 0 and remove the DrawerData structure, but it's not trivial */
								}
								else
								{
									WORD wx, wy;
									UWORD wxs, wys;

									wx = x;
									wy = y;
									wxs = xs;
									wys = ys;

									D(ICONIO,bug("ok, writing the new infos (x: %ld, y: %ld, xs: %lu, ys: %lu, flags: 0x%lx)\n", x, y, xs, ys, flags));

									if (file_write(fh, &wx, sizeof(wx)) && file_write(fh, &wy, sizeof(wy)) &&
										file_write(fh, &wxs, sizeof(wxs)) && file_write(fh, &wys, sizeof(wys)) //&&
										//(file_seek(fh, sizeof(struct NewWindow) - sizeof(wx) - sizeof(wy) - sizeof(wxs) - sizeof(wys) + sizeof(LONG) + sizeof(LONG), OFFSET_CURRENT) != -1) && /* what a mess.. */
										//file_write(fh, &flags, sizeof(flags))
										)
									{
										/* XXX: sigh.. fucking drawerdata is at the end.. that sucks majorly! */
										D(ICONIO,bug("writing was fine\n"));
										retval = TRUE;
									}
								}
							}
						}
					}
				}
				#if USE_SVGICONS
				else if(svg_signature_buffer(&diskobj, 8))
				{
					file_close(fh);
					svg = TRUE;
					APTR obj = NewObject(geticonclass(), NULL, TAG_DONE);
					
					if(obj)
					{
						XMLDoc *svgdoc = read_svgtags(defname ? defname : filename, obj);
						if(svgdoc)
						{
							XMLNode *svgnode = XMLDoc_root(svgdoc);
							if(svgnode)
							{
								XMLNode *metadata = getnode_xp(svgnode, "svg/metadata", FALSE);

								XMLNode *morphosicon = NULL;

								if(strcmp(svgnode->tag, "svg") == 0)
								{
									if(metadata)
									{
										D(ICONIO,bug("found metadata node\n"));
										morphosicon = getnode_xp(metadata, "ambient:icondata", FALSE);
										if(morphosicon)
										{
											int nodenr = getnodenum(metadata, morphosicon);
											XMLNode_remove_child(metadata, nodenr, true);
										}
										else
										{
											D(ICONIO,bug("did not find a morphos icon node\n"));
										}
									}
									else
									{
										D(ICONIO,bug("did not find a metadata node, creating one\n"));
										metadata = XMLNode_alloc();
										if (!XMLNode_set_tag(metadata, C2SX("metadata")) ||
										    !XMLNode_set_type(metadata, XTAG_FATHER) ||
										    !XMLNode_insert_child(svgnode, XINSERT_TOP, metadata))
										{
											XMLNode_free(metadata);
											metadata = NULL;
										}
									}

									D(ICONIO,bug("creating a new morphos icon node\n"));
									morphosicon = XMLNode_alloc();
									
									int viewmode = IVM_ICON;

									if (flags == 3)
									{
										viewmode = IVM_SHOWALL;
									}
									else if (flags == 4)
									{
										viewmode = IVM_THUMBS;
									}
									else if (flags == 1)
									{
										viewmode = IVM_LISTER;
									}
									
									if (x == NO_ICON_POSITION || y == NO_ICON_POSITION)
									{
										methodstack_push(obj, 3, MUIM_Set, MA_Icon_HasDrawerData, FALSE);	
									}
									else
									{
										methodstack_push(obj, 3, MUIM_Set, MA_Icon_Type, MV_Icon_Type_Drawer);
										methodstack_push(obj, 3, MUIM_Set, MA_Icon_WindowLeft, x);
										methodstack_push(obj, 3, MUIM_Set, MA_Icon_WindowTop, y);
										methodstack_push(obj, 3, MUIM_Set, MA_Icon_WindowWidth, xs);
										methodstack_push(obj, 3, MUIM_Set, MA_Icon_WindowHeight, ys);
										methodstack_push(obj, 3, MUIM_Set, MA_Icon_ViewMode, viewmode);
										methodstack_push(obj, 3, MUIM_Set, MA_Icon_HasDrawerData, TRUE);
									}
									
									if (SetSVGIconContents(obj, morphosicon) &&
									    XMLNode_add_child(metadata, morphosicon))
									{
										retval = save_xmldoc(svgdoc, filename);
									}
									else
									{
										XMLNode_free(morphosicon);
									}
								}
								else
								{
									D(ICONIO,bug("this does not appear to be an SVG file\n"));
								}
							}
							else
							{
								D(ICONIO,bug("Could not get the the root node for the document\n"));
							}
							
							XMLDoc_free(svgdoc);
							FreeMem(svgdoc, sizeof(XMLDoc));
						}
						
						DisposeObject(obj);
					}
				}
				#endif
				else
				{
					#if USE_PNGICONS
					APTR ctx;

					file_close(fh);

					D(ICONIO,bug("that one is a PNGicon\n"));

					if ( (ctx = pngio_create(defname ? defname : filename, FALSE)) )
					{
						ULONG rc;
						ULONG ctx_saved = FALSE;


						if (x == NO_ICON_POSITION || y == NO_ICON_POSITION)
						{
							pngio_tag_delete(ctx, PNGICON_DrawerLeft);
							pngio_tag_delete(ctx, PNGICON_DrawerTop);
							pngio_tag_delete(ctx, PNGICON_DrawerWidth);
							pngio_tag_delete(ctx, PNGICON_DrawerHeight);
							pngio_tag_delete(ctx, PNGICON_DrawerViewMode);

							rc = TRUE;
						}
						else
						{
							/* XXX: hack! */
							int viewmode = IVM_ICON;

						  	if (flags == 3)
							{
								viewmode = IVM_SHOWALL;
							}
							else if (flags == 4)
							{
								viewmode = IVM_THUMBS;
							}
							else if (flags == 1)
							{
								viewmode = IVM_LISTER;
							}

							/* XXX: end of hack */

							rc = (pngio_tag_add(ctx, PNGICON_DrawerLeft, &x) &&
								  pngio_tag_add(ctx, PNGICON_DrawerTop, &y) &&
								  pngio_tag_add(ctx, PNGICON_DrawerWidth, &xs) &&
								  pngio_tag_add(ctx, PNGICON_DrawerHeight, &ys) &&
								  pngio_tag_add(ctx, PNGICON_DrawerViewMode, &viewmode));
						}

						if (rc)
						{
							D(ICONIO,bug("chunk added\n"));
							retval = TRUE;
							ctx_saved = TRUE;
						}

						if (ctx_saved)
						{
							APTR bm = NULL;
							APTR iconobj = NULL;

							/* Get second image, sigh, isn't there a better way? */
							if(_conf(icon_dualpng))
							{
								if ((iconobj = (APTR)methodstack_push_sync(app, 3, MM_Application_CreateIcon, TRUE, MV_ViewID_Unknown)))
								{
									if(icon_read((STRPTR) defname ? defname : filename, iconobj, ICONTAG_Deficon, FALSE, TAG_DONE))
									{
										methodstack_push_sync(iconobj, 3, OM_GET, MA_Icon_ImageSelected, &bm);
									}
								}
							}

							if ( (fh = file_open(filename, MODE_NEWFILE)) )
							{
								if(bm)
								{
									pngio_add_bitmap(ctx, bm, 1);
								}

								if (pngio_save(filename, ctx, fh))
								{
									retval = TRUE;
								}

								file_close(fh);
							}

							if(iconobj)
							{
								methodstack_push_sync(app, 2, MM_Application_DisposeObject, iconobj);
							}
						}
						pngio_delete(ctx);
					}
					goto done;
					#endif
				}
			}
			if(!svg)
				file_close(fh);
		}

		done:

		if (retval)
		{
			/* snapshoting a partition */
			ULONG len = strlen(filename);
			STRPTR tmpn = filename + len - 9;

			if (len >= 9)
			{
				if (stricmp(tmpn, "disk.info") == 0)
				{
					TEXT buffer[strlen(origfilename) + 9 /* disk.info */ + 1];
					strcpy(buffer, origfilename);
					strcat(buffer, "disk.info");

					/* a deficon is used, no other choice than reloading all icons for now */
					if(stricmp(filename, buffer))
					{
						DoMethod(app, MM_Application_ReloadIcons, TRUE);
					}
					else
					/* path:disk.info -> just remove/readd icon */
					{
						notify_action(filename, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Create);
					}
				}
				/* some drawer */
				else
				{
					notify_action(filename, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Create); /* XXX: that's not entirelly correct */
				}
			}
			/* some drawer */
			else
			{
				notify_action(filename, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Create); /* XXX: that's not entirelly correct */
			}
		}

		/* Enable DOS Notify for source and destination views */

		methodstack_push_sync( app, 3, MM_Application_EnableDOSNotify, filename, TRUE );

		name_delete(filename);
		if (defname) name_delete(defname);
	}
	/* XXX */

	return (retval);
}

