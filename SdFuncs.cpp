//=============================================================================
// FILE: SdFuncs.cpp
//
// This contains some functions related to raw accesses of the SD drive.  It
// does not handle anything related to the mounted drive emulation.

#include <SPI.h>
#include <SD.h>
#include "link.h"
#include "SdFuncs.h"
#include "Errors.h"

extern Link *link;

// For file operations, have a file handy.  Anyone using the file should be
// darned sure it gets closed when done!

static File myFile;


//=============================================================================
// This sends a directory to the host.  On entry, it is assumed there is at
// least one Event available.  Ie, free it before calling this.  This sends
// only file names at the top level, not directories, and it does not
// recurse.

void sendDirectory()
{
        Event *eptr;
#ifdef USE_SDFAT
        SdFat sd;
        SdFile file;
        uint32_t pos = 0;
        char name[30];
#else
        File dir;
#endif
        
        bool go_on = true;

#ifdef DEBUG_DIR
        Serial.println("Sending disk directory...");
#endif
 
        // Open the root directory

#ifdef USE_SDFAT
        sd.chdir("/");
#else
        dir.close();
        dir = SD.open("/");
        dir.rewindDirectory();
        if (!dir.available())
        {
                Serial.println("DIR not available");
        }

        File entry;
#endif
                                
        while (go_on)
        {
#ifdef USE_SDFAT
                sd.vwd()->seekSet(pos);
                if (!file.openNext(sd.vwd(), O_READ))
                {
                        go_on = false;
                }
                else
                {
                        pos = sd.vwd()->curPosition();
                        if (file.isDir() == false)
                        {
                                file.getName(name, sizeof(name));
                                Serial.println(name);
                                byte *bptr = (byte *)name;
                                if (*bptr != '_') // filenames starting with underscore are deleted
                                {
                                        eptr = link->getAnEvent();
                                        eptr->clean(EVT_DIR_INFO);    // this is a directory entry

                                        while (*bptr)
                                        {
                                                eptr->addByte(*bptr++);
                                        }
                                        eptr->addByte(0);    // terminate the name
                                        link->sendEvent(eptr);
                                }
                        }
                        file.close();
                }
        }
        
#else
                entry =  dir.openNextFile();
                if (!entry)
                {
                        // no more files
                        go_on = false;
                }

                // Only process file names, not directories.

                if (entry && !entry.isDirectory())
                {                        
                        // Now copy the filename into the event and send it.
                        
                        byte *bptr = (byte *)(entry.name());
                        if (*bptr != '_') // filenames starting with underscore are deleted
                        {
                                eptr = link->getAnEvent();
                                eptr->clean(EVT_DIR_INFO);    // this is a directory entry
#ifdef DEBUG_DIR
                                Serial.print("   ");
                                Serial.println(entry.name());
#endif
                                while (*bptr)
                                {
                                        eptr->addByte(*bptr++);
                                }
                                eptr->addByte(0);    // terminate the name
                                link->sendEvent(eptr);
                        }
                }
                entry.close();
        }
        
        dir.close();
#endif
        
        // Send an event letting the host know the directory is done
        
        eptr->clean(EVT_DIR_END);
        link->sendEvent(eptr);
}




//=============================================================================
// Given an event with a EVT_TYPE_FILE type, verify the file can be read and
// send back either an ACK or NAK.

void openFileForRead(Event *ep)
{
        if (myFile)
        {
#ifdef DEBUG_FILE_READ
                Serial.println("typeFile found open file... closing");
#endif
                myFile.close();    // make sure an existing file is closed
        }

        // Attempt to open the file
                                
        myFile = SD.open((char *)(ep->getData()));
        if (myFile)
        {
                ep->clean(EVT_ACK);  // woohoo!
                myFile.seek(0);
        }
        else
        {
                Serial.print("typeFile: error opening ");
                Serial.println((char *)(ep->getData()));

                 ep->clean(EVT_NAK);
                 ep->addByte(ERR_FILE_NOT_FOUND);    // misc error
         }

         link->sendEvent(ep);
}




//=============================================================================
// This sends the next block of the open file.

void nextDataBlock(Event *ep)
{
        // They sent the maximum length they can handle.
        byte length = *(ep->getData());  // get maximum length

#ifdef DEBUG_FILE_READ
        Serial.print("Byte count: ");
        Serial.println(length);
#endif

        ep->clean(EVT_FILE_DATA);
                                
        // We're going to cheat a bit here.  Add a length of 0.
        // Once we have written the data and know the length,
        // go back and put the actual length into the buffer.
                           
        ep->addByte(0);    // length is first
                                
        byte actualCount = 0;

        while (myFile.available() && actualCount < length)
        {
                ep->addByte(myFile.read());
                actualCount++;
        }
                                
        // Now go back and put in the actual length
                                
        byte *bptr = ep->getData();  // get start of buffer
        *bptr = actualCount;    // and drop in the actual length
                                
        // If end of file, close the file

        if (actualCount == 0)
        {
#ifdef DEBUG_FILE_READ
                Serial.println("Reached EOF, closing file");
#endif
                myFile.close();
        }
#ifdef DEBUG_FILE_READ
                Serial.print("Actual bytes sent: ");
                Serial.println(actualCount);
#endif                      
        link->sendEvent(ep);
}




//=============================================================================
// This opens a file for writing.  Pass in the event with the filename.  This
// closes any open file, then opens the new one.

void openFileForWrite(Event *ep)
{
        // If there is an open file, close it.

        if (myFile)
                myFile.close();
                                
        byte *ptr = ep->getData();
#ifdef DEBUG_FILE_WRITE
        Serial.print("Got request to open a file for writing: \"");
        Serial.print((char *)ptr);
        Serial.print("\"");
#endif
        SD.remove((char *)(ep->getData()));   // remove existing file
        myFile = SD.open((char *)(ep->getData()), FILE_WRITE);
        if (myFile)
        {
#ifdef DEBUG_FILE_WRITE
                Serial.println(" - success");
#endif
                ep->clean(EVT_ACK);
        }
        else
        {
#ifdef DEBUG_FILE_WRITE
                Serial.println(" - FAILURE");
#endif
                ep->clean(EVT_NAK);
                ep->addByte(ERR_WRITE_ERROR);
        }
        link->sendEvent(ep);
}




//=============================================================================
// Given an event with bytes to write to an open disk file, do the writes.

void writeBytes(Event *ep)
{
        // The first byte is the length.  If zero then the length is actually 256,
        // else the length is the actual length.
                        
        byte *ptr = ep->getData();
        unsigned length = *ptr++;
        if (length == 0)
                length = 256;

#ifdef DEBUG_FILE_WRITE
        Serial.print("Writing ");
        Serial.print(length);
        Serial.println(" bytes to the raw file");
#endif

        // Now write the block of data
        if (myFile.write(ptr, length) == length)
        {
                ep->clean(EVT_ACK);     // send back an ACK.
        }
        else
        {
                Serial.println("Got a NAK!");
                ep->clean(EVT_NAK);
                ep->addByte(ERR_WRITE_ERROR);
        }
        myFile.flush();
        link->sendEvent(ep);
}




//=============================================================================
// Make sure files are closed.

void closeFiles(void)
{
        myFile.close();   // be sure it's closed
}
