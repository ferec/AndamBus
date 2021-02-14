#ifndef CLICKMODIFIERDEVICE_H
#define CLICKMODIFIERDEVICE_H

#include "ArduinoDevice.h"
#include "ArduinoAndamBusUnit.h"
#include "MultiClickDetector.h"
#include "DevPortHelper.h"

#define DEFAULT_CLICK_MOD_DEV_STEP 1

class ClickModifierDevice:public ArduinoDevice,protected MultiClickDetector {
  public:
    ClickModifierDevice(uint8_t id, uint8_t pin, ArduinoAndamBusUnit *_abu);

    virtual VirtualDeviceType getType() { return VirtualDeviceType::MODIFIER; }
    virtual bool isHighPrec() { return true; }

    virtual uint8_t getPropertyList(ItemProperty propList[], uint8_t size);
    virtual bool setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId);
    virtual uint8_t getPersistData(uint8_t data[], uint8_t maxlen);
    virtual uint8_t restore(uint8_t data[], uint8_t length);

    virtual void clicked(uint8_t cnt);
    virtual void doWorkHighPrec();

  protected:
	virtual bool configMissing() { return !dpTrg.hasTarget(); }

  private:
    uint8_t portId;
    int8_t step;
    DevPortHelper dpTrg;
};

#endif // CLICKMODIFIERDEVICE_H
