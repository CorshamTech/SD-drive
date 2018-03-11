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

#include "Event.h"



//=============================================================================
// Constructor does very little.

Event::Event(void)
{
        type = EVT_NONE;
        index = 0;    // no data in buffer yet
}




//=============================================================================
// Destructor.

Event::~Event(void)
{
}




//=============================================================================
// Some events have data associated with them, such as file contents, filenames,
// etc.  This method is used to add a byte to the message contents of the
// Event.

void Event::addByte(byte data)
{
        // Make sure we aren't about to exceed the buffer size
        
        if (index == BUFFER_SIZE)
        {
        }
        else
        {
                buffer[index++] = data;
        }
}



//=============================================================================
// This cleans up an event by removing all old data, clearing the type, etc.

void Event::clean(void)
{
        type = EVT_NONE;
        index = 0;
}



//=============================================================================
// This assigns a new event type to this event and also clears out any old
// data.

void Event::clean(EVENT_TYPE atype)
{
        type = atype;
        index = 0;
}

                
