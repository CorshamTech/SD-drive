//=============================================================================
// This class handles communications with the master system.  This particular
// implementation uses a set of shared I/O lines.  Three lines are dedicated
// handshake lines, and eight are bi-directional data lines.  This isn't a
// sophisticated protocol at all, and can be improved.  Note that this was
// the second piece of code, after the main loop, that was written, so has the
// oldest, least sophisticated, least-RAM using, and therefore most prime for
// improvements code.
//
// This is the only class that knows anything about the low level transport
// mechanism, everything else deals with Event objects.
//
// This class also maintains the list of free Events.  This should probably
// be moved into another class, but works fine here for this simplistic
// system.
//
// August 2014 - Bob Applegate, bob@corshamtech.com
//
//=============================================================================
// Ports and how they're used.  Note that we operate on whole ports for some
// things, bits for others.
//
// This is a very basic communocation protocol using three control bits and
// eight data bits.
// 
// DIRECTION - High if master controls the bus, low if we do.
// STROBE    - From the master.  Indicates either data is available if host
//             is sending, or ACK if we're sending.
// ACK       - To the master.  This is our ACK when the master is sending us
//             data, or a strobe to the host when we're sending data.


// This is a very basic mapping of ports between the processors
//
// 653x  6821   Arduino   Use
//              d0  pd0   rx
//              d1  pd1   tx
//       B0     d2  pd2   DIRECTION
//       B1     d3  pd3   STROBE from 6821
//       B2     d4  pd4   ACK to 6821
//              d5  pd5
//       A6     d6  pd6   DATA 6
//       A7     d7  pd7   DATA 7
//              d8  pb0
//              d9  pb1
//              d10 pb2   Select for I2C?
//              d11 pb3   mosi
//              d12 pb4   miso
//              d13 pb5   sck
//       A0     a0  pc0   DATA 0
//       A1     a1  pc1   DATA 1
//       A2     a2  pc2   DATA 2
//       A3     a3  pc3   DATA 3
//       A4     a4  pc4   DATA 4
//       A5     a5  pc5   DATA 5

#include "Link.h"
#include <Arduino.h>


extern unsigned getSectorSize(byte code);

#define PROTOCOL_VERSION 1

extern bool debounceInputPin(int pin);


//=============================================================================
// Define all the ports in symbolic terms so it'll be easy to move ports/pins
// in the future.

#define LOWER_WRITE  PORTC
#define LOWER_READ PINC
#define LOWER_DDR DDRC
#define LOWER_MASK  0xff
//#define LOWER_MASK 0x3f

#define UPPER_WRITE  PORTD
#define UPPER_READ PIND
#define UPPER_DDR  DDRD
#define UPPER_MASK  0xc0

#define DIRECTION 47
#define STROBE 48
#define ACK 49



// The possible states for the inbound state machine:

typedef enum
{
        STATE_CMD = 1,    // waiting for a command
        STATE_WAIT_NULL,  // get bytes until a NULL
        STATE_GET_ONE,    // get exactly one byte of data
        STATE_GET_TWO,
        STATE_GET_THREE,
        STATE_GET_FOUR,
        STATE_GET_FIVE,
        STATE_GET_SIX,
        STATE_GET_DRV_NUMBER_TO_MOUNT,
        STATE_GET_DRV_NAME,  // get drive number
        STATE_APPEND_SECTOR, // add sector data to end
        STATE_GET_LENGTH,
} STATE;



//=============================================================================
// Constructor.  This does basically nothing, as the hardware gets set up in
// another method.

Link::Link(void)
{
}




//=============================================================================
// This does the start-up work and must be called before any other methods in
// this class.

void Link::begin(void)
{
        hasEvent = false;
        
        // Set the ACK to output, DIRECTION and STROBE to input
        
        digitalWrite(ACK, LOW);
        pinMode(DIRECTION, INPUT);
        pinMode(STROBE, INPUT);
        pinMode(ACK, OUTPUT);
        
        // The slave always starts in READ mode...
        
        prepareRead();
        
        Serial.println("LINK is initialized");
        
        // Make sure we've got an event
        
        freeEvent = new Event();
}




//=============================================================================

Link::~Link(void)
{
        Serial.println("LINK destructor called");
}




