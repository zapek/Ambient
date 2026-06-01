ULONG PanelPrefsApp(APTR app,APTR obj);
ULONG PanelHasChanged(STRPTR panel_name,ULONG mode, ULONG pos1, ULONG pos2);
ULONG PrefsPoolUpdate(STRPTR panel_name,ULONG object_id,ULONG prefs_id);
STRPTR NextPanelName(STRPTR lastname);
APTR LockPanel(STRPTR name);
ULONG UnLockPanel(APTR panel_lock);
STRPTR AddPanel(STRPTR);
ULONG SavePanel(APTR panel_lock,STRPTR old_name);
ULONG CreatePanelItem(ULONG type_ID,  STRPTR class_name);
ULONG AddPanelItem(APTR panel_lock, APTR pitem, ULONG id, CONST_APTR data, ULONG size);
ULONG RemovePanelItem(APTR panel_lock,APTR pitem, ULONG id);
ULONG SetPanelObjectAttr(APTR pobj,ULONG ti_Tag, ULONG ti_Data);
ULONG GetPanelObjectAttr(ULONG Tag,APTR pobj,ULONG *storage);
ULONG NewPanel(STRPTR p_name);
ULONG RemovePanelObject(APTR panel_object);
ULONG InsertPanelObject(APTR panel_win,APTR panel_object,LONG pos);
ULONG MovePanelObject(APTR panel_win,ULONG src_id,ULONG target_id,LONG pos);
ULONG SaveAll(void);
ULONG ReLoadPanels(void);
void AddExternClass(STRPTR path,STRPTR class_name);
STRPTR NextExternClass(STRPTR lastname);
APTR ObtainPanelObject(APTR panel_win,struct TagItem  *tags);
void ReleasePanelObject(APTR panelobject);
APTR AllocPanelBitMap(ULONG width,ULONG height);
void FreePanelBitMap(APTR bitmap);
ULONG SetPanelObjectBitMap(APTR panelobject,struct BitMap *bm,ULONG width,ULONG height);
ULONG PanelMode(APTR panel_lock,ULONG modus);





