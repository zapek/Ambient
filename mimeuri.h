#ifndef AMBIENT_MIMEURI_H
#define AMBIENT_MIMEURI_H
/*
 * $Id: mimeuri.h,v 1.8 2025/08/10 22:08:31 jacadcaps Exp $
 */

/*
 * mimeuri_getattr() attributes.
 */
enum {
	MIMEURIATTR_URI = TAG_USER + 0x200,
	MIMEURIATTR_SCHEME,
	MIMEURIATTR_HOST,
	MIMEURIATTR_PORT,
	MIMEURIATTR_USERNAME,
	MIMEURIATTR_PASSWORD,
	MIMEURIATTR_PATH,
	MIMEURIATTR_ARGS,
	MIMEURIATTR_FRAGMENT,
	MIMEURIATTR_LOCAL,
	MIMEURIATTR_MIMETYPE,
	MIMEURIATTR_MIME_ACTION,
	MIMEURIATTR_MIME_NEWWIN,
	MIMEURIATTR_MIME_SUBTYPE, /* eg. datatypes/multimedia type */
	MIMEURIATTR_MIME_DESCRIPTION,
	MIMEURIATTR_MIME_FILEINFO,
	MIMEURIATTR_MIME_SECONDS,
	MIMEURIATTR_MIME_FILESIZE,
	MIMEURIATTR_MIME_FILESIZEPTR,
};


/*
 * mimeuri_gather() tags.
 */
enum {
	/* mimetype recognition */
	MIMEURIGATHERTAG_URI = TAG_USER + 0x300, /* (TRUE) use the URI. can be called from the main task if it's the only search used */
	MIMEURIGATHERTAG_Extension,              /* (TRUE) uses the extension for mimetype recognition */
	MIMEURIGATHERTAG_Protocol,               /* (TRUE) uses information supplied by the transport protocol (http headers, datatype subsystem, etc..) */
	MIMEURIGATHERTAG_FileIO,                 /* (TRUE) uses fileio to recognize the file */
	MIMEURIGATHERTAG_Recurse,                /* (FALSE) allow recursion to gather infos about total filesize, number of files, etc.. */
	MIMEURIGATHERTAG_Network,                /* (TRUE) allows to perform fileio on network devices */
};


/*
 * mime actions.
 */
enum {
	MIMEACTION_NONE,
	MIMEACTION_VIEW,
	MIMEACTION_EXECUTE,
	MIMEACTION_LOADSEG,
	MIMEACTION_FSCONTEXT,
	MIMEACTION_PLAYSOUND,
	MIMEACTION_DATATYPES,
	MIMEACTION_MULTIMEDIA,
	MIMEACTION_OS4,
	MIMEACTION_USER,
};


/*
 * Some internal mimetypes.
 */
#define MIMETYPE_INTERNAL_ROOTVIEW       "internal/rootview"
#define MIMETYPE_INTERNAL_VOLUMES        "internal/volumes"
#define MIMETYPE_INTERNAL_VFS            "internal/vfs"
#define MIMETYPE_INTERNAL_DIRECTORY      "internal/directory"
#define MIMETYPE_INTERNAL_MULTIMEDIA     "internal/multimedia"
#define MIMETYPE_INTERNAL_DATATYPES      "internal/datatypes"                      /* transcient mimetype */
#define MIMETYPE_INTERNAL_EXECUTABLE_PPC "application/x-executable-morphos-ppc"
#define MIMETYPE_INTERNAL_EXECUTABLE_68K "application/x-executable-amigaos-68k"
#define MIMETYPE_INTERNAL_EXECUTABLE_OS4 "application/x-executable-os4-ppc"
#define MIMETYPE_INTERNAL_PROJECT        "application/x-executable-project"
#define MIMETYPE_INTERNAL_SCRIPT         "application/x-amigados"
#define MIMETYPE_INTERNAL_VORBIS         "audio/x-vorbis"
#define MIMETYPE_INTERNAL_MPEGA          "audio/mpeg"                              /* XXX: sucks too.. and there's audio/x-mpeg as well */


APTR mimeuri_create(void);
void mimeuri_delete(APTR ctx);
APTR mimeuri_duplicate(APTR ctx);
ULONG v_mimeuri_gather(APTR ctx, CONST_STRPTR uri, struct TagItem *tags);
ULONG mimeuri_gather(APTR ctx, CONST_STRPTR uri, ...);
APTR mimeuri_getattr(APTR ctx, ULONG attr);
void v_mimeuri_setattrs(APTR ctx, struct TagItem *tags);
void mimeuri_setattrs(APTR ctx, ...);
ULONG mimeuri_readargs(APTR ctx, CONST_STRPTR templ, LONG *array);
ULONG mimeuri_hasscheme(CONST_STRPTR path);

LONG mimeuri_encodepath(UBYTE *buf, LONG buflen, CONST_STRPTR path);
LONG mimeuri_decodepath(UBYTE *buf, LONG buflen, CONST_STRPTR uri, LONG urilen);

#endif /* AMBIENT_MIMEURI_H */