//=============================================================================
// This is the polling function.  It looks or data from the master, assembles
// the data into messages and sets the hasEvent flag if a complete message has
// arrived.
//
// Returns true if there is an event waiting to be processed, false it not.

bool Link::poll(void)
{
        word data;
  
        // Strobe goes high if the host has put data on the data pins.  Something
        // to consider for a future fix is a timeout here.  If the STROBE line is
        // floating then the code might get stuck here forever waiting for a byte
        // to arrive.
        
        if (debounceInputPin(STROBE))
        {
                // There is a strobe, so get the byte from the host and
                // then send it to the state machine for processing.
                
                data = readByte();
                
                //Serial.print("Got byte: ");
                //Serial.println((byte)data, HEX);

                // Let the state machine process the byte of data.
                
                stateMachine(data);
        }

        return hasEvent;
}




//=============================================================================
// This sets the data bits to an input state in preparation for reading data
// from the master.

void Link::prepareRead(void)
{
        LOWER_DDR = (~LOWER_MASK) & 0xff;
}




//=============================================================================
// This sets the data bits to the output state in preparation for writing data
// to the host.  This will not set the bits to drive unless the host has
// indicated it has turned off its drivers and is in input mode.

void Link::prepareWrite(void)
{
        // Before setting the data bits to output, make sure the other
        // side has indicating it's in read mode or else we might have
        // both drivers fighting each other.
        
        while (debounceInputPin(DIRECTION))
                ;

        LOWER_DDR = LOWER_MASK;
}




//=============================================================================
// Write a single byte to the master.  Assumes prepareForWrite() has been
// called already.  This does all the necessary handshaking.

void Link::writeByte(byte data)
{
        // Put the byte onto the data port
        
        LOWER_WRITE = data;
                
        // raise ACK to indicate data is present, then wait for
        // strobe to go high
                
        digitalWrite(ACK, HIGH);
        while (debounceInputPin(STROBE) == LOW)
                ;
                    
        digitalWrite(ACK, LOW);
        while (debounceInputPin(STROBE) == HIGH);
                ;
}




//=============================================================================
// This gets a single byte from the master using all the proper handshaking.

byte Link::readByte(void)
{
        byte data;
        
        // Wait for STROBE to go high, indicating a byte is ready.
        
        while (debounceInputPin(STROBE) == LOW)
                ;
                
        // Data is available, so grab it right away, then ACK it.
                
        data = LOWER_READ;
        digitalWrite(ACK, HIGH);
                
        // Wait for host to lower strobe
                
        while (debounceInputPin(STROBE))
                ;
                        
        // Lower ACK and we're done.
                
        digitalWrite(ACK, LOW);
        return data;
}




//=============================================================================
// This is used to get the next event waiting, or NULL if there is none.

Event *Link::getEvent(void)
{
        Event *eptr = event;    // temp save of event
        event = NULL;           // indicate no more waiting
        hasEvent = false;       // so its clear there are no more
        return eptr;
}




//=============================================================================
// This is the state machine for incoming messages from the master.  This is
// given one token at a time.  It can set event to point to a new event, and
// will set hasEvent when an event is ready to be read.

