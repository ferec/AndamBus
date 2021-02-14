#include "DigiPot.h"
#include <Wire.h>
#include "util.h"

DigiPot::DigiPot(uint8_t _id, uint8_t _pin, ArduinoAndamBusUnit *_abu):ArduinoDevice(_id, _pin),id0(0),id1(0),val0(0),val1(0),i2c_addr(DEFAULT_DIGIPOT_I2C_SLAVE_ADDR) {
  Wire.begin();
}

uint8_t DigiPot::getPortCount() {
  return 2;
}

uint8_t DigiPot::getPortId(uint8_t idx) {
  if (idx==0)
    return id0;
  if (idx==1)
    return id1;
  return ArduinoDevice::getPortId(idx);
}

uint8_t DigiPot::getPortIndex(uint8_t id) {
  if (id == id0)
    return 0;
  if (id == id1)
    return 1;
  return 0xff;
}

VirtualPortType DigiPot::getPortType(uint8_t idx) {
  return VirtualPortType::ANALOG_OUTPUT;
}

void DigiPot::setPortId(uint8_t idx, uint8_t id) {
  if (idx==0)
    id0 = id;
  if (idx==1)
    id1 = id;

  LOG_U(F("setting port ID ") << (int)id);
}

int16_t DigiPot::getPortValue(uint8_t idx) {
  if (idx==0)
    return val0;
  if (idx==1)
    return val1;
  return 0;
}

void DigiPot::setPortValue(uint8_t idx, int16_t value) {
  if (value < 0)
    value = 0;
  if (value > 0xff)
    value = 0xff;

  setChanged(true);

  if (idx == 0)
    val0 = value;
  if (idx == 1)
    val1 = value;
    
//  LOG_U(F("setting port value ") << (int)value << " on " << (int)idx);
  Wire.beginTransmission(i2c_addr); // transmit to device 
                              // device address is specified in datasheet
  byte wrcmd = idx << 4;
  Wire.write(wrcmd);            // sends instruction byte  
  Wire.write(value);             // sends potentiometer value byte
  int ret = Wire.endTransmission();     // stop transmitting  

  if (ret != 0)
    LOG_U(F("Error setting I2C port value"));
}
