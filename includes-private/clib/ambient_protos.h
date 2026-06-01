ULONG AmbientOpen(STRPTR path);
ULONG GetViews(ULONG nr,STRPTR buffer,ULONG buffer_size);
APTR AddPrefsPool(STRPTR file_name, ULONG type);
APTR LockPPool(STRPTR name,ULONG type);
ULONG UnLockPPool(APTR ppool_lock);
STRPTR NextPPoolName(APTR prev,ULONG type);

ULONG AddPPoolItem(APTR ctx, APTR pitem, ULONG id, CONST_APTR data, ULONG size);
ULONG ChangePPoolItemID(APTR ppool_lock,APTR pitem, ULONG id,ULONG new_id);
ULONG RemovePPoolItem(APTR ctx,APTR pitem, ULONG id);
ULONG GetPPoolItem(APTR ctx,APTR pitem, ULONG id,APTR *p, ULONG *size);
ULONG SavePPool(APTR ppool_lock,STRPTR old_name);
ULONG ReReadPPool(APTR ppool_lock);
ULONG OpenPrefs(STRPTR p);
ULONG StorageGet(ULONG id, ULONG type, APTR *data);
ULONG ExecuteURI(STRPTR path);