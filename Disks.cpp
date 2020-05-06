//=============================================================================
// This is a collection of Disk objects.  The drive emulator isn't much use if
// only a single drive wer available, so this class maintains a collection of
// the Drive objects.  It also provides a standard interface for requesting
// services for a given virtual drive.
//
// Bob Applegate - K2UT, bob@corshamtech.com

#include <SD.h>
#include "Disks.h"
#include "Errors.h"

// This is the configuration file that is read to get the initial files
// to mount, and the backup copy when a change is saved.

#define CONFIG_FILE  "SD.CFG"
#define CONFIG_FILE_ALT "SD2.CFG"
#define CONFIG_BACKUP_FILE  "SD.OLD"

extern bool debounceInputPin(int pin);

// Pin with the presence sensor

#define PRESENCE_PIN 19


//=============================================================================
// The constructor prepares for mounting drives.  It creates instances of
// Disk for each possible drive.

Disks::Disks(void)
{
        SD.begin(SD_PIN);

        // Create the Disk objects
        
        for (int d = 0; d < MAX_DISKS; d++)
        {
                disks[d] = new Disk();
        }
        state = FIRST_CHAR;   // for reading the config file

        userInt = UserInt::getInstance();

        pinMode(PRESENCE_PIN, INPUT);   // pin with presence bit
        presentState = false;     // assume an SD card is there
}




//=============================================================================
// The destructor.  This is never called, so don't bother doing anything.

Disks::~Disks(void)
{
}




//=============================================================================
// This gets called every 100ms for whatever needs to be done.  Currently this
// just monitors for SD card removals and insertions.

void Disks::poll(void)
{
        static bool last = false;

        // Get current state of the present bit

        bool state = debounceInputPin(PRESENCE_PIN);

        if (state != presentState)
        {
                // Finally, we can do the appropriate
                // action based on whether the card is
                // now present or not.

                if (state == true)
                {
                        Serial.println("Disks::poll detected card removal");
                        userInt->sendEvent(UI_SD_REMOVED);

                        //tell all disks to close/unmount
                        closeAll();
                }
                else
                {
                        Serial.println("Disks::poll detected card insertion");
                        userInt->sendEvent(UI_SD_INSERTED);
                        SD.begin(SD_PIN);

                        //Mount all default drives
                        mountDefaults(whichConfigFile);
                }
                presentState = state;
        }
}




//=============================================================================
// This mounts the default drives
//
// The config file is really primitive, and there is little error checking, so
// please be sure to follow the examples EXACTLY as specified.  Don't insert
// extra whitespace, etc.
//
//    # comments
//    x:filename.ext
//    xR:filename.ext
//
// Where 'r' is a digit from 0 to 3, R (if present) indicates read-only.
//
// Example:
//
//    0:SD_BOOT.DSK
//    1:CT_UTILS.DSK
//    2R:DANGER.DSK
//    3:PLAY.DSK

void Disks::mountDefaults(int which)
{
        configFileName = CONFIG_FILE;
        whichConfigFile = which;
        if (which == CONFIG_FILE_ALTERNATE)
        {
                configFileName = CONFIG_FILE_ALT;
        }

        if (debounceInputPin(PRESENCE_PIN))
        {
                Serial.println("No card inserted");
                return;
        }
        
        Serial.print("Reading configuration file ");
        Serial.println(configFileName);

        // Now open and read the configuration file to see which drives
        // we should mount by default.

        if (SD.exists(configFileName))
        {
                file = SD.open(configFileName, FILE_READ);
                if (!file)
                {
                        Serial.println("failed to open config file");
                        return;
                }

                // This is a crude little state machine that processes each
                // character from the config file.
                
                state = FIRST_CHAR;
                while (file.available())
                {
                        char token = file.read();   // get one character from file
#if 0
                        Serial.print("Got char '");
                        Serial.print(token);
                        Serial.print("' state ");
                        Serial.println(state);
#endif

                        switch (state)
                        {
                                case FIRST_CHAR:    // get first character of line
                                        if (token == '#')    // comment
                                        {
                                                state = WAIT_EOL;   // wait for end of line
                                        }
                                        else if (token >= '0' && token <= '3')  // drive
                                        {
                                                drive = token - '0';  // compute drive
                                                readOnly = false;
                                                state = AFTER_DRIVE;
                                        }
                                        break;
                                               
                                case WAIT_EOL:
                                        // In this state, keep reading characters
                                        // until an EOL is found.
                                        
                                        if (token == '\n')
                                                state = FIRST_CHAR;
                                        break;
           
                                case AFTER_DRIVE:    // should be either R or :
                                        if (token == ':')
                                        {
                                                state = FILENAME;
                                                fnptr = filename;
                                        }
                                        else if (token == 'R' || token == 'r')
                                        {
                                                readOnly = true;
                                        }
                                        break;
                                
                                case FILENAME:
                                        if (token == '\n')
                                        {
                                                state = FIRST_CHAR;
                                                *fnptr = '\0';    // terminate the filename
                                                mount(drive, filename, readOnly);
                                        }
                                        else if (token > ' ' && token < '~')
                                        {
                                                *fnptr++ = token;
                                        }
                                        break;
                        }
                }
                file.close();
        }
        else
        {
                Serial.print("Config file not found: ");
                Serial.println(configFileName);
        }
}




//=============================================================================
// This closes any open files.  This is kind of an emergency sort of function
// used to close open files in case another piece of code needs to open a file.

void Disks::closeAll(void)
{
        for (int d = 0; d < MAX_DISKS; d++)
        {
                if (disks[d]->isOpen())
                {
                        disks[d]->close();
                }
        }
}




