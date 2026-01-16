#ifndef TimeUtils_h
#define TimeUtils_h

#include "Arduino.h"

// Initializes the Time library.
void beginTime();

// Checks if the system hour is set and returns a boolean indicating it.
bool isTimeSet();

// Gets the current system hour.
uint16_t getCurrentHour();

// Gets the current system minute.
uint16_t getCurrentMinute();

#endif