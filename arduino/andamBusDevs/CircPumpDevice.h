#ifndef CIRCPUMPDEVICE_H
#define CIRCPUMPDEVICE_H

#define DEFAULT_CIRCPUMP_NOSWITCH_PERIOD_SEC 30
#define DEFAULT_ON_PERIOD_SEC   60
#define DEFAULT_IDLE_PERIOD_SEC 1800
#define DEFAULT_IDLE_PERIOD_HIGH_SEC 600
#define DEFAULT_TEMPERATURE_LIMIT_CG 4000 // under 40 degrees celsius stops circulating
#define DEFAULT_TEMPERATURE_LIMIT_HIGH_CG 6000 // above 60 degrees celsius - high circulating mode

#include "ArduinoDevice.h"
#include "BusDeviceHelper.h"
#include "TimeGuard.h"

class CircPumpDevice:public ArduinoDevice,TimeGuard {
  public:
    CircPumpDevice(uint8_t id, uint8_t pin, ArduinoAndamBusUnit *_abu);

    virtual uint8_t getPersistData(uint8_t data[], uint8_t maxlen);
    virtual uint8_t restore(uint8_t data[], uint8_t length);
    virtual VirtualDeviceType getType() { return VirtualDeviceType::CIRC_PUMP; }

    bool setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId);
    virtual uint8_t getPropertyList(ItemProperty propList[], uint8_t size);

    virtual void doWork();


    virtual uint8_t getPortCount();
    virtual int16_t getPortValue(uint8_t idx);
    virtual uint8_t getPortId(uint8_t idx);
    virtual VirtualPortType getPortType(uint8_t idx);
    virtual void setPortId(uint8_t idx, uint8_t id);
    virtual void setPortValue(uint8_t idx, int16_t value);

  protected:
  	virtual bool configMissing();

  private:
    uint16_t tLimit, tLimitH; // in centigrades; temp > tLimit -> high mode
//    uint32_t t_addrh, t_addrl;
    uint16_t runPeriod, idlePeriod, idlePeriodHigh; // in seconds; will not change output pin value until noSwitchPeriod seconds
//    uint8_t tBusIndex, tDevIndex;
	uint8_t manualMode;

	BusDeviceHelper tmLimit;
	
	uint8_t portIdNight;
	bool nightMode;
};


#endif // CIRCPUMPDEVICE_H
