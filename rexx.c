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
 * $Id: rexx.c,v 1.19 2026/05/16 06:53:06 geit Exp $
 */

#include "ambient.h"

#if USE_REXX

/* public */
#include <exec/lists.h>
#include <rexx/storage.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/rexxsyslib.h>
#include <proto/dos.h>

/* private */
#include "rexx.h"
#include "command.h"
#include "threads.h"
#include "mui_func.h"
#include "file_func.h"
#include "template.h"
#include "wbstart.h"
#include "readargs.h"
#include "methodstack.h"
#include "appclass.h"
#include "prefs.h"
#include "name.h"
#include "locale.h"

static struct MsgPort rexxport;
static ULONG outmsgs; /* number of outstanding messages */

static ULONG rxid;
static struct MinList rxlist;

static APTR rxpool;

struct rxnode {
	struct MinNode n;
	ULONG id;
	struct RexxMsg *rxmsg;
	ULONG rc;
	STRPTR result;
};

ULONG rexxsig;

#define REXX_RETURN_ERROR ((struct RexxMsg *)-1L)


static struct RexxMsg *rx_get_msg(void)
{
	struct RexxMsg *rxmsg;

	if ( (rxmsg = (struct RexxMsg *)GetMsg(&rexxport)) )
	{
		if (rxmsg->rm_Node.mn_Node.ln_Type == NT_REPLYMSG)
		{
			ULONG res = FALSE;

			/*
			 * That's a reply to one of our commands.
			 */
			if (rxmsg->rm_Result1)
			{
				res = TRUE;
			}

			DeleteArgstring(rxmsg->rm_Args[0]);
			DeleteRexxMsg(rxmsg);
			outmsgs--;

			if (res)
			{
				rxmsg = REXX_RETURN_ERROR;
			}
			else
			{
				rxmsg = NULL;
			}
		}
	}
	return (rxmsg);
}


static void rx_reply_msg(struct RexxMsg *rxmsg, STRPTR rxstr, LONG error)
{
	if (rxmsg && rxmsg != REXX_RETURN_ERROR)
	{
		rxmsg->rm_Result2 = 0;

		if (!(rxmsg->rm_Result1 = error))
		{
			/* if there's no error, return the string, if any */
			if ((rxmsg->rm_Action & RXFF_RESULT) && (rxstr))
			{
				rxmsg->rm_Result2 = (LONG)CreateArgstring(rxstr, (LONG)strlen(rxstr));
			}
		}
		/* reply to ARexx */
		ReplyMsg(&rxmsg->rm_Node);
	}
}


static ULONG rx_set_last_error(struct RexxMsg *rxmsg, STRPTR errorstr)
{
	if (rxmsg && CheckRexxMsg(rxmsg))
	{
		if (!SetRexxVar(rxmsg, REXXPORT ".LASTERROR", errorstr, (LONG)strlen(errorstr)))
		{
			return (TRUE);
		}
	}
	return (FALSE);
}


#if 0
static ULONG rx_send_msg(STRPTR rxstr, ULONG stringfile)
{
	ULONG retval = FALSE;

	if (rxstr)
	{
		struct RexxMsg *rxmsg;

		if (rxmsg = CreateRexxMsg(&rexxport, REXXEXT, rexxport.mp_Node.ln_Name))
		{
			rxmsg->rm_Action = RXCOMM | (stringfile ? (1L << RXFB_STRING) : 0);

			if (rxmsg->rm_Args[0] = CreateArgstring(rxstr, (LONG)strlen(rxstr)))
			{
				struct MsgPort *arexxport;

				Forbid();
				if ((arexxport = FindPort(RXSDIR)))
				{
					PutMsg(arexxport, (struct Message *)rxmsg);
					outmsgs++;
					retval = TRUE;
				}
				Permit();

				if (!arexxport)
				{
					DeleteArgstring(rxmsg->rm_Args[0]);
					DeleteRexxMsg(rxmsg);
				}
			}
			else
			{
				DeleteRexxMsg(rxmsg);
			}
		}
	}
	return (retval);
}
#endif


/*
 * Rexx ID stuff
 */

