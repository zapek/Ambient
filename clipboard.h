#ifndef AMBIENT_CLIPBOARD_H
#define AMBIENT_CLIPBOARD_H
/*
 * $Id: clipboard.h,v 1.3 2007/02/11 22:35:28 fab Exp $
 */

#include <exec/lists.h>
#include <exec/types.h>

#define CLIPBOARD_REMINDER_DELAY 60       // 60 seconds reminder delay
#define CLIPBOARD_REMINDER_MAX_ENTRIES 10 // maximum lines count displayed in reminder

typedef enum { CLIPBOARD_VOID, CLIPBOARD_COPY, CLIPBOARD_CUT } clipboard_mode_t;

struct clipboard_node {
	struct MinNode n;
	STRPTR filename;
};

ULONG clipboard_init(void);
void  clipboard_cleanup(void);

void clipboard_clear  (void);                   // empty the clipboard
void clipboard_set_mode(clipboard_mode_t mode, ULONG handleicons); // set clipboard mode
clipboard_mode_t clipboard_get_mode(void); 	    // get clipboard mode
BOOL clipboard_add    (STRPTR filename);        // add a file to clipboard
#if 0
BOOL clipboard_paste(STRPTR destpath, ULONG sync, LONG viewid); // paste clipboard to destination
#endif

/*
BOOL clipboard_paste_as  (STRPTR destpath);     // paste the selection to destination. pops up a requester to give new names
BOOL clipboard_paste_into(STRPTR archive);      // paste into an archive (reading mime to check destination ?)
*/

BOOL clipboard_is_present(STRPTR filename);     // test file presence in clipboard
BOOL clipboard_is_empty(void);                  // test if clipboard is empty
void clipboard_reset_timer(void);               // reset reminder delay

//struct MinList * clipboard_names(void);         // return the file list

ULONG tr_clipboard_paste(STRPTR destpath, LONG viewid, ULONG rename);

/* callbacks for menu */
BOOL clipboard_menu_copy_enable(void);
BOOL clipboard_menu_paste_enable(void);

#endif
