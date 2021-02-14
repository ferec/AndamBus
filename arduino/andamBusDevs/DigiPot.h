#ifndef DIGIPOT_H
#define DIGIPOT_H

#include "ArduinoDevice.h"
#include "ArduinoAndamBusUnit.h"

#define DEFAULT_DIGIPOT_I2C_SLAVE_ADDR 0x28

class DigiPot:public ArduinoDevice {
  public:
    DigiPot(uint8_t id, uint8_t pin, ArduinoAndamBusUnit *_abu);

    virtual uint8_t getPortCount();
    virtual uint8_t getPortId(uint8_t idx);
    virtual void setPortId(uint8_t idx, uint8_t id);
    virtual VirtualPortType getPortType(uint8_t idx);
    virtual uint8_t getPortIndex(uint8_t id);
    virtual void setPortValue(uint8_t idx, int16_t value);
    virtual int16_t getPortValue(uint8_t idx);
    virtual VirtualDeviceType getType() { return VirtualDeviceType::DIGITAL_POTENTIOMETER; }

  private:
    uint8_t id0,id1,i2c_addr, val0, val1;
};

#endif // DIGIPOT_H
