#ifndef LIBRARIES_TRAYPLUGIN_H
#define LIBRARIES_TRAYPLUGIN_H

/*
        trayplugin include

        Copyright © 2005 Adam Waldenberg, OnyxSoft, All Rights Reserved.
*/

#ifndef EXEC_LIBRARIES_H
# include <exec/libraries.h>
#endif

#if defined(__GNUC__)
# pragma pack(4)
#endif

struct TrayBase {
        struct Library  base; 
};

#if defined(__GNUC__)
# pragma pack()
#endif

#endif /* LIBRARIES_TRAYPLUGIN_H */
