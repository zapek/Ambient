/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2007 Ambient Open Source Team
 * prefs_advanced.c, Copyright 2006 Christian Rosentreter 
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
 * $Id: prefs_advanced.c,v 1.17 2022/01/31 12:55:26 geit Exp $
 */

#include "ambient.h"

/* public */
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/asyncio.h>

#include <stddef.h>

/* private */
#include "prefs_advanced.h"
#include "dosnotify.h"
#include "file_io.h"
#include "ambient_cat.h"
#include "locale.h"
#include "prefs.h"
#include "str.h"
#include "copyright.h"



#define AP_FILE      PREFS_PATH ADVANCED_FILE
#define AP_FILE_BAK  AP_FILE ".bak"
#define AP_FILE_TEMP AP_FILE ",temp"

#define IOBUFFERSIZE 2048



struct pa_default {
	STRPTR pa_name;
	UBYTE  pa_type;
	UBYTE  pa_offset;
	BYTE   pa_priority;
	BOOL   pa_initonly;
	ULONG  pa_default;
	STRPTR pa_parameter;
};



#define OPTION(n,t,p,i,d,f) {#n, t, offsetof(struct advanced_prefs, n) , p, i, (ULONG)d, f}

/*  localized string default (only used for PA_STRING)
 */
#define STRINGIZENUMBERFROMDEFINE(s) #s
#define LSD(s) "LSD=" STRINGIZENUMBERFROMDEFINE(s)



/*
 *  How to add a new option?
 *
 *  - add an entry to the following table
 *    - name shouldn't be to long
 *    - set init to TRUE if the option needs a restart of ambient to produce
 *      an effect. This is only a *describing* flag for the user. If you depend on
 *      a fixed value on runtime (e.g. fixed string), make sure you copy the data
 *      as the data of the option *will* change when user edits it.
 *
 *  - document the option in distribution/prefs/advanced.conf,default
 *    - you also can add undocumented options (e.g. options only for
 *      very advanced users or developers), but add them after the
 *      "undocumented options" marker in the table
 *
 *  - supported parameter:
 *    LSD - locale string id which is used when default == NULL (PA_STRING)
 *    MIN - mininmal value, if user value is smaller it will be set to
 *          min (PA_INT, PA_UINT)
 *    MAX - maximal value, if user value is larger it will be set to max
 *          (PA_INT, PA_UINT)
 */

STATIC CONST struct pa_default pa_defaults[] = {
	/*     var                            type,      pri, init,  default, parameter       */  
	OPTION(sfx_boot,                      PA_STRING, 5,   TRUE,  NULL,    NULL            ), 
	OPTION(sfx_panelzip,                  PA_STRING, 5,   FALSE, NULL,    NULL            ),  
	OPTION(sfx_panelunzip,                PA_STRING, 5,   FALSE, NULL,    NULL            ),  
	OPTION(popuprename,                   PA_BOOL,   0,   FALSE, TRUE,    NULL            ),  
	OPTION(commodityfilter,               PA_STRING, 5,   FALSE, "#?",    NULL            ), 
	OPTION(exchangehotkey,                PA_STRING, 5,   TRUE,  "control alt help", NULL ), 
	OPTION(autosort,                      PA_BOOL,   0,   FALSE, TRUE,    NULL            ),  
	OPTION(mmkeyevents,                   PA_BOOL,   0,   FALSE, TRUE,    NULL            ), 
	OPTION(progresswindowdelay,           PA_UINT,   0,   FALSE, 0,       "MIN=0 MAX=2000"), 
	OPTION(progresswindowfocus,           PA_BOOL,   0,   FALSE, TRUE,    NULL            ),  
	OPTION(infowinautoversion,            PA_BOOL,   0,   FALSE, TRUE,    NULL            ),  
	OPTION(infowinautomd5sum,             PA_BOOL,   0,   FALSE, TRUE,    NULL            ),  
	OPTION(infowinautolimit,              PA_UINT,   0,   FALSE, 0,       NULL            ),  
	OPTION(nodatatypesmimename,           PA_BOOL,   0,   TRUE,  FALSE,   NULL            ),
	OPTION(panelspacersize,	              PA_UINT,   0,   FALSE, 12,      NULL            ),
	OPTION(donotputrefwindowtosleep,      PA_BOOL,   0,   FALSE, FALSE,   NULL            ),
#if USE_AVCODEC
	OPTION(videopreview,                  PA_BOOL,   0,   FALSE, TRUE,    NULL            ),
#endif
	OPTION(differentinactivewindowtitles, PA_BOOL,   0,   FALSE, FALSE,   NULL            ),
	OPTION(iconzoomslider,                PA_BOOL,   0,   FALSE, FALSE,   NULL            ),
	/* termination  */
	{NULL, 0, 0, 0, 0, 0, NULL}
};



