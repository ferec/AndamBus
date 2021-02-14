#ifndef TIMERDEVICE_H
#define TIMERDEVICE_H

#include "ArduinoDevice.h"
#include "ArduinoAndamBusUnit.h"

#define DEFAULT_TIMER_PERIOD_SEC 30

class TimerDevice:public ArduinoDevice {
  public:
    TimerDevice(uint8_t id, uint8_t pin, ArduinoAndamBusUnit *_abu);
  
    virtual uint8_t getPortCount();
    virtual uint8_t getPortId(uint8_t idx);
    virtual void setPortId(uint8_t idx, uint8_t id);
    virtual VirtualPortType getPortType(uint8_t idx);
    virtual void setPortValue(uint8_t idx, int16_t value);
    virtual VirtualDeviceType getType();
    virtual int16_t getPortValue(uint8_t idx);

    virtual uint8_t getPropertyList(ItemProperty propList[], uint8_t size);
    virtual bool setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId);
    virtual uint8_t getPersistData(uint8_t data[], uint8_t maxlen);
    virtual uint8_t restore(uint8_t data[], uint8_t length);

    virtual void doWork();

  private:
    uint8_t portId;
    unsigned long switchTime, periodSec;
};

#endif // TIMERDEVICE_H
