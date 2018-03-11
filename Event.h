//=============================================================================
// To decouple the low level communication protocol from the higher level view
// of what needs to get done, all commands and response are passed around as
// Events.  A lower level class converts protocol messages to/from events.
//
// Note that events have a buffer, and buffers consume the little bit of RAM
// present in an Arduino, so minimize the number of Events allocated or else
// memory might be a problem.
//
// Bob Applegate, K2UT - bob@corshamtech.com

#ifndef __EVENT_H__
#define __EVENT_H__

#include "Arduino.h"

// These are the recognized event types.  A given event type can go to and/or
// from the host and the higher levels, so there is no indication in the name
// of which direction each message type is going.

typedef enum
{
        EVT_NONE = 0,
        EVT_GET_VERSION,
        EVT_VERSION_INFO,
        EVT_PING,
        EVT_DONE,
        EVT_CONTROL_LED,
        EVT_TYPE_FILE,
        EVT_ACK,
        EVT_NAK,
        EVT_FILE_DATA,
        EVT_SEND_DATA,
        EVT_GET_DIRECTORY,
        EVT_DIR_INFO,
        EVT_DIR_END,
        EVT_GET_MOUNTED,
        EVT_MOUNTED,
        EVT_MOUNT,
        EVT_UNMOUNT,
        EVT_READ_SECTOR,
        EVT_READ_SECTOR_LONG,
        EVT_WRITE_SECTOR,
        EVT_WRITE_SECTOR_LONG,
        EVT_GET_STATUS,
        EVT_DISK_STATUS,
        EVT_GET_CLOCK,
        EVT_CLOCK_DATA,
        EVT_SET_CLOCK,
        EVT_WRITE_FILE,
        EVT_WRITE_BYTES,
        EVT_SAVE_CONFIG,
        EVT_SET_TIMER,
} EVENT_TYPE;





// Events have a buffer for user data, so this sets the size.  Bear in mind that the
// Arduino has very limited amounts of RAM, so don't make this any larger than
// necessary!  Sectors are 256 bytes so thats the value I chose.

#define BUFFER_SIZE  (256+10)

class Event
{
        public:
                Event(void);
                ~Event(void);
                EVENT_TYPE getType(void) { return type; }
                void addByte(byte data);
                unsigned getLength(void) { return index; }
                byte *getData(void) { return buffer; }
                void clearData(void) { index = 0; }
                void setType(EVENT_TYPE ntype) { type = ntype; clearData(); }
                void clean(void);
                void clean(EVENT_TYPE atype);
        
        private:
                EVENT_TYPE type;
                byte buffer[BUFFER_SIZE];
                unsigned index;  // index into buffer
};

#endif    // __EVENT_H__

