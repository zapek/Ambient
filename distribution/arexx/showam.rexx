/*
 *  $VER: showam.rexx 1.0 (21.02.2007) © 2007 MorphOS Team
 *
 *  synopsis: opens the Ambient view of the current dir from 
 *            Shell 
 *
 *  $Id: showam.rexx,v 1.2 2013/05/19 21:29:28 tokai Exp $
 */

view = "list"    /* icon or list      */
mode = "thumbs"  /* thumbs or all     */
psa  = "50"      /* px from left      */
psb  = "50"      /* px from top       */
psc  = "440"     /* width             */
psd  = "320"     /* height            */

location = pragma(d)

params='?view='||view||'&mode='||mode||'&top='||psb||'&left='||psa||'&width='||psc||'&height='||psd

/*  LoadURI likes double-quote for filenames with spaces
 */
loc = '"'||'file://'||location||params||'"'||" newwin"

address ambient
LoadURI loc
