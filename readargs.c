/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006 Ambient Open Source Team
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
 * $Id: readargs.c,v 1.7 2019/09/19 22:06:04 piru Exp $
 */

#include "ambient.h"

#if USE_INTERNAL_READARGS

/* public */
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <dos/dosasl.h>
#include <proto/utility.h>

/* private */
#include "readargs.h"


/*
 * WARNING
 *
 * This thing is horribly broken. For example /T doesn't work at all.
 * Would require total rewrite. - piru
 *
 */

struct DAList
{
	STRPTR *ArgBuf;
	UBYTE *StrBuf;
	STRPTR *MultVec;
};


/*
 * That ReadArgs() implementation comes from MUI
 * which took it from somewhere else.. but none
 * remembers where from. I think it's PD.
 */

static LONG strtolong(STRPTR string,LONG *value)
{
	LONG sign = 0, v = 0;
	STRPTR s = string;

	// Skip leading whitespace characters
	if (*s == ' ' || *s == '\t') s++;

	// Swallow sign
	if (*s == '+' || *s == '-') sign = *s++;

	// If there is no number return an error.
	if (*s < '0' || *s > '9')
	{
		*value = 0;
		return (-1);
	}

	// Calculate result
	do
	{
		v = v * 10 + *s++ - '0';
	} while (*s >= '0' && *s <= '9');

	// Negative?
	if (sign == '-') v = -v;

	// All done.
	*value = v;
	return (s - string);
}


static LONG findarg(CONST_STRPTR templ, CONST_STRPTR keyword)
{
	LONG count = 0;
	CONST_STRPTR key;

	for (;;)
	{
		//  Compare key to template
		key = keyword;
		for (;;)
		{
			UBYTE lkey;

			// If the keyword has ended check the template
			if (!*key)
			{
				if (!*templ || *templ== '=' || *templ == '/' || *templ == ',')
				{
					return (count); // The template has ended, too. Return count.
				}
				break; // The template isn't finished. Stop comparison.
			}
			// If the two differ stop comparison.
			lkey = ToLower(*key);
			if(lkey != ToLower(*templ))
				break;
			// Go to next character
			key++;
			templ++;
		}
		// Find next keyword in template
		for (;;)
		{
			if (!*templ)
			{
				return (-1);
			}
			if (*templ == '=')
			{
				// Alias found
				templ++;
				break;
			}
			if (*templ == ',')
			{
				// Next item found
				templ++;
				count++;
				break;
			}
			templ++;
		}
	}
	return (-1);
}