/*   global symbols
 */
struct advanced_prefs *aprefs;

static struct MinList         pa_storage;
static struct SignalSemaphore pa_semaphore;
static APTR                   pa_notifyctx;




static BOOL prefs_advanced_defaults(void)
{
	CONST struct pa_default *t = pa_defaults;

	D(ADVANCEDPREFS,bug("apply defaults...\n"));

	while (t->pa_name)
	{
		struct pa_node *tnode;

		D(ADVANCEDPREFS,bug("add entry %s\n", t->pa_name));

		tnode = malloc(sizeof(struct pa_node));

		if (tnode)
		{
			memset(tnode, 0, sizeof(struct pa_node));

			tnode->n_node.ln_Name = t->pa_name;
			tnode->n_node.ln_Pri  = t->pa_priority;

			tnode->n_name         = t->pa_name;
			tnode->n_type         = t->pa_type;
			tnode->n_offset       = t->pa_offset;
			tnode->n_default      = t->pa_default;
			tnode->n_parameter    = t->pa_parameter;
			
			tnode->n_value        = 0;
			tnode->n_flags        = t->pa_initonly ? PAF_INITONLY : 0;


			D(ADVANCEDPREFS,bug("\tparameter list: %s\n", t->pa_parameter));

			switch (t->pa_type)
			{
				case PA_STRING:
					{
						STRPTR mys;

						/*  if we have LSD parameter we initialize the default value from
						 *  locale catalog
						 */
						if ( t->pa_parameter && (mys = strstr(t->pa_parameter, "LSD=")))
						{
							ASSERT(locale); /* Never, never land. */
							tnode->n_default = (ULONG) GSI( strtol(mys+4, NULL, 10) );
						}
					}
					break;

				case PA_UINT:
				case PA_INT:
				case PA_COLOUR:
				case PA_BOOL:
					/* nothing todo here */
					break;

				default:
					D(ADVANCEDPREFS,bug("\tUnknown type: 0x%lx\n", t->pa_type));

			} /* switch t->pa_type */

			tnode->n_value = tnode->n_default;
			D(ADVANCEDPREFS,bug("\tdefault value: 0x%lx\n", tnode->n_value));

			ENQUEUE  (&pa_storage, tnode);
		}
		else
		{
			PDB(("Not enough memory for advanced prefs option.\n"));
			return FALSE;

		} /* tnode */

		t++;

	} /* while t->pa_name */

	return TRUE;
}



static void prefs_advanced_aprefs_sync(struct pa_node *t)
{
	ULONG store = (ULONG)aprefs + t->n_offset;

	D(ADVANCEDPREFS,bug("\toffset: %3ld, dest addr: 0x%lx [%s]\n", t->n_offset, store, t->n_name ));

	switch (t->n_type)
	{
		case PA_BOOL:
			*((BOOL *)store) = (BOOL)t->n_value;
			break;

		case PA_COLOUR:
		case PA_INT:
		case PA_UINT:
		case PA_STRING:
			*((ULONG *)store) = t->n_value;
			break;

		default:
			D(ADVANCEDPREFS,bug("Unknown type 0x%lx\n", t->n_type));
	}
}