void Link::stateMachine(word token)
{
        static STATE state = STATE_CMD;
        bool transactionDone = false;  // set true if this is end of transaction
        static unsigned int count;

        //Serial.println((byte)token, HEX);
        switch (state)
        {
                case STATE_CMD:
                        // This is a command byte.
                        
                        uInt->sendEvent(UI_TRANSACTION_START);
                        switch (token)
                        {
                                case PROTO_VERSION:
                                        prepareWrite();
                                        writeByte(PROTO_VERSION);  // reply
                                        writeByte(PROTOCOL_VERSION);
                                        prepareRead();
                                        transactionDone = true;
                                        break;
                                        
                                case PROTO_PING:    // ping request
                                        // Pings are handled immediately,
                                        // not sent to the main loop.
                                        
                                        Serial.println("Got PING");
                                        prepareWrite();
                                        writeByte(PROTO_PONG);  // reply
                                        prepareRead();
                                        transactionDone = true;
                                        break;

#if 0                                        
                                case PROTO_LED_ON:  // turn on LED
                                        event = getAnEvent();
                                        event->clean(EVT_LED_ON);
                                        hasEvent = true;
                                        transactionDone = true;
                                        break;
                                        
                                case PROTO_LED_OFF:  // turn off LED
                                        event = getAnEvent();
                                        event->clean(EVT_LED_OFF);
                                        hasEvent = true;
                                        transactionDone = true;
                                        break;
#endif

                                case PROTO_READ_FILE:
                                        event = getAnEvent();
                                        event->clean(EVT_TYPE_FILE);
                                        state = STATE_WAIT_NULL;
                                        hasEvent = false;
                                        break;
                                        
                                case PROTO_READ_BYTES:
                                        event = getAnEvent();
                                        event->clean(EVT_SEND_DATA);
                                        state = STATE_GET_ONE;
                                        hasEvent = false;
                                        break;
                                        
                                case PROTO_GET_DIR:
                                        event = getAnEvent();
                                        event->clean(EVT_GET_DIRECTORY);
                                        hasEvent = true;
                                        break;
                                        
                                case PROTO_MOUNT:    // mount a drive
                                        Serial.println("Got a MOUNT");
                                        // Next is the drive number, then the read-only
                                        // flag and then the filename to mount
                                        event = getAnEvent();
                                        event->clean(EVT_MOUNT);
                                        hasEvent = false;
                                        state = STATE_GET_DRV_NUMBER_TO_MOUNT;
                                        break;
                                        
                                case PROTO_UNMOUNT:  // unmount a drive
                                        Serial.println("Got an UNMOUNT");
                                        event = getAnEvent();
                                        event->clean(EVT_UNMOUNT);
                                        hasEvent = false;
                                        state = STATE_GET_ONE;  // add the drive
                                        break;
                                        
                                case PROTO_READ_SECTOR:
                                        // This is followed by five more bytes:
                                        // (1) Drive (zero based)
                                        // (2) Sector size (1 = 128, 2 = 256, 3 = 512, 4 = 1024)
                                        // (3) Track (zero based)
                                        // (4) Sector (zero based)
                                        // (5) Number of sectors per track, one based
                                        
                                        event = getAnEvent();
                                        event->clean(EVT_READ_SECTOR);
                                        state = STATE_GET_FIVE;
                                        break;

                                case PROTO_READ_SECTOR_LONG:
                                        // This is followed by five more bytes:
                                        // (1) Drive (zero based)
                                        // (2) Sector size (1 = 128, 2 = 256, 3 = 512, 4 = 1024)
                                        // (3) Sector # MSB - zero based
                                        // (4) Sector #
                                        // (5) Sector #
                                        // (6) Sector # LSB
                                        
                                        event = getAnEvent();
                                        event->clean(EVT_READ_SECTOR_LONG);
                                        state = STATE_GET_SIX;
                                        break;
                                        
                                case PROTO_WRITE_SECTOR:
                                        // This is followed by five more bytes:
                                        // (1) Drive (zero based)
                                        // (2) Sector size (1 = 128, 2 = 256, 3 = 512, 4 = 1024)
                                        // (3) Track (zero based)
                                        // (4) Sector (zero based)
                                        // (5) Number of sectors per track, one based
                                        //
                                        // ...and then the sector data
                                        
                                        event = getAnEvent();
                                        event->clean(EVT_WRITE_SECTOR);
                                        state = STATE_GET_FIVE;
                                        count = 0;  // no bytes received yet
                                        break;

                                case PROTO_WRITE_SECTOR_LONG:
                                        // This is followed by five more bytes:
                                        // (1) Drive (zero based)
                                        // (2) Sector size (1 = 128, 2 = 256, 3 = 512, 4 = 1024)
                                        // (3) Sector # MSB - zero based
                                        // (4) Sector #
                                        // (5) Sector #
                                        // (6) Sector # LSB
                                        //
                                        // ...and then the sector data
                                        
                                        event = getAnEvent();
                                        event->clean(EVT_WRITE_SECTOR_LONG);
                                        state = STATE_GET_SIX;
                                        count = 0;  // no bytes received yet
                                        break;

                                case PROTO_DONE:    // Also PROTO_ABORT
                                        transactionDone = true;
                                        event = getAnEvent();
                                        event->clean(EVT_DONE);
                                        break;

                                case PROTO_GET_STATUS:
                                        event = getAnEvent();
                                        event->clean(EVT_GET_STATUS);
                                        state = STATE_GET_ONE;
                                        break;
                                        
                                case PROTO_GET_VERSION:
                                        event = getAnEvent();
                                        event->clean(EVT_GET_VERSION);
                                        hasEvent = true;
                                        break;

                                case PROTO_GET_MOUNTED_LIST:
                                        event = getAnEvent();
                                        event->clean(EVT_GET_MOUNTED);
                                        hasEvent = true;
                                        break;

                                case PROTO_GET_CLOCK:
                                        event = getAnEvent();
                                        event->clean(EVT_GET_CLOCK);
                                        hasEvent = true;
                                        break;

                                case PROTO_SET_CLOCK:
                                        event = getAnEvent();
                                        event->clean(EVT_SET_CLOCK);
                                        count = 8;
                                        state = STATE_APPEND_SECTOR;
                                        break;

                                case PROTO_WRITE_FILE:
                                        event = getAnEvent();
                                        event->clean(EVT_WRITE_FILE);
                                        state = STATE_WAIT_NULL;
                                        hasEvent = false;
                                        break;

                                case PROTO_WRITE_BYTES:
                                        event = getAnEvent();
                                        event->clean(EVT_WRITE_BYTES);
                                        state = STATE_GET_LENGTH;
                                        hasEvent = false;
                                        break;

                                case PROTO_SAVE_CONFIG:
                                        event = getAnEvent();
                                        event->clean(EVT_SAVE_CONFIG);
                                        hasEvent = true;
                                        break;

                                case PROTO_SET_TIMER:
                                        event = getAnEvent();
                                        event->clean(EVT_SET_TIMER);
                                        state = STATE_GET_ONE;
                                        break;

                                default:
                                        Serial.print("Got unknown command code: ");
                                        Serial.println((byte)token, HEX);
                                        transactionDone = true;
                        }
                        break;
                        
                case STATE_WAIT_NULL:
                        // This keeps adding bytes until a 0x00 is seen, then
                        // it sends the message and returns to the main state.
                        
                        event->addByte(token);    // always add it, even if null
                        if (token == 0x00)        // if null, end of the data
                        {
                                state = STATE_CMD;
                                hasEvent = true;
                        }
                        break;

                case STATE_GET_SIX:
                        event->addByte(token);
                        state = STATE_GET_FIVE;
                        break;
                        
                case STATE_GET_FIVE:
                        event->addByte(token);
                        state = STATE_GET_FOUR;
                        break;
                        
                case STATE_GET_FOUR:
                        // Adds this byte, then moves to get three more.
                        
                        event->addByte(token);
                        state = STATE_GET_THREE;
                        break;
                        
                case STATE_GET_THREE:
                        event->addByte(token);
                        state = STATE_GET_TWO;
                        break;
                        
                case STATE_GET_TWO:
                        event->addByte(token);
                        state = STATE_GET_ONE;
                        break;
                        
                case STATE_GET_ONE:
                        // This gets one more byte, sends the message, then
                        // returns to the main state.
                        
                        event->addByte(token);
                        
                        // Some event types need special processing to get
                        // additional bytes.  This logic handles those cases.
                        
                        if (event->getType() == EVT_WRITE_SECTOR || event->getType() == EVT_WRITE_SECTOR_LONG)
                        {
                                // Get the whole sector's worth of data
                                
                                count = 256;    // need to calculate from message
                                state = STATE_APPEND_SECTOR;
                        }
                        else
                        {
                                state = STATE_CMD;
                                hasEvent = true;
                        }
                        break;

                case STATE_GET_DRV_NUMBER_TO_MOUNT:
                        // This is the drive number they want to mount

                        event->addByte(token);
                        state = STATE_GET_DRV_NAME;
                        break;
                        
                case STATE_GET_DRV_NAME:
                        // This is the read-only flag for the drive they want to mount,
                        // so add it to the message, then get the filename.
                        
                        event->addByte(token);
                        state = STATE_WAIT_NULL;
                        break;
                        
                case STATE_APPEND_SECTOR:
                        // A sector's worth of data is next.
                        
                        event->addByte(token);
                        if (--count == 0)
                        {
                                state = STATE_CMD;
                                hasEvent = true;
                        }
                        break;

                case STATE_GET_LENGTH:
                        // This is data for an open write file.  This byte is the length
                        // and is either 1-255 for 1-255 bytes to follow, or 0 to indicate
                        // 256 bytes follow.  Set up the count but put the original value
                        // into the event.
                        
                        count = (token == 0 ? 256 : token);
                        event->addByte(token);
                        state = STATE_APPEND_SECTOR;
                        break;
        }
        
        // If this is the end of a transaction, indicate it on the UI.
        
        if (transactionDone)
        {
                uInt->sendEvent(UI_TRANSACTION_STOP);
        }
}




