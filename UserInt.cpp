//=============================================================================
// FILE: UserInt.h
//
// This provides a user interface of some sort.  Right now it's just the three
// LEDs on the SD Shield, but feel free to add an LCD shield with better
// status displays.
//
// Current code uses three LEDs.  The meaning of each LED:
//
// GREEN  - I am alive.  Changes states about once a second and indicates the
//          software is still running.
//
// YELLOW - When on, indicates a transaction is currently in progress.  Turns
//          on when an inbound transaction starts and turns off when the last
//          byte of the response is sent.
//
// RED    - Comes on when SD is removed, off when inserted.
//
//=============================================================================

#include <Arduino.h>
#include "UserInt.h"


// Define the pins used for the LEDs

#define RED_LED_PIN  24
#define YELLOW_LED_PIN  23
#define GREEN_LED_PIN  22


// Trying to remember if HIGH or LOW turns the LEDs on or off, so these
// two macros make it easier.

#define LED_OFF  HIGH
#define LED_ON   LOW

// The single instance of this class.

static UserInt *instance;

// Is the green LED on or not?

static bool greenOn;
static int pollCount;

//=============================================================================
// This is a singleton, so this is the static function used to get the single
// instance of it.

UserInt *UserInt::getInstance(void)
{
        // If there is no instance yet, create one and then always return a
        // pointer to that one.
        
        if (instance == NULL)
        {
                instance = new UserInt();
        }
        return instance;
}




//=============================================================================
// Constructor.  Set up pins driving the LEDs and set them to the initial
// state.

UserInt::UserInt(void)
{
        // Turn everything off by default
        
        pinMode(RED_LED_PIN, OUTPUT);
        digitalWrite(RED_LED_PIN, LED_OFF);
        
        pinMode(YELLOW_LED_PIN, OUTPUT);
        digitalWrite(YELLOW_LED_PIN, LED_OFF);
        
        pinMode(GREEN_LED_PIN, OUTPUT);
        digitalWrite(GREEN_LED_PIN, LED_OFF);

        // Now cycle through and turn on red then yellow for a short
        // period, finally leaving green on.

        digitalWrite(RED_LED_PIN, LED_ON);
        delay(300);
        digitalWrite(RED_LED_PIN, LED_OFF);

        digitalWrite(YELLOW_LED_PIN, LED_ON);
        delay(300);
        digitalWrite(YELLOW_LED_PIN, LED_OFF);

        digitalWrite(GREEN_LED_PIN, LED_ON);
        delay(300);
        digitalWrite(GREEN_LED_PIN, LED_OFF);
        
        //digitalWrite(GREEN_LED_PIN, LED_ON);
        
        greenOn = false;
        pollCount = 0;
}




//=============================================================================
// Destructor

UserInt::~UserInt(void)
{
}




//=============================================================================
// Given an UiTransactionType, indicate to the user what's going on.  This
// version simply turns LEDs on or off but if an LCD id attached you can write
// something to it.

void UserInt::sendEvent(UiTransactionType trans)
{
        switch (trans)
        {
                case UI_TRANSACTION_START:
                        digitalWrite(YELLOW_LED_PIN, LED_ON);
                        break;
                        
                case UI_TRANSACTION_STOP:
                        digitalWrite(YELLOW_LED_PIN, LED_OFF);
                        break;

                case UI_SD_REMOVED:
                        digitalWrite(RED_LED_PIN, LED_ON);
                        break;
                        
                case UI_SD_INSERTED:
                        digitalWrite(RED_LED_PIN, LED_OFF);
                        break;
        }
}




//=============================================================================
// Given an UiTransactionType and text, display it on the User Interface.  This
// does nothing with the text for this version.

void UserInt::sendEvent(UiTransactionType trans, char *text)
{
        sendEvent(trans);    // send to main routine
}




//=============================================================================
// A poll function.  The main loop defines how often this is called, but is
// usually every 100 ms.

void UserInt::poll(void)
{
        if (++pollCount >= 10)
        {
                pollCount = 0;
                greenOn = greenOn ? false : true;
                digitalWrite(GREEN_LED_PIN, greenOn);
        }
}



