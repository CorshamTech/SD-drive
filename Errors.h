//=============================================================================
// Error codes are defined by the protocol, so here is the list of orricial
// error code values.
//
// These are defined in The Remote Disk Protocol Guide.

#ifndef __ERRORS_H__
#define __ERRORS_H__

// Standard error return values

#define ERR_NONE                0    // No error
#define ERR_NOT_MOUNTED        10    // No drive mounted
#define ERR_MOUNTED            11    // already mounted
#define ERR_FILE_NOT_FOUND     12
#define ERR_READ_ONLY          13
#define ERR_BAD_DRIVE          14
#define ERR_BAD_TRACK          15
#define ERR_BAD_SECTOR         16
#define ERR_READ_ERROR         17
#define ERR_WRITE_ERROR        18
#define ERR_DEVICE_NOT_PRESENT 19
#define ERR_NOT_IMPLEMENTED    20


#endif  // __ERRORS_H__