void prefs_advanced_refresh(BOOL init)
{
	struct AsyncFile *file;

	ObtainSemaphore(&pa_semaphore);

	D(ADVANCEDPREFS,bug("(re)load values from prefsfile...\n"));

	if ((file = OpenAsync(AP_FILE, MODE_READ, 8192)))
	{
		LONG res;
		UBYTE temp[1024]; /* should be enough for everything */

		while((res = ReadLineAsync(file, temp, 1024)) > 0)
		{
			STRPTR s = strpassws(temp);
			ULONG  l;

			D(ADVANCEDPREFS,bug(" [scan] %s", temp)); /* \n is in buffer, except last line ;) */

			strterminate(s);
			l = strlen(s);

			if (s[0] && (s[0] != ';') && (s[0] != '=')) /* comment, empty line or marker at start.. let's skip */
			{
				STRPTR m;

				if ((m = strchr(s,'=')))
				{
					if ( m+1 < s+l )
					{
						struct pa_node *tnode;

						/*  terminate variable name
						 */
						*m = '\0';
						strterminate(s);

						/*  get pointer to content
						 */
						m = strpassws(m+1);

						if ((tnode = (struct pa_node *)FindName((struct List *)&pa_storage, s)))
						{
							ULONG  cl        = strlen(m);
							BOOL   updated   = FALSE;
							STRPTR parameter = tnode->n_parameter;

							D(ADVANCEDPREFS,bug("*** found %s = %s\n", s, m));

							switch (tnode->n_type)
							{
								case PA_STRING:
									{
										/*   skip possible termination
										 */
										if (m[0] == '\"')
										{
											if (m[cl-1] == '\"')
											{
												m[cl-1] = '\0';
												cl--;
											}
											else
											{
												D(ADVANCEDPREFS,bug("no string termination found, leading marker still ignored\n"));
											}

											m++;
										}

										/*  only set update flag if new string is different from default string (automatically 
										 *  gets set back to default later when it resets not updated variables (quite tricky, but 
										 *  saves a few lines code ;)
										 */
										if ((tnode->n_default ? strcmp(m, (STRPTR)tnode->n_default) : 1)) 
										{
											STRPTR t = malloc(cl+1);

											if (t)
											{
												STRPTR temp = (STRPTR)tnode->n_value;

												strcpy(t, m);

												tnode->n_value = (ULONG)t;

												/*  free memory of old string if required, do not free 
										 		 *  when it's the default string
										 		 */
												if (temp && (tnode->n_flags & PAF_USERDEFINED))
												{
													ASSERT(temp != (STRPTR)tnode->n_default); 
													free(temp);
												}

												updated = TRUE;
											}
											else
											{
												PDB(("Couldn't allocate memory for string\n"));
											}
										}
										else
										{
											D(ADVANCEDPREFS,bug("string == default string\n"));
										}

										D(ADVANCEDPREFS,bug("STRING: %s\n", (STRPTR)tnode->n_value ));
									}
									break;

								case PA_BOOL:
									{
										ULONG v = FALSE;

										if ((m[0] == '1' && m[1] == '\0') ||
											(!stricmp(m, "on"  )) ||
											(!stricmp(m, "true")) )
										{
											v = TRUE;
										}

										tnode->n_value = v;

										updated = TRUE;

										D(ADVANCEDPREFS,bug("BOOL: %s\n", v ? "TRUE" : "FALSE"));
									}
									break;

								case PA_INT:
									{
										STRPTR mys;
										LONG v1, v2;

										v1 = strtol(m, NULL, 0);

										if (parameter)
										{
											if ((mys = strstr(parameter, "MIN=")) && (v1 < (v2 = strtol(mys+4, NULL, 10))) )
											{
												v1 = v2;
											}

											if ((mys = strstr(parameter, "MAX=")) && (v1 > (v2 = strtol(mys+4, NULL, 10))) )
											{
												v1 = v2;
											}
										}

										tnode ->n_value = (ULONG)v1;

										updated = TRUE;

										D(ADVANCEDPREFS,bug("INT: %ld\n", v1 ));
									}
									break;

								case PA_UINT:
									{
										STRPTR mys;
										ULONG v1, v2;

										v1 = strtoul(m, NULL, 0);

										if (parameter)
										{
											if ((mys = strstr(parameter, "MIN=")) && (v1 < (v2 = strtoul(mys+4, NULL, 10))) )
											{
												v1 = v2;
											}

											if ((mys = strstr(parameter, "MAX=")) && (v1 > (v2 = strtoul(mys+4, NULL, 10))) )
											{
												v1 = v2;
											}
										}

										tnode->n_value = v1;

										updated = TRUE;

										D(ADVANCEDPREFS,bug("UINT: %lu\n", v1 ));
									}
									break;

								case PA_COLOUR:
									{
										if (!strncmp("0x", m, 2)) /* HEX number? */
										{
											tnode->n_value = strtoul(m + 2, NULL, 16);

											updated = TRUE;

											D(ADVANCEDPREFS,bug("COLOUR: 0x%lx\n", tnode->n_value ));
										}
										else
										{
											D(ADVANCEDPREFS,bug("COLOUR variable not in HEX format.\n"));
										}
									}
									break;

								default:
									{ D(ADVANCEDPREFS,bug("  unknown type.. wtf.?\n")); }

							} /* switch type */

							if (updated)
							{
								D(ADVANCEDPREFS,bug("variable %s updated\n", s));

								tnode->n_flags |= PAF_UPDATED;
								
								if (tnode->n_value != tnode->n_default)
								{
									tnode->n_flags |= PAF_USERDEFINED;
								}
							}
						}
						else
						{
							D(ADVANCEDPREFS,bug("  unknown variable\n"));
						}
					}
					else
					{
						D(ADVANCEDPREFS,bug("  no value given\n"));
					}
				}
				else
				{
					D(ADVANCEDPREFS,bug("  no '=' marker\n"));
				}
			}
			else
			{
				D(ADVANCEDPREFS,bug("  skip line (comment, empty or marker at start)\n"));
			}
		}

		CloseAsync(file);
	}
	else
	{
		D(ADVANCEDPREFS,bug("Couldn't open " AP_FILE));
	}

	/*  reset possible deactivated on-the-fly variables to default value
	 */
	if (!init)
	{
		struct pa_node *t;

		D(ADVANCEDPREFS,bug("Reset deactivated on-the-fly variables\n"));

		ITERATELIST(t, &pa_storage)
		{
			if ( !(t->n_flags & PAF_UPDATED) )
			{
				D(ADVANCEDPREFS,bug("\t%s\n",t->n_name));

				switch (t->n_type)
				{
					case PA_STRING:
						if (t->n_value && (t->n_flags & PAF_USERDEFINED))
						{
							ASSERT(t->n_value != t->n_default); 
							free((APTR)t->n_value);
						}
						break;

					case PA_INT:
					case PA_UINT:
					case PA_COLOUR:
					case PA_BOOL:
						/* nothing todo here */
						break;
				}
				
				t->n_value  = t->n_default; /* reset to default value */
				t->n_flags &= ~PAF_USERDEFINED;
			}

			t->n_flags &= ~PAF_UPDATED;
		}
	}


	/*  sync with prefs structure for quick access
	 */
	{
		struct pa_node *t;

		D(ADVANCEDPREFS,bug("sync structure with list values.\nbase address: 0x%lx\n", (ULONG)aprefs));

		ITERATELIST(t, &pa_storage)
		{
			prefs_advanced_aprefs_sync(t);
		}
	}

	ReleaseSemaphore(&pa_semaphore);
}