//=============================================================================
// Send a message back to the host.  The message has to be reformatted to the
// line side protocol.  The event is freed after being sent.  It is safe to
// assume this blocks until the event has been sent.

void Link::sendEvent(Event *eptr)
{
        byte *bptr;
        
        prepareWrite();    // get ready to write and for host to read
        
        switch (eptr->getType())
        {
                case EVT_ACK:
                        writeByte(PROTO_ACK);
                        break;
                        
                case EVT_NAK:
                        // A NAK is always followed by a single byte
                        // reason code, so send the NAK and then the
                        // reason byte.
                        
                        writeByte(PROTO_NAK);    // NAK
                        bptr = eptr->getData();
                        writeByte(*bptr);  // reason code
                        break;
                        
                case EVT_FILE_DATA:
                {
                        // This is a request to send a message back to the host.  The
                        // data buffer contains a byte count (one byte, 0-255) followed
                        // by that number of bytes to send to the host.  Note that the
                        // length can be zero, indicating end of file.
                        
                        writeByte(PROTO_FILE_DATA);    // send the command
                        byte *dptr= eptr->getData();  // pointer to the data
                        byte msgLength = *dptr++;  // number of bytes to send
                        writeByte(msgLength);    // length of data to follow
                        while (msgLength--)
                        {
                                writeByte(*dptr++);
                        }
                        break;
                }
                        
                case EVT_DIR_INFO:
                {
                        writeByte(PROTO_DIR);
                        byte *dptr= eptr->getData();  // pointer to the data
                        do
                        {
                                writeByte(*dptr);
                        }
                        while (*dptr++);
                        break;
                }
                
                case EVT_DIR_END:
                        writeByte(PROTO_DIR_END);
                        break;
                        
                case EVT_READ_SECTOR:
                {
                        // Sector data heading back to host.  Always 256 bytes.
                        
                        writeByte(PROTO_SECTOR_DATA);
                        byte *dptr = eptr->getData();
                        unsigned int size = getSectorSize(*dptr++);
                        while (size--)
                        {
                                writeByte(*dptr++);
                        }
                        break;
                }
                
                case EVT_DISK_STATUS:
                {
                        writeByte(PROTO_STATUS);
                        byte *dptr = eptr->getData();
                        writeByte(*dptr++);
                        break;
                }

                case EVT_MOUNTED:
                {
                        writeByte(PROTO_MOUNT_INFO);
                        byte *dptr = eptr->getData();
                        writeByte(*dptr++);   // drive number
                        writeByte(*dptr++);   // read-only flag
                        while (*dptr)
                        {
                                writeByte(*dptr++);
                        }
                        writeByte(0);
                        break;
                }

                case EVT_CLOCK_DATA:
                        writeByte(PROTO_CLOCK_DATA);
                        byte *dptr = eptr->getData();
                        for (int i = 0; i < 8; i++)
                        {
                                writeByte(*dptr++);
                        }
                        break;
        }
        
        prepareRead();    // back to read mode
        uInt->sendEvent(UI_TRANSACTION_STOP);
        
        freeAnEvent(eptr);      // all done with event
}




//=============================================================================
// Rather than constantly freeing and new'ing Events, maintain a set of free
// ones and just ask for a new one.  This is called to get one, or NULL if
// none is available.

Event *Link::getAnEvent(void)
{
        return freeEvent;
}




//=============================================================================
// When done using an Event, use this function to free it for future use.

void Link::freeAnEvent(Event *eptr)
{
        freeEvent = eptr;
}
