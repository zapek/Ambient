#ifndef AMBIENT_BOOKMARKS_H
#define AMBIENT_BOOKMARKS_H

struct bookmarkitem {
	STRPTR description;
	STRPTR location;
};

APTR bookmarks_buildmenuobj(LONG viewid);
void bookmarks_adduri(STRPTR uri, STRPTR description, LONG permanent);
void bookmarks_updatefromlist(APTR list, LONG permanent);

#endif /* AMBIENT_BOOKMARKS_H */