//=============================================================================
// This saves the current configuration to the default config file.  Also saves
// the current file.  Returns true on success, false if didn't save.

enum
{
  STATE_NEWLINE,
  STATE_COPY_LINE,
  STATE_SKIP_LINE,
};

bool Disks::saveConfig(void)
{
        bool ret = false;
        File ofile;
        int state = STATE_NEWLINE;
        int d;
        int written[MAX_DISKS];

        Serial.print("Writing configuration file ");
        Serial.println(file);

        // There is no RENAME function in this SD card library, so to make a backup copy
        // requires that the existing file be opened, a new output file created, and
        // the contents copied from one to the other.

        SD.remove(CONFIG_BACKUP_FILE);      // remove old backup
        file = SD.open(configFileName, FILE_READ);
        ofile = SD.open(CONFIG_BACKUP_FILE, FILE_WRITE);
        if (!ofile || !file)
        {
                Serial.print("Failed copying file");
                file.close();
                ofile.close();
                return ret;
        }

        while (file.available())
        {
          ofile.write(file.read());
        }

        ofile.close();
        file.close();

        SD.remove(configFileName);   // remove the existing config file

        // We now have a backup.

        file = SD.open(CONFIG_BACKUP_FILE, FILE_READ);  // original file
        ofile = SD.open(configFileName, FILE_WRITE);       // new version
        if (!ofile || !file)
        {
                Serial.println("failed to open config file for updating");
        }
        else
        {
                // Clear the written flags because no drive has been written out yet.
                
                for (d = 0; d < MAX_DISKS; d++)
                {
                        written[d] = 0;
                }
                
                while (file.available())
                {
                        char key = file.read();   // get next character

                        switch (state)
                        {
                                case STATE_NEWLINE:   // handles first character on line
                                        if (key >= '0' && key < MAX_DISKS+'0')
                                        {
                                                // replace old drive info with current info

                                                d = key - '0';
                                                if (disks[d]->isOpen())
                                                {
                                                        ofile.print(d);
                                                        ofile.print(":");
                                                        ofile.println(disks[d]->getFilename());
                                                        written[d] = 1;
                                                }
                                                state = STATE_SKIP_LINE;  // skip rest of line
                                        }
                                        else
                                        {
                                                ofile.write(key);
                                                state = STATE_COPY_LINE;  // copy rest of line
                                        }
                                        break;

                                case STATE_COPY_LINE:
                                        ofile.write(key);   // then fall through

                                case STATE_SKIP_LINE:
                                        if (key == '\n')
                                                state = STATE_NEWLINE;
                                        break;
                        }
                }

                // Now see if there are any mounted drives that aren't written

                for (d = 0; d < MAX_DISKS; d++)
                {
                        if (written[d] == 0 && disks[d]->isOpen())
                        {
                                ofile.print(d);
                                ofile.print(":");
                                ofile.println(disks[d]->getFilename());
                        }
                }
        }
        ofile.close();
        file.close();
                
        return ret;
}




//=============================================================================
// This is called to mount a disk image to one of the drives.  
// Returns false on error

bool Disks::mount(byte drive, char *filename, bool readOnly)
{
        bool ret = false;    // assume no error
        
        Serial.print("Got mount request for drive ");
        Serial.print(drive);
        Serial.print(": \"");
        Serial.print(filename);
        Serial.print("\"");
        if (readOnly)
                Serial.print(" - read only");
        Serial.println("");
        
        disks[drive]->mount(filename, readOnly);
        if (disks[drive]->isGood())
        {
                ret = true;
                Serial.println(" - SUCCESS!");
        }
        else
        {
                setError(disks[drive]->getError());    // move their error into our error code
                Serial.print(" - FAILED!  Error code ");
                Serial.println(errorCode);
        }
        
        return ret;
}




//=============================================================================
// Unmount just one drive, the number being passed in.  Returns true on error
// false if not.

bool Disks::unmount(byte drive)
{
        bool ret = false;    // assume no error
        
        disks[drive]->unmount();
        
        return ret;
}




//=============================================================================
// Read a sector.  On entry this is given the drive (0-3), the offset from the
// start of the file, and a pointer to a buffer large enought to receive the
// data.  Returns true if no error, false if error

bool Disks::read(byte drive, unsigned long offset, byte *buf)
{
        bool ret = false;
        
        // Is the drive even mounted?
        
        if (disks[drive]->isMounted())
        {
                if (disks[drive]->read(offset, buf))
                {
                        ret = true;
                        errorCode = ERR_NONE;
                }
                else    // error
                {
                        Serial.println("**** read error ****");
                        Serial.flush();
                        errorCode = disks[drive]->getError();
                }
        }
        else
        {
                errorCode = ERR_NOT_MOUNTED;
        }

        return ret;
}




//=============================================================================
// Write a sector.  On entry this is given the drive (0-3), the offset from the
// start of the file, and a pointer to the buffer with the data.  Returns true
// if successful, false if not.

bool Disks::write(byte drive, unsigned long offset, byte *buf)
{
        bool ret = false;
        
        // Is the drive even mounted?
        
        if (disks[drive]->isMounted())
        {
                if (disks[drive]->write(offset, buf))
                {
                        ret = true;
                        errorCode = ERR_NONE;
                }
                else    // error
                {
                        Serial.println("**** write error ****");
                        errorCode = disks[drive]->getError();
                }
        }
        else
        {
                errorCode = ERR_NOT_MOUNTED;
        }

        return ret;
}



//=============================================================================
// Returns the status of a particular drive.

byte Disks::getStatus(byte drive)
{
        return disks[drive]->getStatus();
}





