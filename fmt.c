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
 * $Id: fmt.c,v 1.5 2006/08/08 13:31:34 fab Exp $
 */

#include "ambient.h"

/* public */
#include <proto/exec.h>
#include <emul/emulinterface.h>

/* private */
#include "fmt.h"

struct fmt_data {
	UBYTE *buf;
	ULONG size;
};

static void putchproc_GATE(void);
static const struct EmulLibEntry putchproc = {
	TRAP_LIBNR, 0, (void(*)(void))putchproc_GATE
};
static void putchproc_GATE(void)
{
	TEXT c = REG_D0;
	struct fmt_data *fd = (struct fmt_data *)REG_A3;

	if (fd->size)
	{
		*fd->buf++ = c;
		fd->size--;
	}
}


void snprintf_array(STRPTR buf, ULONG size, CONST_STRPTR fmt, APTR array1)
{
	struct fmt_data fd;
	UBYTE c;
	char *array = (char *)array1;

	ASSERT(buf);
	ASSERT(size);
	ASSERT(fmt);

	if (size == 1)
	{
		*buf = '\0';
		return;
	}

	size--;

	/* Since the trap driven RawDoFmt() putchproc is pretty slow,
	 * optimize by formatting manually as far as possible. Once we run
	 * into something we can't handle directly, process rest of the
	 * string with the slow routine. - piru
	 */
	for (;;)
	{
		c = fmt[0];
		if (c == '\0' || size == 0)
		{
			*buf = '\0';
			return;
		}

		if (c == '%')
		{
			LONG l;

			c = fmt[1];
			if (c == 's')
			{
				char *p = *((char **) array);
				array += sizeof(char **);
				stccpy(buf, p ? p : "", size + 1);
				fmt += 2;
			}
			else if (c == 'l')
			{
				c = fmt[2];
				if (c == 'u')
				{
					snprintf(buf, size + 1, "%lu", *((ULONG *) array));
					array += sizeof(ULONG *);
				}
				else if (c == 'd')
				{
					snprintf(buf, size + 1, "%ld", *((LONG *) array));
					array += sizeof(LONG *);
				}
				else if (c == 'x')
				{
					snprintf(buf, size + 1, "%lX", *((LONG *) array));
					array += sizeof(LONG *);
				}
				else
				{
					break;
				}

				fmt += 3;
			}
			else if (c == '%')
			{
				*buf++ = '%';
				fmt += 2;
				size--;
				continue;
			}
			else
			{
				break;
			}

			l = strlen(buf);
			buf += l;
			size -= l;
		}
		else
		{
			*buf++ = c;
			fmt++;
			size--;
		}
	}
	/* If we end up here it means we ran into some unknown format code.
	 * Fear not, we just use RawDoFmt from here on. Note that 'buf',
	 * 'size', 'fmt' and 'array' are all maintained properly by the
	 * above code. Neat, eh?
	 */

	fd.buf  = buf;
	fd.size = size;

	RawDoFmt((UBYTE *)fmt, array, (APTR)&putchproc, &fd);

	/* Did we run into buffer end? If so, make sure the buffer is '\0'-
	 * terminated.
	 */
	if (fd.size == 0)
	{
		*fd.buf = '\0';
	}
}

