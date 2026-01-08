#pragma once
#include <exec/types.h>

BOOL is_networksfs(CONST_STRPTR path);
void networksfs_settings(CONST_STRPTR path);
void networksfs_connect(void);

ULONG icon_flags_for_path(CONST_STRPTR path, ULONG type);
