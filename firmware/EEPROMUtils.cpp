#include "EEPROMUtils.h"
#include <EEPROM.h>

void beginEEPROM() {
  EEPROM.begin(512);
}

uint16_t readFromEEPROM(int index) {
  int _index = index * sizeof(uint16_t);
  uint16_t value;
  EEPROM.get(_index, value);
  return value;
}

void writeToEEPROM(int index, uint16_t value) {
  int _index = index * sizeof(uint16_t);
  EEPROM.put(_index, value);
  EEPROM.commit();
}