static LONG readitem(STRPTR buffer, LONG maxchars, struct CSource *input, ULONG ignoreequal)
{
/*

// Macro to get a character from the input source
#define GET(c)                                          \
	if(input!=NULL)                                     \
	{                                                   \
		if(input->CS_CurChr>=input->CS_Length)          \
			{ c=EOF; }                                  \
		else                                            \
			{ c=input->CS_Buffer[input->CS_CurChr++]; } \
	}                                                   \
	else                                                \
	{                                                   \
		c=FGetC(Input());                               \
		if(c==EOF&&*result)                             \
			{ return ITEM_ERROR; }                      \
	}

// Macro to push the character back
#define UNGET() {if(input!=NULL) input->CS_CurChr--; else UnGetC(Input(),-1);}

*/

#define GET(c) \
{ \
	if (input->CS_CurChr >= input->CS_Length) \
		c = EOF; \
	else \
		c = input->CS_Buffer[input->CS_CurChr++]; \
}

// Macro to push the character back
#define UNGET() {input->CS_CurChr--; }


	STRPTR b = buffer;
	LONG c;
	LONG dummy;
	//LONG *result=&((struct Process *)FindTask(NULL))->pr_Result2;
	LONG *result = &dummy;
	
	// Skip leading whitespace characters
	do
	{
		GET(c);
	}
	while (c == ' ' || c == '\t' || c == '\n');

	if (!c || c == '\n' || c == EOF)
	{
		// End of line found. Note that unlike the Amiga DOS original this funtion doesn't know about ';' comments. Comments are the shell's job, IMO. I don't need them here.
		if (c != EOF)
		{
			UNGET();
		}
		*b = 0;
		return (ITEM_NOTHING);
	}
	else if (!ignoreequal && (c == '='))
	{
		// Found '='. Return it.
		*b = 0;
		return (ITEM_EQUAL);
	}
	else if (c == '\"')
	{
		// Quoted item found. Convert Contents.
		for (;;)
		{
			if (!maxchars)
			{
				*buffer = 0;
				*result = ERROR_BUFFER_OVERFLOW;
				return (ITEM_ERROR);
			}
			maxchars--;
			GET(c);
			// Convert ** to *, *" to ", *n to \n and *e to 0x1b.
			if (c == '*')
			{
				GET(c);
				// Check for premature end of line.
				if (!c || c == '\n' || c == EOF)
				{
					if (c != EOF)
					{
						UNGET();
					}
					*buffer = 0;
					*result = ERROR_UNMATCHED_QUOTES;
					return (ITEM_ERROR);
				}
				else if (c == 'n' || c == 'N')
				{
					c = '\n';
				}
				else if (c == 'e' || c == 'E')
				{
					c = 0x1b;
				}
			}
			else if (!c || c == '\n' || c == EOF)
			{
				if (c != EOF)
				{
					UNGET();
				}
				*buffer = 0;
				*result = ERROR_UNMATCHED_QUOTES;
				return (ITEM_ERROR);
			}
			else if (c == '\"')
			{
				// " ends the item.
				*b = 0;
				return (ITEM_QUOTED);
			}
			*b++ = c;
		}
	}
	else
	{
		// Unquoted item. Store first character.
		if (!maxchars)
		{
			*buffer = 0;
			*result = ERROR_BUFFER_OVERFLOW;
			return (ITEM_ERROR);
		}
		maxchars--;
		*b++ = c;
		// Read upto the next terminator.
		for (;;)
		{
			if (!maxchars)
			{
				*buffer = 0;
				*result = ERROR_BUFFER_OVERFLOW;
				return (ITEM_ERROR);
			}
			maxchars--;
			GET(c);
			// Check for terminator
			if(!c || c == ' ' || c == '\t' || c == '\n' || (!ignoreequal && (c == '=')) || c == EOF)
			{
				if (c != EOF)
				{
					UNGET();
				}
				*b = 0;
				return (ignoreequal ? ITEM_QUOTED : ITEM_UNQUOTED);
			}
			*b++ = c;
		}
	}
}


/*
 * Extension: there's a /U for URIs. It's returned as quoted in that
 * case and doesn't care about '=' signs.
 */
struct RDArgs *readargs(CONST_STRPTR templ, LONG *array, struct RDArgs *rda)
{
	// Allocated resources
	struct DAList *dalist = NULL;
	UBYTE *flags = NULL;
	STRPTR strbuf = NULL;
	STRPTR *multvec=NULL, *argbuf=NULL;
	ULONG multnum=0, multmax=0;
	int strbufsize;

	// Some variables
	CONST_STRPTR cs1;
	STRPTR s1, s2, *newmult = newmult; /* shut up gcc */
	ULONG arg, numargs, nextarg;
	LONG it, item, chars, value;

	// Error recovery. C has no exceptions. This is a simple replacement.
	LONG error;
	#undef ERROR
	#define ERROR(a) { error=a; goto end; }

	// Template options
	#define REQUIRED 0x80 /* /A */
	#define KEYWORD  0x40 /* /K */
	#define TYPEMASK 0x07
	#define NORMAL	 0x00 /* No option */
	#define SWITCH	 0x01 /* /S, implies /K */
	#define TOGGLE	 0x02 /* /T, implies /K */
	#define NUMERIC  0x03 /* /N */
	#define MULTIPLE 0x04 /* /M */
	#define REST	 0x05 /* /F */
	#define URI      0x20 /* /U */

