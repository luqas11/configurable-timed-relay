#ifndef EEPROMUtils_h
#define EEPROMUtils_h

#include "Arduino.h"

// Initializes the EEPROM library.
void beginEEPROM();

// Writes a uint16_t value to the EEPROM at the specified address.
void writeToEEPROM(int index, uint16_t value);

// Reads a uint16_t value from the EEPROM at the specified address.
uint16_t readFromEEPROM(int index);

#endif