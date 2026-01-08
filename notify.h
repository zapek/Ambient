#ifndef AMBIENT_NOTIFY_H
#define AMBIENT_NOTIFY_H
/*
 * $Id: notify.h,v 1.7 2016/01/24 21:53:03 itix Exp $
 */

#include <utility/tagitem.h>

ULONG notify_init(void);
void notify_cleanup(void);


APTR notify_create(void);
ULONG notify_register(APTR ctx, ...);
ULONG v_notify_register(APTR ctx, struct TagItem *tags);

#define UNRELIABLE 2
/*
 * That tag value means it's ok to miss some events.
 * Typically used for operations that could send many events.
 * Also for views which don't require absolute reliability.
 */

enum {
	NOTIFYTAG_Monitor_File = TAG_USER + 1, /* URI or path */
	/*
	 * Puts a notify on a path, like "SYS:foo", accepts patterns like "SYS:foo/ *",
	 * can be specified multiple times but each NOTIFYTAG_Monitor_File_#? refers to
	 * the previous NOTIFYTAG_Monitor_File only.
	 */
	NOTIFYTAG_Monitor_File_Create, /* TRUE/FALSE/UNRELIABLE */
	NOTIFYTAG_Monitor_File_Delete, /* TRUE/FALSE/UNRELIABLE */
	NOTIFYTAG_Monitor_File_Name,   /* TRUE/FALSE/UNRELIABLE */
	/*
	 * Monitors name changes but only on the same place.
	 * Rename operations changing the directory location would
	 * send create/delete notifications.
	 */
	NOTIFYTAG_Monitor_File_Date,    /* TRUE/FALSE/UNRELIABLE */
	NOTIFYTAG_Monitor_File_Comment, /* TRUE/FALSE/UNRELIABLE */
	NOTIFYTAG_Monitor_File_Size,    /* TRUE/FALSE/UNRELIABLE */
	NOTIFYTAG_Monitor_File_Flags,   /* TRUE/FALSE/UNRELIABLE */
	NOTIFYTAG_Monitor_File_UID,     /* TRUE/FALSE/UNRELIABLE */
	NOTIFYTAG_Monitor_File_GID,     /* TRUE/FALSE/UNRELIABLE */
	NOTIFYTAG_Monitor_File_Icon,    /* TRUE/FALSE/UNRELIABLE */

	NOTIFYTAG_Monitor_Device,       /* URI or path */
	/*
	 * Puts a notify on devices, like "foo:", accepts patterns
	 * like "*", other patterns work too but don't make sense.
	 * Can be specified multiple times but each NOTIFYTAG_Monitor_Device_#?
	 * refers to the previous NOTIFYTAG_Monitor_Device only.
	 */
	NOTIFYTAG_Monitor_Device_Mount,
	NOTIFYTAG_Monitor_Device_UnMount,
	NOTIFYTAG_Monitor_Device_Name,

	NOTIFYTAG_Inform_Object, /* APTR obj */
	/*
	 * Sends a MM_Notify_Change method to 'obj', one per context.
	 */
	NOTIFYTAG_Inform_Hook, /* struct Hook * (XXX: NYI) */
	/*
	 * Calls the hook, one per context.
	 */
	NOTIFYTAG_Monitor_Enable,    /* TRUE/FALSE/UNRELIABLE */
};


/*
 * That one uses the same tags as above except
 * NOTIFYTAG_Monitor_File can have NOTIFYVAL_Monitor_File_ClearAll
 * (and same for devices)
 */
void notify_unregister(APTR ctx, ...);
void v_notify_unregister(APTR ctx, struct TagItem *tags);

#define NOTIFYVAL_Monitor_File_ClearAll 1
#define NOTIFYVAL_Monitor_Device_ClearAll 1


void notify_action(CONST_STRPTR path, ULONG type, ...);

void notify_delete(APTR ctx);


struct notifyact * notify_action_get(APTR ctx);

struct notifyact {
	ULONG action;
};

struct notifyact_file_create {
	ULONG action;
	STRPTR uri;
};

struct notifyact_file_icon {
	ULONG action;
	STRPTR uri;
};

struct notifyact_file_delete {
	ULONG action;
	STRPTR uri;
};

struct notifyact_file_name {
	ULONG action;
	STRPTR uri;
	STRPTR newname;
};

struct notifyact_file_enable {
	ULONG action;
	STRPTR uri;
	ULONG enable;
};

struct notifyact_file_date {
	ULONG action;
	STRPTR uri;
	struct DateStamp datestamp;
};

struct notifyact_file_comment {
	ULONG action;
	STRPTR uri;
	STRPTR comment;
};

struct notifyact_file_size {
	ULONG action;
	STRPTR uri;
	UQUAD size;
};

struct notifyact_file_flags {
	ULONG action;
	STRPTR uri;
	ULONG flags;
};

struct notifyact_file_uid {
	ULONG action;
	STRPTR uri;
	ULONG uid;
};

struct notifyact_file_gid {
	ULONG action;
	STRPTR uri;
	ULONG gid;
};

struct notifyact_device_mount {
	ULONG action;
	STRPTR uri;
};

struct notifyact_device_unmount {
	ULONG action;
	STRPTR uri;
};

struct notifyact_device_name {
	ULONG action;
	STRPTR uri;
	STRPTR newname;
};

#endif /* AMBIENT_NOTIFY_H */
