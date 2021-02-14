#ifndef ARDUINODEVICE_H
#define ARDUINODEVICE_H

#include <inttypes.h>
#include "AndamBusTypes.h"
#include "DevicePort.h"
#include "Persistent.h"
//#include "ArduinoAndamBusUnit.h"

#define MAX_VIRTUAL_DEVICE_PORTS 4

#define AR_DEV_CHANGED       1

class ArduinoDevice:public Persistent {
  public:
    ArduinoDevice();
    ArduinoDevice(uint8_t id, uint8_t pin);

    virtual ~ArduinoDevice();

    virtual uint8_t getPersistData(uint8_t data[], uint8_t maxlen);
    virtual uint8_t restore(uint8_t data[], uint8_t length);
    virtual uint8_t typeId();
    bool isActive() { return id > 0; }
//    void setActive(bool act) { active = act; }
    virtual void deactivate() { id =0; }

//    void setPin(uint8_t pin);
    uint8_t getPin() { return pin; }
    uint8_t getId() { return id; }
    virtual VirtualDeviceType getType() = 0; //{ return VirtualDeviceType::NONE; }
    inline virtual bool isHighPrec() { return false; }
	
	uint8_t getStatus() { return (configMissing()?1:0) | (referenceMissing()?2:0) | (referenceFailure()?4:0); }
	
    virtual uint8_t getPortCount();
    virtual uint8_t getPortId(uint8_t idx);
    virtual uint8_t getPortIndex(uint8_t id);
    virtual int16_t getPortValue(uint8_t idx);
    virtual void setPortValue(uint8_t idx, int16_t value);
    virtual VirtualPortType getPortType(uint8_t idx);
    virtual void setPortId(uint8_t idx, uint8_t id);

    virtual uint8_t getPropertyList(ItemProperty propList[], uint8_t size);
    virtual bool setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId);
    DevicePort* getPortById(uint8_t id);
	
	virtual bool setData(const char *data, uint8_t size) {};

    virtual void doWork();
    inline virtual void doWorkHighPrec() {};

	virtual bool isChanged();
	void setChanged(bool val);

  protected:
    uint8_t pin, id;
//    VirtualDeviceType type;
//    bool active;

//    DevicePort ports[MAX_VIRTUAL_DEVICE_PORTS];
	uint8_t boolPack;

	virtual bool configMissing() { return false; } // needs to set minimal configuration items
	virtual bool referenceMissing() { return false; } // configured but not found reference, e.g. missing thermometer with specified address
	virtual bool referenceFailure() { return false; } // reference identified but not working, e.g. thermometer detected but read errors occured

};

#endif // ARDUINODEVICE_H
