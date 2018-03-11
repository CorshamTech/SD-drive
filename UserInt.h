//=============================================================================
// FILE: UserInt.h
//
// This provides a user interface of some sort.  Right now it's just the three
// LEDs on the SD Shield, but feel free to add an LCD shield with better
// status displays.
//=============================================================================

#ifndef __USERINT_H__
#define __USERINT_H__


typedef enum
{
        UI_TRANSACTION_STOP,
        UI_TRANSACTION_START,
        UI_SD_REMOVED,
        UI_SD_INSERTED,
} UiTransactionType;

class UserInt
{
        public:
                void sendEvent(UiTransactionType trans);
                void sendEvent(UiTransactionType trans, char *text);
                void poll(void);
                static UserInt *getInstance(void);
                
        private:
                UserInt(void);
                ~UserInt(void);
};

#endif  // __USERINT_H__


