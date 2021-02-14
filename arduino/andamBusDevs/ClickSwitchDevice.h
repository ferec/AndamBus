#ifndef CLICKSWITCHDEVICE_H
#define CLICKSWITCHDEVICE_H

#include "ArduinoDevice.h"
#include "ArduinoAndamBusUnit.h"
#include "MultiClickDetector.h"
#include "DevPortHelper.h"

//#define CLICKSW_HIGHLOGIC       1
#define CLICKSW_VALUE1    2
#define CLICKSW_VALUE2     4
#define CLICKSW_VALUE3     8

class ClickSwitchDevice:public ArduinoDevice,protected MultiClickDetector {
  public:
    ClickSwitchDevice(uint8_t id, uint8_t pin, ArduinoAndamBusUnit *_abu);

    virtual uint8_t getPortCount();
    virtual uint8_t getPortId(uint8_t idx);
    virtual void setPortId(uint8_t idx, uint8_t id);
    virtual VirtualPortType getPortType(uint8_t idx);
    virtual uint8_t getPortIndex(uint8_t id);
//    virtual void setPortValue(uint8_t idx, int16_t value);
    virtual int16_t getPortValue(uint8_t idx);
    virtual VirtualDeviceType getType() { return VirtualDeviceType::PUSH_DETECTOR; }
    virtual bool isHighPrec() { return true; }

    virtual uint8_t getPropertyList(ItemProperty propList[], uint8_t size);
    virtual bool setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId);
    virtual uint8_t getPersistData(uint8_t data[], uint8_t maxlen);
    virtual uint8_t restore(uint8_t data[], uint8_t length);

    virtual void clicked(uint8_t cnt);
    virtual void holdStarted(uint8_t cnt);
    virtual void doWorkHighPrec();

  private:
    uint8_t portId1Click, portId2Click, portId3Click;
//    bool highLogic, value1, value2, value3;
	uint8_t boolPack;
	ArduinoAndamBusUnit *abu;

    DevPortHelper dp1Click, dp2Click, dp3Click;

	void setValue(uint8_t idx, bool value);
};


#endif // CLICKSWITCHDEVICE_H
