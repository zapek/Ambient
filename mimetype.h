#ifndef AMBIENT_MIMETYPE_H
#define AMBIENT_MIMETYPE_H
/*
 * $Id: mimetype.h,v 1.9 2008/07/12 23:20:37 kiero Exp $
 */

#define RECOGNITION_FILE "recognition.db"
#define FILETYPE_DIR "filetypes"

struct internal_mimetype_node {
	struct MinNode n;
	APTR rctx;
	STRPTR mimetype;
	STRPTR description;
	ULONG action;
	ULONG newwin;
	ULONG flags;
	STRPTR descriptor;
	LONG priority;
	struct MinList *action_list;     /* refers to internal_action_list in most cases */
	struct MinList internal_action_list;
};

/*
 * Flags.
 */
#define MIMETYPEFLAG_INTERNAL	    1 /* Is this an internal mimetype? */
#define MIMETYPEFLAG_FOREIGNRECOG   2 /* recognition rule taken from external node */
#define MIMETYPEFLAG_FOREIGNACTIONS 4 /* actions taken from external node */
#define MIMETYPEFLAG_FOREIGN        8 /* whole node is a foreign node */

/*
 * Attributes.
 */
enum {
	MIMETYPETAG_MEDIA = TAG_USER + 1, /* [.G] MEDIATYPE_#? */
	MIMETYPETAG_MIME,                 /* [SG] foobar/plop etc.. */
	MIMETYPETAG_ACTION,               /* [SG] MIMEACTION_#? */
	MIMETYPETAG_NEWWIN,               /* [SG] open a new window or not */
	MIMETYPETAG_SUBTYPE,              /* [SG] used by datatypes and multimedia */
	MIMETYPETAG_DESCRIPTION,          /* [SG] description */
	MIMETYPETAG_FILEINFO,             /* [SG] has fileinfo or not */
	MIMETYPETAG_SECONDS,              /* [SG] number of seconds since 1978, valid if FILEINFO is TRUE */
	MIMETYPETAG_FILESIZE,             /* [SG] size of the file, valid if FILEINFO is TRUE */
	MIMETYPETAG_FILESIZEPTR,          /* [SG] same as above put passed as a pointer to QUAD. Should be used instead of FILESIZE */
};


/*
 * Flags for mimetype_create().
 */
#define MTF_URI       (1 << 0UL)
#define MTF_EXTENSION (1 << 1UL) /* NYI */
#define MTF_PROTOCOL  (1 << 2UL) /* NYI */
#define MTF_FILEIO    (1 << 3UL) /* NYI */
#define MTF_RECURSE   (1 << 4UL) /* NYI */
#define MTF_NETWORK   (1 << 5UL) /* NYI */

/*
 * MTA_MEDIATYPEs.
 */
enum {
	MEDIATYPE_UNKNOWN,
	MEDIATYPE_INTERNAL,
	MEDIATYPE_APPLICATION,
	MEDIATYPE_AUDIO,
	MEDIATYPE_IMAGE,
	MEDIATYPE_MESSAGE,
	MEDIATYPE_MULTIPART,
	MEDIATYPE_TEXT,
	MEDIATYPE_VIDEO,
};

/*
 * Action events.
 */
enum {
	ACTION_EVENT_NONE,
	ACTION_EVENT_DOUBLECLICK,
	ACTION_EVENT_DRAGNDROP,
	ACTION_EVENT_MENU,
};

/*
 * Action qualifiers.
 */
enum {
	ACTION_QUALIFIER_NONE,
	ACTION_QUALIFIER_SHIFT,
	ACTION_QUALIFIER_CONTROL,
	ACTION_QUALIFIER_ALT,
};

/*
 * Action types.
 */
enum {
	ACTION_TYPE_INVALID,
	ACTION_TYPE_INTERNAL,
	ACTION_TYPE_CLI,
	ACTION_TYPE_WB,
	ACTION_TYPE_AREXX,
};


/*
 * Action flags.
 */
enum {
	ACTION_FLAG_NONE = 0,
	ACTION_FLAG_UNQUOTED = 1,           /* don't add quotes to each uri/path */
	ACTION_FLAG_NEED_DEST = 2,          /* (auto) tells that the action needs info about destination */
	ACTION_FLAG_ASYNC = 4,
	ACTION_FLAG_MULTIPLE = 8,           /* perform action on multiple files instead of on each one separately */
	ACTION_FLAG_CD_SOURCE = 16,         /* change directory to source one (have to be provided) */
	ACTION_FLAG_CD_DESTINATION = 32,    /* change directory to destination one (have to be provided) */
	ACTION_FLAG_SPATIALMODE = 64,       /* action is selected if spatial windowing mode is enabled */
	ACTION_FLAG_NEED_ENTRIES =128,      /* set if the action operates on files/dirs/etc */
	ACTION_FLAG_NEED_PATH =256,         /* set if the action needs paths */
	ACTION_FLAG_NEED_URI =512,          /* set if the action needs uris */
	ACTION_FLAG_NEED_FILEPART =1024,    /* set if the action needs file parts of paths */
};

ULONG mimetype_init(void);
void mimetype_cleanup(void);

APTR mimetype_create(CONST_STRPTR scheme, CONST_STRPTR path, ULONG flags);
void mimetype_delete(APTR ctx);

APTR mimetype_getattr(APTR ctx, ULONG attr);
void v_mimetype_setattrs(APTR ctx, struct TagItem *tags);
void mimetype_setattrs(APTR ctx, ...);

APTR mimetype_findaction(CONST_STRPTR scheme, CONST_STRPTR path, ULONG event, ULONG qualifier, ULONG flags);
APTR mimetype_find(CONST_STRPTR scheme, CONST_STRPTR path, ULONG flags);
APTR mimetype_find_pattern(CONST_STRPTR scheme, CONST_STRPTR path, ULONG flags, CONST_STRPTR mimetypepattern);

void mimetype_load_database(CONST_STRPTR filename);
void mimetype_invalidate(APTR imn, CONST_STRPTR mimetype);



/*
 * XXX: Used by finding function. Maybe something more generic needed?
 */

STRPTR *mimetype_gettypesarray(LONG internal, LONG mimetypes);
void mimetype_freetypesarray(STRPTR *array);
ULONG mimetype_checkpath(APTR mimetype, CONST_STRPTR path);
APTR mimetype_find_by_description(CONST_STRPTR description);
APTR mimetype_find_by_mimetype(CONST_STRPTR mimetype);
APTR mimetype_find_generic_by_mimetype(CONST_STRPTR mimetype);
APTR mimetype_find_foreign_actions_by_mimetype(CONST_STRPTR mimetype);

/* mimetype editor helpers */
struct internal_mimetype_node * imn_create(CONST_STRPTR mimetype, CONST_STRPTR description);
void imn_delete(struct internal_mimetype_node *imn);
struct internal_mimetype_node * imn_duplicate(struct internal_mimetype_node * simn);
void mimetype_clear_actions(CONST_STRPTR mimetype);

#endif /* AMBIENT_MIMETYPE_H */
