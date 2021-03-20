//=============================================================================
// This class handles communications with the master system.  This particular
// implementation uses a set of shared I/O lines.  Three lines are dedicated
// handshake lines, and eight are bi-directional data lines.  This isn't a
// sophisticated protocol at all, and can be improved.
//
// The data in this file must comply with The Remote Disk Protocol Guide.
//
// August 2014 - Bob Applegate, bob@corshamtech.com
//
// March 2021 - Changed filename to Link.h

#ifndef __LINK_H__
#define __LINK_H__

#include "Event.h"
#include "UserInt.h"


// These are the command codes and results defined as the interface between
// the host and the Arduino

#define PROTO_GET_VERSION  0x01
#define PROTO_PING   0x05
#define PROTO_LED_CONTROL  0x06
#define PROTO_GET_CLOCK  0x07
#define PROTO_SET_CLOCK  0x08
#define PROTO_GET_DIR  0x10
#define PROTO_GET_MOUNTED_LIST  0x11
#define PROTO_MOUNT  0x12
#define PROTO_UNMOUNT  0x13
#define PROTO_GET_STATUS 0x14
#define PROTO_DONE  0x15
#define PROTO_ABORT  0x15
#define PROTO_READ_FILE  0x16
#define PROTO_READ_BYTES  0x17
#define PROTO_READ_SECTOR  0x18
#define PROTO_WRITE_SECTOR  0x19
#define PROTO_GET_MAX_DRIVES  0x1a
#define PROTO_WRITE_FILE  0x1b
#define PROTO_WRITE_BYTES 0x1c
#define PROTO_SAVE_CONFIG 0x1d
#define PROTO_SET_TIMER 0x1e
#define PROTO_READ_SECTOR_LONG 0x1f
#define PROTO_WRITE_SECTOR_LONG 0x20

#define PROTO_VERSION  0x81
#define PROTO_ACK    0x82
#define PROTO_NAK    0x83
#define PROTO_PONG   0x85
#define PROTO_CLOCK_DATA 0x87
#define PROTO_DIR    0x90
#define PROTO_DIR_END  0x91
#define PROTO_FILE_DATA  0x92
#define PROTO_STATUS  0x93
#define PROTO_SECTOR_DATA  0x94
#define PROTO_MOUNT_INFO  0x95




class Link
{
        public:
                Link(void);
                ~Link(void);
                bool poll(void);
                void begin(void);
                void prepareRead(void);
                void prepareWrite(void);
                void writeByte(byte data);
                byte readByte(void);
                bool waitingEvent(void) { return hasEvent; }
                Event *getEvent(void);
                void sendEvent(Event *ep);
                Event *getAnEvent(void);
                void freeAnEvent(Event *eptr);
                     
        private:
                bool hasEvent;
                byte assembleByte(void);
                void disassembleByte(byte raw);
                void stateMachine(word token);
                Event *event;
                Event *freeEvent;
                UserInt *uInt;
};

#endif    // __LINK_H__
