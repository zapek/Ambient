#ifndef AMBIENT_PNGICON_SPECS_H
#define AMBIENT_PNGICON_SPECS_H
/*
 * Ambient's PNGicon format definition.
 *
 * © 2002 by David Gerber <zapek@meanmachine.ch>, All Rights Reserved
 *
 * $Id: pngicon_specs.h,v 1.3 2006/02/22 14:48:22 fab Exp $
 */

#include <utility/tagitem.h>

#define PNG_icOn const UBYTE png_icOn[] = { 'i', 'c', 'O', 'n', '\0' }

#define PNGICON_Dummy (TAG_USER + 0x1000)

/*
  Should have one of the following size (in pixels):
  - 128x128
  - 64x64
  - 32x32
  - 16x16

  The prefered size is 64x64 except for icons aimed at small toolbars.
  The user will be able to chose what size he prefers the most and Ambient
  will scale the icons accordingly using sophisticated algorithms.

  There are 2 depths supported: 24-bit and 32-bit (with alpha).

  For consistancy purpose, there's only 1 image per icon. The selected state
  display is Ambient's job and must be the same for every icon.

  The icon is a standard PNG image with the additional chunk 'icOn'.
  It contains tags. All of them are optional and can appear anywhere in the file.
  They're unique except for PNGICON_ToolType. Ambient will reject icons with non
  unique tags.

  There are the tags:
*/


#define PNGICON_LocationX (PNGICON_Dummy + 1) /* LONG */
/*
  Horizontal location of the icon in pixels.
  If not provided, Ambient will place the icon itself.
*/


#define PNGICON_LocationY (PNGICON_Dummy + 2) /* LONG */
/*
  Vertical location of the icon in pixels.
  If not provided, Ambient will place the icon itself.
*/


#define PNGICON_DrawerLeft     (PNGICON_Dummy + 3) /* LONG */
/*
  Left border of the window to open when clicking on
  the icon. Makes only sense for icons that open a window, is stripped on save
  otherwise. If not provided, Ambient will chose where to open the
  window. This tag should NOT be preset. Ambient will be smart enough to chose
  where to open the window. Therefore you MUST strip it when distributing icons
  or applications with icons.
*/


#define PNGICON_DrawerTop      (PNGICON_Dummy + 4) /* LONG */
/*
  Top border of the window to open when clicking on
  the icon. Makes only sense for icons that open a window, is stripped on save
  otherwise. If not provided, Ambient will chose where to open the
  window. This tag should NOT be preset. Ambient will be smart enough to chose
  where to open the window. Therefore you MUST strip it when distributing icons
  or applications with icons.
*/


#define PNGICON_DrawerWidth    (PNGICON_Dummy + 5) /* ULONG */
/*
  Width of the window to open when clicking on the
  icon. Makes only sense for icons that open a window, is stripped on save
  otherwise. If not provided, Ambient will try to provide a useable
  size but might be wrong as it opens the window before scanning everything in
  it.
*/


#define PNGICON_DrawerHeight   (PNGICON_Dummy + 6) /* ULONG */
/*
  Height of the window to open when clicking on the
  icon. Makes only sense for icons that open a window, is stripped on save
  otherwise. If not provided, Ambient will try to provide a useable
  size but might be wrong as it opens the window before scanning everything in
  it.
*/


#define PNGICON_DrawerViewMode (PNGICON_Dummy + 7) /* ULONG */
/*
  Value of the viewmode. Private.
*/


#define PNGICON_StackSize    (PNGICON_Dummy + 9) /* ULONG */
/*
  Size of the stack, makes only sense for TOOLS icons,
  is stripped by Ambient otherwise. Note: there's no excuse for applications to
  not set their stack properly. This tooltype is mostly for old applications.
*/


#define PNGICON_DefaultTool  (PNGICON_Dummy + 10) /* BSTR */
/*
  Path for the default tool. Makes only sense for
  PROJECT icons and if it's present it means that the icon is exactly that: a
  PROJECT. Ambient will silently strip it when saving non PROJECT icons
  (device, etc..).
*/


#define PNGICON_ToolType     (PNGICON_Dummy + 11) /* BSTR, can appear multiple times */
/*
  String describing a tooltype. Is the only tag
  which can appear many times. The tooltypes are presented to the user in
  the order of appearance.
*/


#define PNGICON_DrawerSortMode (PNGICON_Dummy + 12) /* ULONG */
/*
  Value of the sortmode. Private.
*/

#endif /* AMBIENT_PNGICON_SPECS_H */
