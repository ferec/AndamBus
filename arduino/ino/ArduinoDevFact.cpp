#include "ArduinoDevFact.h"

#include "DiffThsArduinoDevice.h"
#include "ThermostatDevice.h"
#include "SolarThermostatDevice.h"
#include "CircPumpDevice.h"
#include "DigiPot.h"
#include "Display.h"
//#include "Midea.h"
#include "TimerDevice.h"
#include "ClickSwitchDevice.h"
#include "ClickModifierDevice.h"
#include "WindowShadingDevice.h"
#include "util.h"

ArduinoDevice* ArduinoDevFact::getDeviceByType(VirtualDeviceType type, uint8_t id, uint8_t pin, ArduinoAndamBusUnit *abu) {
 switch (type) {
  case VirtualDeviceType::THERMOSTAT:
    return new ThermostatDevice(id,pin,abu);
  case VirtualDeviceType::SOLAR_THERMOSTAT:
    return new SolarThermostatDevice(id,pin,abu);
  case VirtualDeviceType::DIFF_THERMOSTAT:
    return new DiffThsArduinoDevice(id,pin,abu);
  case VirtualDeviceType::CIRC_PUMP:
    return new CircPumpDevice(id,pin,abu);
  case VirtualDeviceType::I2C_DISPLAY:
    return new Display(id,pin,abu);
  case VirtualDeviceType::DIGITAL_POTENTIOMETER:
    return new DigiPot(id,pin,abu);
/*  case VirtualDeviceType::HVAC:
    return new Midea(id,pin,abu);*/
  case VirtualDeviceType::TIMER:
    return new TimerDevice(id,pin,abu);
  case VirtualDeviceType::PUSH_DETECTOR:
    return new ClickSwitchDevice(id,pin,abu);
  case VirtualDeviceType::MODIFIER:
    return new ClickModifierDevice(id,pin,abu);
  case VirtualDeviceType::BLINDS_CONTROL:
    return new WindowShadingDevice(id,pin,abu);
  default:
    LOG_U(F("unknown type ") << (int)type);
    return nullptr;
//    return new ArduinoDevice(id, pin);
 }
}

ArduinoDevice* ArduinoDevFact::restoreDevice(uint8_t data[], uint8_t length, ArduinoAndamBusUnit *abu) {
  if (length < 3)
    return nullptr;
    
  VirtualDeviceType type = static_cast<VirtualDeviceType>(data[2]);
  uint8_t pin = data[0];

  ArduinoDevice *dev = getDeviceByType(type, abu->getID(), pin, abu);

  if (dev != nullptr)
    dev->restore(data, length);

  return dev;
}