/*
 * The following is used so we don't
 * have to pass the message to MM_Application_DoRexx.
 * Mostly for rx_setrexxvar_id().
 */

static struct rxnode current_rxn;


static ULONG rx_get_id(void)
{
	return (++rxid);
}


static ULONG rx_register_id(ULONG id, struct RexxMsg *rxmsg)
{
	struct rxnode *rxn;

	if (!id)
	{
		return (TRUE); /* that can only happen for internal stuff */
	}

	if ( (rxn = AllocPooled(rxpool, sizeof(*rxn))) )
	{
		rxn->id = id;
		rxn->rxmsg = rxmsg;
		rxn->rc = 0;
		rxn->result = NULL;

		ADDTAIL(&rxlist, rxn);

		return (TRUE);
	}
	return (FALSE);
}


static struct rxnode *rx_find_rxnode(ULONG id)
{
	if (id)
	{
		if (id == rxid)
		{
			return (&current_rxn);
		}
		else
		{
			struct rxnode *rxn;

			ITERATELIST(rxn, &rxlist)
			{
				if (rxn->id == id)
				{
					return (rxn);
				}
			}
		}
	}
	return (NULL);
}

void rx_set_result(ULONG id, LONG val, CONST_STRPTR str)
{
	struct rxnode *rxn;

	if ( (rxn = rx_find_rxnode(id)) )
	{
		rxn->rc = val;

		if (str)
		{
			if ( (rxn->result = AllocVecPooled(rxpool, strlen(str) + 1)) )
			{
				strcpy(rxn->result, str);
			}
			/* XXX */
		}
	}
	/* that's an internal message then */
}


void rx_reply_id(ULONG id)
{
	struct rxnode *rxn;

	if ( (rxn = rx_find_rxnode(id)) )
	{
		rx_reply_msg(rxn->rxmsg, rxn->result, rxn->rc);

		if(rxn != &current_rxn)
		{
			REMOVE(rxn);

			if (rxn->result)
			{
				FreeVecPooled(rxpool, rxn->result);
			}
			FreePooled(rxpool, rxn, sizeof(*rxn));
		}
	}
	/* that's an internal message then */
}


void rx_setrexxvar_id(ULONG id, CONST_STRPTR name, CONST_STRPTR val)
{
	struct rxnode *rxn;

	ASSERT(name);

	if ( (rxn = rx_find_rxnode(id)) )
	{
		if (CheckRexxMsg(rxn->rxmsg))
		{
			#ifdef DEBUG
			/*
			 * ARexx is stupid and needs vars to be
			 * in capital otherwise it doesn't work
			 * at all. How braindead can that be ?
			 */
			{
				CONST_STRPTR p = name;

				while (*p)
				{
					if (isalpha(*p) && islower(*p))
					{
						PDB(("lowercase detected in arexx var string <%s>\n", name));
						break;
					}
					p++;
				}
			}
			#endif

			D(REXX,bug("setting var <%s> value <%s> (msg: %p)\n", name, val, rxn->rxmsg));
			if (val)
			{
				SetRexxVar(rxn->rxmsg, name, val, strlen(val)); /* XXX: retval ? */
			}
			else
			{
				/*
				 * We need to do that otherwise a stem would look
				 * wrong (STEM.NAME_OF_VAR).
				 */
				SetRexxVar(rxn->rxmsg, name, "", 0); /* XXX: retval ? */
			}
		}
		else
		{
			PDB(("msg not from arexx..\n"));
		}
	}
	else
	{
		PDB(("argh! impossible but yet it happend, glurps\n"));
	}
}


