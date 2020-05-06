//=============================================================================
// FILE: SdFuncs.h
//
// This contains some functions related to raw accesses of the SD drive.  It
// does not handle anything related to the mounted drive emulation.

#ifndef __SDFUNCS_H__
#define __SDFUNCS_H__

void sendDirectory();
void openFileForRead(Event *ep);
void nextDataBlock(Event *ep);
void openFileForWrite(Event *ep);
void writeBytes(Event *ep);
void closeFiles(void);

#endif  // __SDFUNCS_H__


