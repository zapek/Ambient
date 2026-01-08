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
 * $Id: exdir.c,v 1.10 2016/07/31 18:51:13 itix Exp $
 */

#include "ambient.h"

/* public */
#include <dos/exall.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <stddef.h> /* for offsetof() */

/* private */
#include "exdir.h"
#include "iconio.h"
#include "mui_func.h"
#include "methodstack.h"
#include "fonts.h"
#include "prefs.h"
#include "threads.h"


#define EAC_BUFFERSIZE 4096 /* ExAll() buffer */

ULONG exdir(APTR obj, CONST_STRPTR path, CONST_STRPTR pattern, ULONG max_ed, LONG type, ULONG (*matchfunc)(APTR obj, CONST_STRPTR path, struct ExAllData *ead, APTR userdata), ULONG (*notmatchfunc)(APTR obj, CONST_STRPTR path, struct ExAllData *ead, APTR userdata), struct dirinfo *di, APTR userdata)
{
	THREAD;

	if (path)
	{
		BPTR l;
		struct ExAllControl *eac;
		struct ExAllData *ead;
		APTR ex_buffer;
		ULONG retval = FALSE;
		CONST TEXT iconspattern[] = "#?.info";
		ASSERT(pattern);

		D(EXDIR,bug("ExDir()ing <%s>, pattern: <%s>..\n", path, pattern));

		if ( (l = Lock(path, ACCESS_READ)) )
		{
			D_S(struct FileInfoBlock, fib);

			D(EXDIR,bug("locked\n"));

			if ( Examine(l, fib) && fib->fib_DirEntryType > 0 )
			{
				D(EXDIR,bug("is a dir\n"));

				if ( (eac = AllocDosObject(DOS_EXALLCONTROL, NULL)) )
				{
					D(EXDIR,bug("eac structure allocated\n"));
					if ( (ex_buffer = malloc(EAC_BUFFERSIZE)) )
					{
						ULONG patsize = strlen(pattern) * 2 + 2;
						STRPTR parsepat;

						D(EXDIR,bug("ex_buffer allocated, patsize: %lu\n", patsize));

						if ((parsepat = malloc(patsize)))
						{
							TEXT iconsparsepat[sizeof(iconspattern) * 2 + 2];

							D(EXDIR,bug("parsepats allocated\n"));

							if ((ParsePatternNoCase(pattern, parsepat, patsize) > -1) &&
							    (ParsePatternNoCase(iconspattern, iconsparsepat, sizeof(iconsparsepat)) > -1))
							{
								LONG ioerr;
								ULONG use_ed;
								int more;

								D(EXDIR,bug("parsed\n"));

								eac->eac_LastKey = 0;
								eac->eac_MatchString = NULL; /* Scan all, will match against pattern ourself */

								use_ed = max_ed;

								/*
								 * Promote use_ed to at least ED_SIZE if 'dirinfo' is requested
								 */
								#if USE_LEGACY
								if (di && use_ed < ED_SIZE)
								{
									use_ed = ED_SIZE;
								}
								#else
								if (di && use_ed < ED_SIZE64)
								{
									use_ed = ED_SIZE64;
								}
								#endif

								retval = TRUE;

								do
								{
									more = ExAll(l, ex_buffer, EAC_BUFFERSIZE, use_ed, eac); /* XXX: date ? */
									D(EXDIR,bug("ExAll()ed, retval: %ld\n", (LONG)more));

									if (!more)
									{
										ioerr = IoErr();

										if (ioerr != ERROR_NO_MORE_ENTRIES)
										{

											if (ioerr == ERROR_BAD_NUMBER &&
											    use_ed > ED_COMMENT)
											{
												#if !USE_LEGACY
												/*
												 * If ED_SIZE64 is not available,
												 * fallback to ED_OWNER.
												 */
												if (use_ed >= ED_SIZE64)
												{
													use_ed = ED_OWNER;
													more = 1;
													continue;
												}
												#endif

												/*
												 * If ED_OWNER is not available,
												 * fallback to ED_COMMENT.
												 */
												if (use_ed >= ED_OWNER)
												{
													use_ed = ED_COMMENT;
													more = 1;
													continue;
												}
											}

											/*
											 * kiero: We don't return error if ioerr is ERROR_READ_PROTECTED.
											 */

											if (ioerr != ERROR_READ_PROTECTED)
												retval = FALSE;

											break;
										}
									}

									if (eac->eac_Entries == 0)
									{
										continue;
									}

									D(EXDIR,bug("got %ld entries\n", eac->eac_Entries));

									ead = (struct ExAllData *)ex_buffer;
									do
									{
										struct ExAllData tmp_ead;
										struct ExAllData *use_ead;

										D(EXDIR,bug("acting on <%s>..\n", (STRPTR)ead->ed_Name));
										if (threads_check_abort())
										{
											retval = ABORTED;
											more = FALSE;
											D(EXDIR, bug("aborting..\n"));
											ExAllEnd(l, ex_buffer, EAC_BUFFERSIZE, use_ed, eac);
											break;
										}

										if (di)
										{
											switch (ead->ed_Type)
											{
												case ST_FILE:
													di->files++;

													if (use_ed >= ED_SIZE64)
													{
														di->diskusage += ead->ed_Size64;
													}
													else
													{
														di->diskusage += (ULONG)ead->ed_Size;
													}

													if (MatchPatternNoCase(iconsparsepat, ead->ed_Name))
														di->icons++;

													break;

												case ST_ROOT:    /* well ST_ROOT shouldn't happen */
												case ST_USERDIR:
													di->dirs++;
													break;

												case ST_LINKDIR:
												case ST_LINKFILE:
												case ST_SOFTLINK:
													di->links++;
													break;
											}
										}

										/*
										 * If the filesystem didn't support the requested
										 * fields, we must provide them.
										 */
										if (use_ed < max_ed)
										{
											memcpy(&tmp_ead, ead, offsetof(struct ExAllData, ed_OwnerUID));

											if (max_ed >= ED_OWNER && use_ed < ED_OWNER)
											{
												tmp_ead.ed_OwnerUID = 0;
												tmp_ead.ed_OwnerGID = 0;
											}

											#if !USE_LEGACY
											if (max_ed >= ED_SIZE64 && use_ed < ED_SIZE64)
											{
												tmp_ead.ed_Size64 = (ULONG)ead->ed_Size;
											}
											#endif

											use_ead = &tmp_ead;
										}
										else
										{
											use_ead = ead;
										}


										if (MatchPatternNoCase(parsepat, ead->ed_Name) && (!type || (ead->ed_Type == type)))
										{
											D(EXDIR,bug("type matches or no type\n"));

											if (!(matchfunc(obj, path, use_ead, userdata)))
											{
												D(EXDIR,bug("func() failed\n"));
												/* XXX: should we fail more here ? */
												break;
											}
										}
										else
										{
											if (notmatchfunc && !(notmatchfunc(obj, path, use_ead, userdata)))
											{
												D(EXDIR,bug("notmatchfunc() failed\n"));
												/* XXX: should we fail more here ? */
												break;
											}
										}
									} while ((ead = ead->ed_Next));
								}
								while (more);
							}
							D(EXDIR,bug("freeing parsepats..\n"));
							free(parsepat);
						}
						D(EXDIR,bug("freeing ex_buffer..\n"));
						free(ex_buffer);
					}
					D(EXDIR,bug("FreeDosObject()..\n"));
					FreeDosObject(DOS_EXALLCONTROL, eac);
				}
			}
			D(EXDIR,bug("UnLock()ing..\n"));
			UnLock(l);
		}
		return (retval);
	}
	/* Is this correct? */
	return (TRUE);
}