ULONG rexx_init(void)
{
	if (RexxSysBase) /* that one is opened from init_libs() */
	{
		if ( (rxpool = CreatePool(MEMF_ANY | MEMF_SEM_PROTECTED, 2048, 1024)) )
		{
			if ( (BYTE) (rexxport.mp_SigBit = AllocSignal(-1)) != -1 )
			{
				rexxport.mp_Node.ln_Type = NT_MSGPORT;
				rexxport.mp_Node.ln_Name = REXXPORT;
				rexxport.mp_Flags        = PA_SIGNAL;
				rexxport.mp_SigTask      = FindTask(NULL);
				NEWLIST(&rexxport.mp_MsgList);

				Forbid();
				if (!FindPort(REXXPORT))
				{
					AddPort(&rexxport);
					Permit();

					rexxsig = 1L << rexxport.mp_SigBit;

					NEWLIST(&rxlist);

					return (TRUE);
				}
				Permit();

				FreeSignal(rexxport.mp_SigBit);
				rexxport.mp_Node.ln_Type = 0;
			}
			DeletePool(rxpool);
		}
		return (FALSE);
	}
	return (TRUE); /* arexx is optional */
}


void rexx_cleanup(void)
{
	if (rexxport.mp_Node.ln_Type)
	{
		struct RexxMsg *rxmsg;

		RemPort(&rexxport);

		while (outmsgs)
		{
			WaitPort(&rexxport);
			while ( (rxmsg = rx_get_msg()) )
			{
				if (rxmsg != REXX_RETURN_ERROR)
				{
					rx_set_last_error(rxmsg, "99: Port closed, go away");
					rx_reply_msg(rxmsg, NULL, 100);
				}
			}
		}

		while ( (rxmsg = rx_get_msg()) )
		{
			rx_set_last_error(rxmsg, "99: Port closed, go away");
			rx_reply_msg(rxmsg, NULL, 100);
		}

		FreeSignal(rexxport.mp_SigBit);
		DeletePool(rxpool);
	}
}


/* XXX: check DoRexx's return and if it's not ok, reply immediately with the error */
static inline ULONG send_internal_command(APTR obj, STRPTR str, ULONG internal, LONG *retval, STRPTR *errstr, ULONG id, APTR *objlist, ULONG sync)
{
	return (DoMethod(app, MM_Application_DoRexx, internal, obj, str, retval, errstr, id, objlist, sync));
}

static ULONG push_internal_command(APTR obj, STRPTR str, ULONG internal, LONG *retval, STRPTR *errstr, ULONG id, APTR *objlist, ULONG sync)
{
	/*
	 * We can't push the method as it would result in execution on main thread which we don't want to happen.
	 * Function we are calling have to handle a case when it's called from different task or return an error.
	 */

	ULONG msg[ sizeof( struct MP_Application_DoRexx ) / sizeof( ULONG ) ];

	msg[ 0 ] = MM_Application_DoRexx;
	msg[ 1 ] = internal;
	msg[ 2 ] = (ULONG)obj;
	msg[ 3 ] = (ULONG)str;
	msg[ 4 ] = (ULONG)retval;
	msg[ 5 ] = (ULONG)errstr;
	msg[ 6 ] = id;
	msg[ 7 ] = (ULONG)objlist;
	msg[ 8 ] = sync;
	DB(("Finalize!\n"));

	application_dorexx( OCLASS(app), app, ( struct MP_Application_DoRexx * )msg );

	return retval ? *retval : 0;
}


struct RDArgs *readargsstring(CONST_STRPTR source, CONST_STRPTR templ, ULONG *array)
{
	struct RDArgs *result = NULL;

	struct RDArgs *rdasrc;
	int len;
	STRPTR alloc = NULL;

	if ((rdasrc = AllocDosObject(DOS_RDARGS,NULL)))
	{
		if (!source || !*source) source = "\n";

		len = strlen(source);

		if (source[len - 1] != '\n')
		{
			if ( (alloc = malloc(len + 2)) )
			{
				strcpy(alloc, source);
				alloc[len] = '\n';
				alloc[++len] = '\0';
				source = alloc;
			}
			/* XXX: hm.. */
		}

		rdasrc->RDA_Source.CS_Buffer = (STRPTR)source;
		rdasrc->RDA_Source.CS_Length = len;
		rdasrc->RDA_Source.CS_CurChr = 0;

		D(RDARGS,bug("string to parse: <%s>, template: <%s>, array: %p\n", source, templ, array)); /* XXX: source gets trashed after readargs(), at least when it's left=foobar, etc.. */

		result = readargs((STRPTR) templ, (LONG *)array, rdasrc);

		if (alloc)
		{
			free(alloc);
		}

		if (!result)
		{
			FreeDosObject(DOS_RDARGS, rdasrc);
		}
	}
	return (result);
}