ULONG prefs_advanced_init(void)
{
	ASSERT(AsyncIOBase);

	D(ADVANCEDPREFS,bug("initialize...\n"));

	InitSemaphore(&pa_semaphore);

	if ((aprefs = (struct advanced_prefs *)malloc(sizeof(struct advanced_prefs))))
	{
		memset(aprefs, 0, sizeof(struct advanced_prefs));

		NEWLIST  (&pa_storage);   /* don't touch those spaces, or MorphED/ GoldEd will fuck up build */

		if (prefs_advanced_defaults())
		{
			D(ADVANCEDPREFS,bug("init successed\n"));

			prefs_advanced_refresh(TRUE);

			/* dos notification */
			if (!pa_notifyctx)
			{
				D(ADVANCEDPREFS,bug("add notify for " AP_FILE "...\n"));
				pa_notifyctx = dosnotify_start( AP_FILE , 0 );
			}

			return TRUE;
		}
	}

	D(ADVANCEDPREFS,bug("init failed\n"));

	return FALSE;
}



void prefs_advanced_cleanup(void)
{
	struct pa_node *temp, *nexttemp;

	D(ADVANCEDPREFS,bug("cleanup\n"));

	if (pa_notifyctx)
	{
		dosnotify_stop( pa_notifyctx );
	}

	ITERATELISTSAFE  (temp, nexttemp, &pa_storage)  
	{
		if ((temp->n_type == PA_STRING) && temp->n_value && (temp->n_flags & PAF_USERDEFINED))
		{
			ASSERT(temp->n_value != temp->n_default);
			free((APTR)temp->n_value);
		}

		free(temp);
	}

	if (aprefs)
	{
		free(aprefs);
	}
}