	// Flags for each possible character ('a', 'b, 'c', etc..)
	static const UBYTE argflags[] =	{
		REQUIRED, 0, 0, 0, 0, REST, 0, 0, 0, 0, KEYWORD, 0, MULTIPLE, NUMERIC, 0, 0, 0, 0, SWITCH|KEYWORD, TOGGLE|KEYWORD, URI, 0, 0, 0, 0, 0
	};

	ASSERT(rda);

	// Allocate readargs structure (and private internal one)
	if (!(dalist = (struct DAList *)malloc(sizeof(struct DAList))))
	{
		ERROR(ERROR_NO_FREE_STORE);
	}

	// Get enough space for string buffer. It's always smaller than the size of the input line+1.
	strbufsize = rda->RDA_Source.CS_Length + 1;
	if (!(strbuf = (STRPTR)malloc(strbufsize)))
	{
		ERROR(ERROR_NO_FREE_STORE);
	}

	// Count the number of items in the template (number of ','+1).
	numargs = 1;
	cs1 = templ;
	while (*cs1)
	{
		if (*cs1++ == ',')
		{
			numargs++;
		}
	}

	// Use this count to get space for temporary flag array and result buffer.
	if ( (flags = (UBYTE *)malloc(numargs + 1)) )
	{
		memclr(flags, numargs + 1);
	}
	else
	{
		ERROR(ERROR_NO_FREE_STORE);
	}
	
	if ( (argbuf = (STRPTR *)malloc((numargs+1)*sizeof(STRPTR))) )
	{
		memclr(argbuf, (numargs + 1) * sizeof(STRPTR));
	}
	else
	{
		ERROR(ERROR_NO_FREE_STORE);
	}

	// Fill the flag array.
	cs1 = templ;
	s2 = flags;
	while (*cs1)
	{
		// A ',' means: goto next item.
		if (*cs1 == ',')
		{
			s2++;
		}
		// In case of a '/' use the next character as option.
		if (*cs1++ == '/')
		{
			*s2 |= argflags[*cs1 - 'A'];
		}
	}
	// Add a dummy so that the whole line is processed.
	*++s2=MULTIPLE;

