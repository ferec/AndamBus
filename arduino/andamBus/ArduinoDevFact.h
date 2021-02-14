#ifndef ARDUINODEVFACT_H
#define ARDUINODEVFACT_H

#include "ArduinoDevice.h"
#include "ArduinoAndamBusUnit.h"

class ArduinoDevFact {

  public:
    static ArduinoDevice* getDeviceByType(VirtualDeviceType type, uint8_t id, uint8_t pin, ArduinoAndamBusUnit *abu);
    static ArduinoDevice* restoreDevice(uint8_t data[], uint8_t length, ArduinoAndamBusUnit *abu);
};

#endif //ARDUINODEVFACT_H