ULONG prefs_advanced_getvalue(CONST_STRPTR name)
{
	struct pa_node *tnode;
	ULONG rv = (ULONG)NULL;

	ObtainSemaphoreShared(&pa_semaphore);

	D(ADVANCEDPREFS,bug("Lockup variable %s\n", name));

	if ((tnode = (struct pa_node *)FindName((struct List *)&pa_storage, name)))
	{
		rv = tnode->n_value;

		D(ADVANCEDPREFS,bug("Got value 0x%lx\n", (ULONG)rv));
	}
	else
	{
		D(ADVANCEDPREFS,bug("Request to variable %s failed.\n", name));
	}

	ReleaseSemaphore(&pa_semaphore);

	return rv;
}


BOOL prefs_advanced_setvalue(struct pa_node *tnode, CONST ULONG v)
{
	BOOL rc = TRUE;

	ObtainSemaphore(&pa_semaphore);

	switch(tnode->n_type)
	{
		case PA_STRING:
			{
				STRPTR s    = (STRPTR)v;
				STRPTR t    = (STRPTR)tnode->n_default; 
				STRPTR temp = (STRPTR)tnode->n_value;

				if (s)
				{
					if ((tnode->n_default ? strcmp(s, (STRPTR)tnode->n_default) : 1))
					{
						if ((t = malloc(strlen(s)+1)))
						{
							strcpy(t, s);
						}
						else
						{
							/*  if allocation fails it will set the variable to NULL... but w/o free memory 
						 	 *  it prolly doesn't matter anyway ;) 
						 	 */
							rc = FALSE;
						}
					}
				}

				/*  free memory of old string if required, do not free 
				 *  when it's the default string
				 */
				if (temp && (tnode->n_flags & PAF_USERDEFINED))
				{
					ASSERT(temp != (STRPTR)tnode->n_default); 
					free(temp);
				}

				tnode->n_value = (ULONG)t;
			}
			break;

		case PA_UINT:
		case PA_INT:
			/*
			 *   XXX: implement MIN= and MAX= checking here.  -- tokai
			 */
			/*  fall through for now... 
			 */
		case PA_COLOUR:
		case PA_BOOL:
			tnode->n_value = v;
			break;

		default:
			DB(("Unknown type\n"));
	}

	if (tnode->n_value != tnode->n_default)
	{
		tnode->n_flags |= PAF_USERDEFINED;
	}
	else
	{
		tnode->n_flags &= !PAF_USERDEFINED;
	}

	/*  sync with prefs structure for quick access
 	 */
	prefs_advanced_aprefs_sync(tnode);


	D(ADVANCEDPREFS,bug("\tvariable update done\n"));
	
	ReleaseSemaphore(&pa_semaphore);

	return rc;
}


BOOL prefs_advanced_namesetvalue(CONST_STRPTR name, CONST ULONG v)
{
	struct pa_node *tnode;
	BOOL rc = FALSE;

	ObtainSemaphore(&pa_semaphore);

	D(ADVANCEDPREFS,bug("Lockup variable %s for storing value 0x%lx\n", name, (ULONG)v));

	if ((tnode = (struct pa_node *)FindName((struct List *)&pa_storage, name)))
	{
		rc = prefs_advanced_setvalue(tnode, v);
	}
	else
	{
		D(ADVANCEDPREFS,bug("Request to variable failed.\n"));
	}

	ReleaseSemaphore(&pa_semaphore);

	return rc;
}


