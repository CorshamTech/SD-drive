//=============================================================================
// FILE: Disk.cpp
//
// This class represents one virtual disk, using a DSK format file.  This class
// provides methods to read and write raw sectors, mount and umount DSK format
// files and other low-level functions.
//
// Note that there is no caching here.  Writes immediately get written to the
// disk, and reads are always read directly from the disk.  This might be
// optimized a bit by delaying writes and doing them during idle polls, but
// I'll leave that to someone else.
//
// Bob Applegate, K2UT - bob@corshamtech.com

#include <SD.h>
#include "Disk.h"
#include "Errors.h"

extern void hexdump(unsigned char *, unsigned int);



// Define to dump sectors

#undef DUMP_SECTORS


//=============================================================================
// This creates an instance of the Disk but does not do any initialization.

Disk::Disk(void)
{
        mountedFlag = false;
        isOpenF = false;;
}




//=============================================================================
// Destructor.

Disk::~Disk(void)
{
        unmount();  // make sure it's unmounted
}




//=============================================================================
// Unmounts the disk currently mounted.  Does not produce an error if there is
// nothing mounted, but it does set the next error to be NOT MOUNTED for
// future status requests.

void Disk::unmount(void)
{
        if (mountedFlag)  // no sense unmounting if not mounted
        {
                file.close();
                isOpenF = false;;
                setError(ERR_NOT_MOUNTED);
                mountedFlag = false;
        }
}




//=============================================================================
// Given a pathname to a file, attempt to open it.  Returns true if mounted,
// false if not and the error flag is set with the reason.

bool Disk::mount(char *afilename, bool readOnly)
{
        goodFlag = false;    // assume it is not good
        //byte buffer[SECTOR_SIZE];
        
        Serial.print("Disk::Disk ");
        Serial.println(afilename);
      
        // Make sure the file exists!
        
        if (SD.exists(afilename))
        {
                // Set the right open flag depending on whether it's read-only
                // or not.
                
                if (readOnly)
                {
                        openFlag = FILE_READ;
                        Serial.println("opening read only");
                }
                else
                {
                        openFlag = FILE_WRITE;
                }

                // Open the file!
                
                file = SD.open(afilename, openFlag);
                readOnlyFlag = readOnly;
                
                if (!file)
                {
                        Serial.println("Error opening file!");
                        // should set an error code
                }
                else
                {
                        goodFlag = true;
                        mountedFlag = true;
                        isOpenF = true;
                
                        strcpy(filename, afilename);    // save name for later
                }
        }
        else
        {
                setError(ERR_FILE_NOT_FOUND);
        }
}




//=============================================================================
// Close a file

void Disk::close(void)
{
        file.close();
        isOpenF = false;
}




//=============================================================================
// Reads a sector of data.  On entry this is given the offset into the DSK
// file and a pointer to where to place the data.  This always reads exactly
// SECTOR_SIZE bytes.
//
// Returns true on success, false on error

bool Disk::read(unsigned long offset, byte *buf)
{
        bool ret = true;    // assume a good return code
        errorCode = ERR_NONE;
        
//  Serial.print("Disk::read at offset ");
//  Serial.println(offset, HEX);
//  Serial.flush();
 
#ifdef DUMP_SECTORS
        Serial.println("Sector dump:");
        byte *orig = buf;
#endif

        file.seek(offset);
        
        if ((file.available() < SECTOR_SIZE) || (offset + SECTOR_SIZE > file.size()))
        {
                Serial.print("Not enough bytes: ");
                Serial.println(file.available());
                ret = false;
                errorCode = ERR_READ_ERROR;
        }
        for (int i = 0; i < SECTOR_SIZE; i++)
        {
                *buf = file.read();
                buf++;
        }
        
#ifdef DUMP_SECTORS
        hexdump(orig, SECTOR_SIZE);
#endif
        return ret;
}




//=============================================================================
// This writes a single sector to the specified offset.  On entry, this is 
// given the long offset (must be a multiple of the sector size) and a pointer
// to exactly one sector's worth of data.  Does the write, then rReturns true
// on success or false on error.

bool Disk::write(unsigned long offset, byte *buf)
{
        bool ret = false;    // assume failure
        errorCode = ERR_NONE;
        
//        Serial.print("Disk::write(");
//        Serial.print(offset >> 16, HEX);
//        Serial.println(offset & 0xffff, HEX);
        
        if (readOnlyFlag)
        {
                errorCode = ERR_READ_ONLY;
        }
        else
        {
                file.seek(offset);
                if (file.available() < SECTOR_SIZE)
                {
                        Serial.print("Not enough bytes: ");
                        Serial.println(file.available());
                        errorCode = ERR_WRITE_ERROR;
                }
                else
                {
                        // Write the data and then flush it to be sure the
                        // data gets written.
                        
                        int wrote = file.write(buf, SECTOR_SIZE);
                        file.flush();
                   
                        if (wrote != SECTOR_SIZE)
                        {
                                Serial.print("Didn't write enough bytes: ");
                                Serial.println(wrote);
                                errorCode = ERR_WRITE_ERROR;
                        }
                        else
                        {
                                ret = true;    // success!
//                                Serial.print("Just wrote ");
//                                Serial.print(wrote);
//                                Serial.println(" bytes");
                        }
                }
        }
        return ret;
}



//=============================================================================
// This returns (for now) a single byte indicating the disk status via a
// bitmap:
//
// Bit
//  0: 0 = disk not present, 1 = mounted
//  1: 0 = read only, 1 = R/W
//  2: 0 = sector readable, 1 = sector unreadable
//  3: 
//  4: 
//  5: 
//  6: 
//  7: 
//
// Generally speaking, a value of 0 means no problems.

byte Disk::getStatus(void)
{
        byte ret = 0;
        
        if (mountedFlag)
        {
                ret |= 0x01;

                if (readOnlyFlag)
                {
                        ret |= 0x02;
                }
        }
                
        return ret;
}