	/* Now process commandline for the first time:
	* Go from left to right and fill all items that need filling.
	* If an item is given as 'OPTION=VALUE' or 'OPTION VALUE' fill it out of turn.
	*/
	s1 = strbuf;
	for (arg = 0; arg <= numargs; arg = nextarg)
	{
		nextarg = arg + 1;

		/* Skip /K options and options that are already done. */
		if (flags[arg] & KEYWORD || argbuf[arg]) continue;

		/* If the current option is of type /F do not look for keywords */
		if ((flags[arg] & TYPEMASK) != REST)
		{
			/* Get item. Quoted items are no keywords. */
			it = readitem(s1, strbufsize, &rda->RDA_Source, (flags[arg] & URI));
			D(RDARGS,bug("item: <%s>\n", it ? s1 : (STRPTR)"none"));
			if ((it == ITEM_UNQUOTED))
			{
				D(RDARGS,bug("  not quoted\n"));
				/* Not quoted. Check if it's a keyword. */
				item = findarg(templ, s1);
				if (item >= 0 && !argbuf[item])
				{
					D(RDARGS,bug("  keyword found\n"));
					/* It's a keyword. Fill it and retry the current option at the next turn */
					nextarg = arg;
					arg = item;

					/* /S /T and /F may not be given as 'OPTION=VALUE'. */
					if ((flags[item] & TYPEMASK) != SWITCH && (flags[item] & TYPEMASK) != TOGGLE && (flags[item] & TYPEMASK) != REST)
					{
						/* Get value. */
						it = readitem(s1, strbufsize, &rda->RDA_Source, FALSE);
						D(RDARGS,bug("  value: <%s>\n", it ? s1 : (STRPTR)"none"));
						if (it == ITEM_EQUAL)
						{
							it = readitem(s1, strbufsize, &rda->RDA_Source, FALSE);
							D(RDARGS,bug("  == <%s>\n", it ? s1 : (STRPTR)"none"));
						}
					}
				}
			}

			/* Check returncode of readitem(). */
			if (it == ITEM_EQUAL)
				ERROR(ERROR_BAD_TEMPLATE);
			if (it == ITEM_ERROR)
				ERROR(ERROR_BAD_TEMPLATE);
			if (it == ITEM_NOTHING)
				break;
		}

		/* /F takes all the rest */
		if ((flags[arg] & TYPEMASK) == REST)
		{
			D(RDARGS,bug("/F.. taking the rest\n"));
			/* Skip leading whitespace */
			while (rda->RDA_Source.CS_CurChr < rda->RDA_Source.CS_Length && (rda->RDA_Source.CS_Buffer[rda->RDA_Source.CS_CurChr] == ' ' || rda->RDA_Source.CS_Buffer[rda->RDA_Source.CS_CurChr] == '\t'))
			{
				rda->RDA_Source.CS_CurChr++;
			}

			/* Find the last non-whitespace character */
			s2 = s1 - 1;
			argbuf[arg] = s1;
			while (rda->RDA_Source.CS_CurChr<rda->RDA_Source.CS_Length && rda->RDA_Source.CS_Buffer[rda->RDA_Source.CS_CurChr] && rda->RDA_Source.CS_Buffer[rda->RDA_Source.CS_CurChr] != '\n')
			{
				if (rda->RDA_Source.CS_Buffer[rda->RDA_Source.CS_CurChr] != ' ' && rda->RDA_Source.CS_Buffer[rda->RDA_Source.CS_CurChr] != '\t')
				{
					s2 = s1;
				}
				/* Copy string by the way. */
				*s1++ = rda->RDA_Source.CS_Buffer[rda->RDA_Source.CS_CurChr++];
			}
			/* Add terminator (1 after the character found). */
			s2[1] = '\0';
			it = ITEM_NOTHING;
			break;
		}

		/* /S or /T just set a flag */
		if ((flags[arg] & TYPEMASK) == SWITCH || (flags[arg] & TYPEMASK) == TOGGLE)
		{
			D(RDARGS,bug("/S or /T.. setting flag\n"));
			argbuf[arg] = (STRPTR)1;
		}
		else if ((flags[arg] & TYPEMASK) == MULTIPLE)
		{
			D(RDARGS,bug("/M mess..\n"));
			/* All /M arguments are stored in a buffer. */
			if (multnum >= multmax)
			{
				/* Buffer too small. Get a new one. */
				multmax += 16;
				if (!(newmult = (STRPTR *)malloc(multmax * sizeof(STRPTR))))
				{
					ERROR(ERROR_NO_FREE_STORE);
				}
				if (multvec)
				{
					memcpy((ULONG *)newmult, (ULONG *)multvec, multnum * sizeof(STRPTR));
					free(multvec);
				}
				multvec = newmult;
			}
			/* Put string into the buffer. */
			multvec[multnum++] = s1;
			while (*s1++);
			/* /M takes more than one argument, so retry. */
			nextarg = arg;
		}
		else /* NORMAL || NUMERIC */
		{
			D(RDARGS,bug("/N or normal, just copying the argument..\n"));
			/* Put argument into argument buffer. */
			argbuf[arg] = s1;
			while (*s1++);
		}
	}

	/* Unfilled /A options steal Arguments from /M */
	for (arg = numargs; arg-- > 0;)
	{
		if (flags[arg] & REQUIRED && argbuf[arg] == NULL && (flags[arg] & TYPEMASK) != MULTIPLE)
		{
			if (!multnum)
			{
				ERROR(ERROR_REQUIRED_ARG_MISSING);
			}
			argbuf[arg] = multvec[--multnum];
		}
	}

	/* Put the rest of /M where it belongs */
	for (arg = 0; arg < numargs; arg++)
	{
		if ((flags[arg] & TYPEMASK) == MULTIPLE)
		{
			if (flags[arg] & REQUIRED && !multnum)
			{
				ERROR(ERROR_REQUIRED_ARG_MISSING);
			}

			/* NULL terminate it. */
			if (multnum >= multmax)
			{
				multmax += 16;
				if (!(newmult = (STRPTR *)malloc(multmax * sizeof(STRPTR))))
				{
					ERROR(ERROR_NO_FREE_STORE);
				}
				if (multvec)
				{
					memcpy((ULONG *)newmult, (ULONG *)multvec, multnum * sizeof(STRPTR));
					free(multvec);
				}
				multvec = newmult;
			}
			multvec[multnum++] = NULL;
			argbuf[arg] = (STRPTR)multvec;
			break;
		}
	}

