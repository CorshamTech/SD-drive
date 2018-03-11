//=============================================================================
//
// This class represents one virtual disk, using a DSK format file.  This class
// provides methods to read and write raw sectors, mount and umount DSK format
// files and other low-level functions.
//
// Bob Applegate, K2UT - bob@corshamtech.com

#ifndef __DISK_H__
#define __DISK_H__

#include <SD.h>
#include "Event.h"


// Max sector size supported.  Using Flex, the only sector size supported
// is 256 bytes.

#define SECTOR_SIZE  256


// The file name size is fixed by the Arduino SD library.  It only supports
// 8.3 format names.

#define FNAME_SIZE  12  // xxxxxxxx.xxx


class Disk
{
        public:
                Disk(void);
                ~Disk(void);
                bool isGood(void) { return goodFlag; }  
                bool read(unsigned long offset, byte *buf);
                bool write(unsigned long offset, byte *buf);
                char *getFilename(void) { return filename; }
                bool mount(char *afilename, bool readOnly);
                void unmount(void);
                bool isMounted(void) { return mountedFlag; }
                bool isOpen(void) { return isOpenF; }
                void close(void);
                byte getStatus(void);
                byte getError(void) { return errorCode; }
                bool isReadOnly(void) { return readOnlyFlag; }
        
        private:
                bool goodFlag;
                int openFlag;
                bool mountedFlag;
                bool isOpenF;
                bool readOnlyFlag;
                File file;
                void setError(byte err) { goodFlag = false; errorCode = err; }
                char filename[FNAME_SIZE + 1];
                byte errorCode;
};

#endif  // __DISK_H__