void freeargsstring(struct RDArgs *rda)
{
	D(RDARGS,bug("freeing args %p\n", rda));

	if (rda)
	{
		freeargs(rda);
		FreeDosObject(DOS_RDARGS, rda);
	}
}


void rexx_handle(void)
{
	struct RexxMsg *rxmsg;

	while ( (rxmsg = rx_get_msg()) ) /* XXX: perhaps we shouldn't do that in a loop and find a smarter way (like resetting the signal ourself for everything, including ipc, etc...) */
	{
		D(REXX,bug("got msg %p\n", rxmsg));

		switch ((int)rxmsg)
		{
			case (int)REXX_RETURN_ERROR:
				/* XXX: and error came back.. analyze this */
				break;

			default:
				{
					ULONG id;
					LONG retval;
					STRPTR retstr = NULL;

					id = rx_get_id();

					current_rxn.id = id;
					current_rxn.rxmsg = rxmsg;
					current_rxn.rc = 0;
					current_rxn.result = NULL;

					if (send_internal_command(NULL, rxmsg->rm_Args[0], FALSE, &retval, &retstr, id, NULL, FALSE))
					{
						/*
						 * We can reply immediately.
						 */
						D(REXX,bug("id: %ld, rc: %ld, retstr: <%s>\n", id, retval, retstr ? retstr : (STRPTR)"none"));
						rx_reply_msg(rxmsg, retstr, retval);
					}
					else
					{
						/*
						 * We can't so fill in the ID with more infos.
						 */
						D(REXX,bug("delayed reply, registering id %ld..\n", id));
						rx_register_id(id, rxmsg);
					}

					if (retstr)
					{
						free(retstr);
					}
				}
				break;
		}
	}

}


struct RX_WBArgs {
	STRPTR * items;
};

#if USE_RECOGTRANSLATION
#define PARSEDBUF_SIZEOF 0x2000   /* This is a temp buffer, so give operation some space. */
#else
#define PARSEDBUF_SIZEOF 0
#endif
/*
 * That one is called internally by Ambient (context menu, etc..)
 */
