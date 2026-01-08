#ifndef STORAGE_H
#define STORAGE_H

ULONG storage_init(void);
void storage_cleanup(void);
void storage_commit(void);
void storage_set(ULONG id, ULONG type, CONST_APTR data);
void storage_get(ULONG id, ULONG type, APTR *data);
void storage_delete(ULONG id);


enum
{
	STORAGE_STRING,
	STORAGE_STRARRAY,

	/* these types won't be saved on disk */
	
	STORAGE_MSTRING,
	STORAGE_MSTRARRAY
};


#define STORAGE_EXEC_HISTORY                MAKE_ID('0', '0', '0', '1')
#define STORAGE_FIND_NAME_HISTORY           MAKE_ID('0', '0', '0', '2')
#define STORAGE_FIND_TEXT_HISTORY           MAKE_ID('0', '0', '0', '3')
#define STORAGE_CHANGE_LOCATION_HISTORY     MAKE_ID('0', '0', '0', '4')
#define STORAGE_LISTVIEW_FORMAT             MAKE_ID('0', '0', '0', '5')
#define STORAGE_SELECTION_PATTERN           MAKE_ID('0', '0', '0', '6')
#define STORAGE_BOOKMARKS                   MAKE_ID('0', '0', '0', '7')
#define STORAGE_TEMPORARY_BOOKMARKS         MAKE_ID('0', '0', '0', '8')
#define STORAGE_IMAGEVIEW_SNAPSHOT          MAKE_ID('0', '0', '0', '9')
#define STORAGE_TEXTVIEW_SNAPSHOT           MAKE_ID('0', '0', '1', '0')
#define STORAGE_FIND_COMMENT_HISTORY        MAKE_ID('0', '0', '1', '1')
#define STORAGE_HEXVIEW_SNAPSHOT            MAKE_ID('0', '0', '1', '2')

#endif
