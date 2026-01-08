/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * © 2001-2004 by David Gerber <zapek@morphos.net>
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
 * $Id: replacements.c,v 1.3 2006/12/20 13:34:18 fab Exp $
 */

#include "globals.h"
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include <time.h>

unsigned long errno;

static const char monthtable[]=
{ 31,29,31,30,31,30,31,31,30,31,30 };

struct tm *gmtime(const time_t *t)
{
  static struct tm utim;
  signed long tim;
  int leapday=0,leapyear=0,i;
  tim=*t;
  utim.tm_sec=tim%60;
  tim/=60;
  utim.tm_min=tim%60;
  tim/=60;
  utim.tm_hour=tim%24;
  tim=tim/24+719162;
  utim.tm_wday=(tim+1)%7;
  utim.tm_year=tim/146097*400-1899;
  tim%=146097;
  if(tim>=145731)
  { leapyear++; /* The day is in one of the 400th */
	if(tim==146096)
	{ tim--; /* Be careful: The last of the 4 centuries is 1 day longer */
	  leapday++; }
  }
  utim.tm_year+=tim/36524*100;
  tim%=36524;
  if(tim>=36159)
	leapyear--; /* The day is in one of the 100th */
  utim.tm_year+=tim/1461*4;
  tim%=1461;
  if(tim>=1095)
  { leapyear++; /* The day is in one of the 4th */
	if(tim==1460)
	{ tim--; /* Be careful: The 4th year is 1 day longer */
	  leapday++; }
  }
  utim.tm_year+=tim/365;
  tim=tim%365+leapday;
  utim.tm_yday=tim;
  if(!leapyear&&tim>=31+28)
	tim++; /* add 1 for 29-Feb if no leap year */
  for(i=0;i<11;i++)
	if(tim<monthtable[i])
	  break;
	else
	  tim-=monthtable[i];
  utim.tm_mon=i;
  utim.tm_mday=tim+1;
  utim.tm_isdst=0;
  return &utim;
}

void local_abort(void);

void local_abort(void)
{
  struct Library *IntuitionBase;

  IntuitionBase = OpenLibrary("intuition.library", 37);
  if (IntuitionBase)
  {
    static const struct EasyStruct es =
    {
      sizeof(struct EasyStruct),
      0,
      "png.alib",
      "This program has run into fatal abort condition\n"
      "inside the png.alib. The program will be halted.",
      "Ok"
    };

    EasyRequestArgs(NULL, (struct EasyStruct *)&es, NULL, NULL);

    CloseLibrary(IntuitionBase);
  }
  Wait(0);
}
