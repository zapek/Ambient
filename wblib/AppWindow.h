#ifndef APPWINDOW_H
#define APPWINDOW_H


#ifndef UTILITY_TAGITEM_H
# include <utility/tagitem.h>
#endif

/* tags for AddAppWindow */

#define WB_AW_Dummy               (TAG_USER + 0x200000)
#define WB_AW_MouseReport 		  (WB_AW_Dummy + 1)
	          
/* AppWindow message classes */

#define AM_CLASS_MOUSEMOVE		0x00000001
#define AM_CLASS_MOUSEENTER		0x00000002
#define AM_CLASS_MOUSEEXIT		0x00000004

#endif