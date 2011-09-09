//
// Basic defines to test large file creation and access
// $Id: lfs.h,v 1.1 2008-03-27 22:19:27 crueda Exp $
//
// Include this file BEFORE any other
//

#define _FILE_OFFSET_BITS 64

#if defined WIN32
        #define my_OFF_T off64_t
        #define my_FSEEK fseeko64
        const char* lfs_logmsg = "windows env detected: Using off64_t and fseeko64";
#else
        #define my_OFF_T off_t
        #define my_FSEEK fseeko
        const char* lfs_logmsg = "Using off_t and fseeko";
#endif
