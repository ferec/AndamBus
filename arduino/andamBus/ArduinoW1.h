#ifndef ARDUINOW1_H
#define ARDUINOW1_H

#include <inttypes.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "W1Slave.h"
#include "AndamBusTypes.h"
#include "Persistent.h"

#define DEFAULT_CONVERT_SECONDS 60

class ArduinoAndamBusUnit;

class ArduinoW1:public Persistent {
  public:
    ArduinoW1();
    static ArduinoAndamBusUnit *abu;

  // persistent
    virtual uint8_t getPersistData(uint8_t data[], uint8_t maxlen);
    virtual uint8_t restore(uint8_t data[], uint8_t length);
    virtual uint8_t typeId();

	uint8_t getStatus() { return isDeviceMissing()?1:0; }

    void setPin(uint8_t pin);
    uint8_t getPin() { return pin; }
    void runDetect();
    void runCommand(OnewireBusCommand cmd);

    void doWork();

    void convert();

    void refreshValues();

    static void printAddress(DeviceAddress &deviceAddress);

    void addSlave(DeviceAddress &addr);
  
    bool active;

    uint8_t getDeviceCount();
    W1Slave* getDeviceById(uint8_t id, uint8_t &devIndex);
    uint8_t getDeviceIndexById(uint8_t id);
    W1Slave* getPortById(uint8_t id);
    
    W1Slave slaves[ANDAMBUS_MAX_BUS_DEVS];

    W1Slave* findSlaveByAddress(DeviceAddress &deviceAddress,uint8_t &devIndex);

    virtual uint8_t getPropertyList(ItemProperty propList[], uint8_t size);
    bool setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId);
    unsigned long getConvertSeconds() { return convertSeconds; }
  private:
    uint8_t pin, devCount;
    bool inConversion;
    unsigned long lastWork, convertSeconds;
    
    OneWire w1;
    DallasTemperature sensors;
	
	bool isDeviceMissing() { return devCount > getDeviceCount(); }
};

#endif // ARDUINOW1_H
