#ifndef THERMOSTATDEVICE_H
#define THERMOSTATDEVICE_H

#include "ArduinoDevice.h"
#include "ArduinoAndamBusUnit.h"
#include "BusDeviceHelper.h"

#define THERMOSTAT_DEFAULT_TEMPERATURE 2000 // 20.00 C
#define THERMOSTAT_DEFAULT_HYSTERESIS 100 // 1.00 C

class ThermostatDevice:public ArduinoDevice {
  public:
    ThermostatDevice(uint8_t id, uint8_t pin, ArduinoAndamBusUnit *_abu);
  
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

  protected:
	bool isRunning();
	void turnOff();
	void turnOn();

	virtual bool configMissing();
	
  private:
    uint8_t portId;
	int16_t tempSet, tempHyst; // in centigrades
	BusDeviceHelper tmAct;
	bool highLogic;
};

#endif // THERMOSTATDEVICE_H