	/* There are some arguments left? Return error. */
	if(multnum && arg == numargs)
	{
		ERROR(ERROR_TOO_MANY_ARGS);
	}

	D(RDARGS,bug("commandline fully processed, array:\n"));

	/* The commandline is processed now. Put the results in the result array. Convert /N arguments by the way. */
	for (arg = 0; arg < numargs; arg++)
	{
		/* Just for the arguments given. */
		if (argbuf[arg])
		{
			switch (flags[arg] & TYPEMASK)
			{
				case NORMAL:
				case MULTIPLE:
				case REST:
				case SWITCH:
				case URI:
					/* Simple arguments are just copied. */
					array[arg] = (LONG)argbuf[arg];
					if ((flags[arg] & TYPEMASK) == MULTIPLE ||
					    (flags[arg] & TYPEMASK) == SWITCH)
					{
						D(RDARGS,bug("  0x%lx>\n", array[arg]));
					}
					else
					{
						D(RDARGS,bug("  <%s>\n", (STRPTR)array[arg]));
					}
					break;
				
				case TOGGLE:
					/* /T logically inverts the argument. */
					array[arg] = !array[arg];
					D(RDARGS,bug("  %ld/T\n", array[arg]));
					break;
				
				case NUMERIC:
					/* Convert /N argument. */
					chars = strtolong(argbuf[arg], &value);
					if (chars <= 0 || argbuf[arg][chars])
					{
						ERROR(ERROR_BAD_NUMBER);
					}
					/* Put the result where it belongs. */
					if (flags[arg] & REQUIRED)
					{
						/* Required argument. Return number. */
						array[arg]=value;
					}
					else
					{
						/* not required, abuse the argbuf buffer then. it's not needed anymore. */
						argbuf[arg] = (STRPTR)value;
						array[arg] = (LONG)&argbuf[arg];
					}
					/* XXX: I commented out the stuff.. makes no sense for me */
					D(RDARGS,bug("  %ld/N\n", value));
					break;
			}
		}
	}

	/* All OK. */
	error = 0;
end:
	if (error)
	{
		PDB(("*** ERROR: %ld\nTemplate was: %s)\nGot: %s\n",error,templ,rda->RDA_Source.CS_Buffer));
	}

	/* Cleanup and return. */
	if (flags) free(flags);
	
	if(error)
	{
		/* ReadArgs() failed. Clean everything up. */
		if (dalist) free(dalist);
		if (argbuf) free(argbuf);
		if (strbuf) free(strbuf);
		if (multvec) free(multvec);
		//me->pr_Result2=error;
		return (NULL);
	}
	else
	{
		/* All went well. Prepare result and return. */
		rda->RDA_DAList=(LONG)dalist;
		dalist->ArgBuf=argbuf;
		dalist->StrBuf=strbuf;
		dalist->MultVec=multvec;
		return (rda);
	}
}


void freeargs(struct RDArgs *args)
{
	if(args)
	{
	
		/* ReadArgs() failed. Clean everything up. */
		if (args->RDA_DAList)
		{
			struct DAList *da = (struct DAList *)args->RDA_DAList;

			if (da->ArgBuf) free(da->ArgBuf);
			if (da->StrBuf) free(da->StrBuf);
			if (da->MultVec) free(da->MultVec);
			free(da);

		/*
		    Why do I put this here. Unless the user has been bad and
		    set RDA_DAList to something other than NULL, then this
		    RDArgs structure was allocated by ReadArgs(), so we can
		    free it. Otherwise the RDArgs was allocated by
		    AllocDosObject(), so we are not allowed to free it.

		    See the original AmigaOS autodoc if you don't believe me
		*/
			//free(args); /* XXX: hum.. */
		}
	}
}

#endif /* USE_INTERNAL_READARGS */
