//=============================================================================
// This is a collection of Disk objects.  The drive emulator isn't much use if
// only a single drive wer available, so this class maintains a collection of
// the Drive objects.  It also provides a standard interface for requesting
// services for a given virtual drive.
//
// Bob Applegate - K2UT, bob@corshamtech.com

#ifndef __DISKS_H__
#define __DISKS_H__

#include "Disk.h"
#include "UserInt.h"


// Sets the number of drives supported.  This depends on the OS, but FLEX
// only supports four.

#define MAX_DISKS  4


// Pin used by the SD card

#define SD_PIN  53


// Reading the config file involves a very simple state machine.
// These are the possible states.

typedef enum
{
        FIRST_CHAR,
        AFTER_DRIVE,
        WAIT_EOL,
        FILENAME,
} configState_t;

enum
{
        CONFIG_FILE_PRIMARY,
        CONFIG_FILE_ALTERNATE,
};


class Disks
{
        public:
                Disks(void);
                ~Disks(void);
                bool saveConfig(void);
                bool mount(byte drive, char *filename, bool readOnly);
                bool unmount(byte drive);
                void mountDefaults(void) { mountDefaults(CONFIG_FILE_PRIMARY); }
                void mountDefaults(int which);
                void closeAll(void);
                bool read(byte drive, unsigned long offset, byte *buf);
                bool write(byte drive, unsigned long offset, byte *buf);
                void poll(void);
                byte getStatus(byte drive);
                byte getErrorCode(void) { return errorCode; }
                bool isDriveValid(byte drive) { return (drive < MAX_DISKS); }
                bool isReadOnly(byte drive) { return (disks[drive]->isReadOnly()); }
                bool isMounted(byte drive) { return (disks[drive]->isMounted()); }
                char *getFilename(byte drive) { return disks[drive]->getFilename(); }
                bool format(char *filename, int tracks, int sectors, byte fillPattern);
                
        private:
                Disk *disks[MAX_DISKS];
                byte errorCode;
                void freeRam();
                configState_t state;
                bool readOnly;
                char filename[13];
                char *fnptr;
                int drive;
                File file;
                bool presentState;
                UserInt *userInt;
                int whichConfigFile;
                const char *configFileName;
                
                void setError(byte code) { errorCode = code; }
};


#endif  // __DISKS_H__
