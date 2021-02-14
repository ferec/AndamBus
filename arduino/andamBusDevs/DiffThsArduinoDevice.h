#ifndef DIFFTHSARDUINODEVICE_H
#define DIFFTHSARDUINODEVICE_H

#include "ArduinoDevice.h"
#include "TimeGuard.h"
#include "BusDeviceHelper.h"

#define DEFAULT_ON_TEMP_DIFF_CG 1500 // turn on limit difference
#define DEFAULT_OFF_TEMP_DIFF_CG 100 // turn off limit difference
#define DEFAULT_MAX_TEMP_LIMIT_CG 9500 // limit to 95 degrees celsius in boiler
#define HYSTERESIS_CENTIGRADES 500 // hysteresis near upper limit
#define DEFAULT_DIFFTHS_NOSWITCH_PERIOD_SEC 300 // 5 minutes

class DiffThsArduinoDevice:public ArduinoDevice,public TimeGuard {

  public:
//    DiffThsArduinoDevice(ArduinoW1 **buses);
    DiffThsArduinoDevice(ArduinoAndamBusUnit *abu);
    DiffThsArduinoDevice(uint8_t id, uint8_t pin, ArduinoAndamBusUnit *_abu);

    virtual uint8_t getPersistData(uint8_t data[], uint8_t maxlen);
    virtual uint8_t restore(uint8_t data[], uint8_t length);

    virtual VirtualDeviceType getType() { return VirtualDeviceType::DIFF_THERMOSTAT; }

    virtual uint8_t getPropertyList(ItemProperty propList[], uint8_t size);
    virtual bool setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId);
    virtual void doWork();
    virtual uint8_t getPortCount();
    virtual int16_t getPortValue(uint8_t idx);
    virtual uint8_t getPortId(uint8_t idx);
    virtual VirtualPortType getPortType(uint8_t idx);
    virtual void setPortId(uint8_t idx, uint8_t id);

    bool checkBusDevActive(uint8_t bi, uint8_t di);

  protected:
//    virtual bool getHighTemp(int16_t &t);
    virtual bool getLowTemp(int16_t &t);

	virtual bool configMissing();

  private:
    uint16_t diffOn, diffOff, tLimit; // in centigrades; diffOn,diffOff - (tempH-tempL)>= diffmin, tLimit > tempL
//    uint32_t th_addrh, th_addrl, tl_addrh, tl_addrl;
//    uint8_t thBusIndex, tlBusIndex, thDevIndex, tlDevIndex;
    
	BusDeviceHelper tmHigh, tmLow;

};

#endif // ARDUINODEVICE_H