void execute_command(APTR obj, ULONG type, CONST_STRPTR str, APTR *array)
{
	//TEXT str_parsed[REXX_MAXLENGTH];
	STRPTR str_parsed = NULL;	 
	ULONG sync = type & AC_SYNC_MASK;
	type = type & AC_TYPE_MASK;

	DB(("Execute_Command: %s\n", sync ? "Synchronous" : "Asynchronous" ));

	/* XXX: we might have to find out the SRC lister and apply the action to it */

	if (str && str[0])
	{
		ULONG len = 2 * strlen(str) + PARSEDBUF_SIZEOF + 1;   /* old was 2* strlen(str) + 1 */

		str_parsed = (STRPTR) malloc(len);

		if(str_parsed)
		{

			template_expand(str, str_parsed, len,
				'p', "prout",
				//'u', obj_url,
				//'l', obj_link,
				//'w', obj_window
				NULL
			);

#if USE_RECOGTRANSLATION
/* if this causes trouble due to the "${}" pattern, we can change the pattern used in locale.c */
			locale_translationfillpattern( str_parsed, len );
#endif
			/*
			 * Remove ticks, if any.
			 */
			len = strlen(str_parsed);

			if (str_parsed[0] == '\'' && str[1] && str_parsed[len - 1] == '\'')
			{
				memmove(&str_parsed[0], &str_parsed[1], len);
				str_parsed[len - 2] = '\0'; /* kill final backtick */
			}

			DB(("Execute command based on type %x\n",type));

			switch (type)
			{
				case AC_INTERNAL:
					D(REXX,bug("sending internal command <%s>..\n", str_parsed));
					PDB(("Is %sMainTask\n", IS_MAINTASK ? "" : "not "));
					if ( IS_MAINTASK )
						send_internal_command(obj, str_parsed, TRUE, NULL, NULL, 0, array, sync); /* XXX: reply immediately if possible? hm, probably not */
					else
						push_internal_command(obj, str_parsed, TRUE, NULL, NULL, 0, array, sync); /* XXX:  */
					break;

				case AC_AMIGADOS:
					{
						/*
						 * TODO: Allow input and output to be passed here. Would allow to
						 * have single output for multiple calls.
						 */

						BPTR output = (BPTR)NULL;

						if (!_conf(cli_device) || !_conf(cli_device)[0] || !(output = Open(_conf(cli_device), MODE_NEWFILE)))
						{
							output = Open("NIL:", MODE_NEWFILE); /* XXX: erk, close input and output if we fail later.. */
						}

						if ( output )
						{
							BPTR input;
							struct MsgPort *old;

							old = SetConsoleTask(((struct FileHandle *)BADDR(output))->fh_Type);
							input = Open("*", MODE_OLDFILE); /* XXX: check retcode.. */
							SetConsoleTask(old);

							D(REXX,bug("executing <%s> in AmigaDOS mode..\n", str_parsed));
							if (systemtags(str_parsed, /* XXX: should we add quotes ? think so. is that async ? */
								SYS_Asynch, sync ? FALSE : TRUE,
								SYS_Input, input,
								SYS_Output, output,
								NP_Priority, 0,
								TAG_DONE) )
							{
								/*
								 * In sync mode we are responsible for closing in/out.
								 */

								if ( sync )
								{
									if ( input )
										Close( input );
									Close( output );
								}
							}
							else
							{
								if ( input )
									Close( input );
								Close( output );
							}
						}

					}
					break;

				case AC_WORKBENCH:
					{
						struct MinList * arglist;

						D(REXX,bug("executing <%s> in WB mode..\n", str));

						arglist = (struct MinList *) malloc(sizeof(*arglist));

						if(arglist)
						{
							struct RDArgs *rda = NULL;
							ULONG * array = (ULONG *) malloc(sizeof(ULONG));        ;

							NEWLIST(arglist);

							if(array)
							{
								memset(array, 0, sizeof(ULONG));

								if( (rda = readargsstring(str, "ARGS/M", array)) )
								{
									struct RX_WBArgs * arg = (struct RX_WBArgs *) array;
									STRPTR * list = arg->items;
									int i=0;

									while(list[i])
									{
										struct wbsnode *wbsn = (struct wbsnode *) malloc(sizeof(*wbsn)+strlen(list[i])+1);
										if(wbsn)
										{
											if(i>0)
											{
												strcpy(wbsn->arg, list[i]);
												ADDTAIL(arglist, wbsn);
											}
										}
										i++;
									}

									/* XXX: wrong! not async.. */
									wbstart(list[0],
										WBSTARTTAG_Stack, 8192,
										WBSTARTTAG_Argument_List, arglist,  /* arglist is freed by wbstart */
									TAG_DONE);

									freeargsstring(rda);
								}
								free(array);
							}

							if(!rda || !array)
							{
								free(arglist);
							}
						}
					}
					break;

				case AC_SCRIPT:
					{
						TEXT buf[REXX_MAXLENGTH + 10]; /* "Execute " */

						sprintf(buf, "execute %s\n", str_parsed);
						D(REXX,bug("executing DOS script <%s>..\n", buf));
						/* XXX: wrong! not async */
						Execute(buf, (BPTR)NULL, (BPTR)NULL); /* XXX: input/output.. hm. should use execute_async() perhaps */
					}
					break;

				case AC_AREXX:
					D(REXX,bug("executing ARexx script <%s>\n", str_parsed));
					/* XXX: sendrxmsg(str_parsed, &rexxport, rexxext), something like that */
					break;
			}

			free(str_parsed);
		}
	}
}


void execute_command_objarray(APTR obj, ULONG type, CONST_STRPTR s)
{
	APTR array = (APTR)DoMethod(obj, MM_View_PickSelected);

	if (array)
	{
		execute_command(obj, type, s, array);
		FreeVecTaskPooled(array);
	}
}

#endif
