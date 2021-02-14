#ifndef SOLARTHERMOSTATDEVICE_H
#define SOLARTHERMOSTATDEVICE_H

#include "DiffThsArduinoDevice.h"
#include "ArduinoAndamBusUnit.h"

#define DEFAULT_TEMP_LIMIT_TANK_CG 7500 // at 75 degrees celsius switches to the secondary tank

class SolarThermostatDevice:public DiffThsArduinoDevice {
  public:
    SolarThermostatDevice(uint8_t id, uint8_t pin, ArduinoAndamBusUnit *_abu);
    virtual uint8_t getPersistData(uint8_t data[], uint8_t maxlen);
    virtual uint8_t restore(uint8_t data[], uint8_t length);

    virtual VirtualDeviceType getType() { return VirtualDeviceType::SOLAR_THERMOSTAT; }
    virtual uint8_t getPropertyList(ItemProperty propList[], uint8_t size);
    virtual bool setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId);
    virtual void doWork();

    virtual uint8_t getPortCount();
    virtual int16_t getPortValue(uint8_t idx);
    virtual uint8_t getPortId(uint8_t idx);
    virtual void setPortId(uint8_t idx, uint8_t id);

    virtual void deactivate();
    
  protected:
    virtual bool getLowTemp(int16_t &t);
	virtual bool configMissing();

  private:
//    uint32_t tt_addrh, tt_addrl;
//    uint8_t ttBusIndex, ttDevIndex;
    uint16_t tLimitTank; // in centigrades; temperature when switches to the secondary tank
    bool secondary;
    TimeGuard valve;
	
	BusDeviceHelper tmSwitch;	
};

#endif  //SOLARTHERMOSTAT_H
