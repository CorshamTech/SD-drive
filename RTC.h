//=============================================================================
// Header file for the RTC class.  RTC handles a real-time clock.

#ifndef __RTC_H__
#define __RTC_H__

#include <Arduino.h>

class RTC
{
        public:
              RTC(void);
              ~RTC(void);
              bool isPresent(void) { return present; }
              bool getClock(byte *ptr);
              bool setClock(byte *ptr);

        private:
              bool present;
              void setRtcTime(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year);
              void readDS3231time(byte *second, byte *minute, byte *hour, byte *dayOfWeek, byte *dayOfMonth, byte *month, byte *year);
              byte decToBcd(byte val);
              byte bcdToDec(byte val);
};

#endif  // __RTC_H__