BOOL prefs_advanced_resetvalue(struct pa_node *tnode)
{
	/* no need to obtain semaphore here */

	return prefs_advanced_setvalue(tnode, tnode->n_default); /* whoops, lets hope this will do ;) */
}


#define BUFFER_SIZE 32

BOOL prefs_advanced_save(void)
{
	APTR fh;
	BOOL rc = FALSE;

	ObtainSemaphoreShared(&pa_semaphore);
	
	if ((fh = file_open(AP_FILE_TEMP, MODE_NEWFILE)))
	{
		static CONST TEXT header[] =
			";\n"
			";  Ambient " LVERTAG "\n"
			";  Advanced Config File\n"
			";\n"
			";  This file was created automatically. For a description of all options\n"
			";  check out MOSSYS:Ambient/prefs/Advanced.conf,default.\n" 
			";\n\n";

		if (file_write(fh, header, sizeof(header)-1))
		{
			struct pa_node *t;
			BOOL  succ = TRUE;      /* if there is nothing to iterate don't fail */
			UBYTE buf[BUFFER_SIZE]; /* only used for numerical values            */

			ITERATELIST(t, &pa_storage)
			{
				STRPTR bptr  = buf;
				BOOL   quote = FALSE;
				
				succ = FALSE; 

				if (!(t->n_flags & PAF_USERDEFINED))
				{
					if (!file_write(fh, "; ", 2))
						break;
				}

				/*  name
				 */
				if (!file_write(fh, t->n_name, strlen(t->n_name)))
					break;

				if (!file_write(fh, " = ", 3)) /* XXX: put more spaces after name to align values properly? */
					break;


				/*  value
				 */
				if (t->n_flags & PAF_USERDEFINED)
				{
					switch (t->n_type)
					{
						case PA_STRING: 
							if (t->n_value)
							{
								bptr  = (STRPTR)t->n_value;
								quote = TRUE; 
							}
							else
							{
								bptr  = "\"\"";
							}
							break;

						case PA_COLOUR:  
							snprintf(buf, BUFFER_SIZE, "0x%08lx", t->n_value); 
							break;

						case PA_INT:    
							snprintf(buf, BUFFER_SIZE, "%ld", t->n_value); 
							break;

						case PA_UINT:   
							snprintf(buf, BUFFER_SIZE, "%lu", t->n_value); 
							break;

						case PA_BOOL:   
							snprintf(buf, BUFFER_SIZE, "%s", t->n_value   ? "true" : "false"); 
							break;

						default: 
							D(ADVANCEDPREFS,bug("Unknown type.\n"));
					}

					
					if (quote && !file_write(fh, "\"", 1))
						break;

					if (!file_write(fh, bptr, strlen(bptr)))		
						break;

					if (quote && !file_write(fh, "\"", 1))
						break;		
				}

				/*  last but not least the final line feed
				 */
				if (!file_write(fh, "\n", 1))
					break;	

				succ = TRUE;
			}

			rc = succ;
		}
		
		file_close(fh);
	}
	
	ReleaseSemaphore(&pa_semaphore);

	/*  funny file shuffle
	 */
	if (rc)
	{
		DeleteFile(AP_FILE_BAK); 

		D(ADVANCEDPREFS,bug("remove notify for " AP_FILE "...\n"));
		if (pa_notifyctx) /* be padantic */
		{
			dosnotify_stop(pa_notifyctx);
			pa_notifyctx = NULL;
		}

		Rename(AP_FILE, AP_FILE_BAK);

		rc = Rename(AP_FILE_TEMP, AP_FILE);

		D(ADVANCEDPREFS,bug("add notify for " AP_FILE "...\n"));
		pa_notifyctx = dosnotify_start(AP_FILE, 0);
	}

	if (!rc)
	{
		PDB(("Failed to save Advanced.conf.\n"));
	}

	return rc;
}






struct MinList * prefs_advanced_lockstorage(void)
{
	ObtainSemaphoreShared(&pa_semaphore);

	return &pa_storage;
}



void prefs_advanced_unlockstorage(void)
{
	ReleaseSemaphore(&pa_semaphore);
}
