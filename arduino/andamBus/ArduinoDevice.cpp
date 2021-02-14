#include "ArduinoDevice.h"
#include "util.h"

DevicePort dpstatic;


/*ArduinoDevice::ArduinoDevice():id(0) {
  
}*/

ArduinoDevice::ArduinoDevice(uint8_t _id, uint8_t _pin):id(_id),pin(_pin),boolPack(0) {
  
}

ArduinoDevice::~ArduinoDevice() {
}


uint8_t ArduinoDevice::getPersistData(uint8_t data[], uint8_t maxlen)
{
  data[0] = pin;
//  data[1] = id;
  data[2] = static_cast<uint8_t>(getType());
  return 3;
}

uint8_t ArduinoDevice::restore(uint8_t data[], uint8_t length)
{
	
  LOG_U(F("restore dev ") << (int)pin);
//  id = _id;
  
  if (length >= 3) {
    pin = data[0];
//    id = data[1];
//    type = static_cast<VirtualDeviceType>(data[2]);
//    active=true;
//    LOG_U("type " << (int)getType() << " id" << (int)id);
  }
  return 3;
}

uint8_t ArduinoDevice::typeId()
{
  return static_cast<uint8_t>(Persistent::Type::ArduinoDevice);
}

bool ArduinoDevice::setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId)
{
}

uint8_t ArduinoDevice::getPropertyList(ItemProperty propList[], uint8_t size) {
  return 0;
}

void ArduinoDevice::doWork() {}

DevicePort* ArduinoDevice::getPortById(uint8_t id) {
  for (int i=0;i<getPortCount();i++) {
    if (getPortId(i) == id)
      return &dpstatic;
  }
  return nullptr;
}

uint8_t ArduinoDevice::getPortCount() {
  return 0;
}

int16_t ArduinoDevice::getPortValue(uint8_t idx) {
  return 0;
}

void ArduinoDevice::setPortValue(uint8_t idx, int16_t value) {
}

uint8_t ArduinoDevice::getPortId(uint8_t idx) {
  return 0;
}

VirtualPortType ArduinoDevice::getPortType(uint8_t idx) {
  return VirtualPortType::DIGITAL_INPUT;
}

void ArduinoDevice::setPortId(uint8_t idx, uint8_t id) {
}

uint8_t ArduinoDevice::getPortIndex(uint8_t id) {
	for (int i=0;i<getPortCount();i++) {
		if (getPortId(i) == id)
			return i;
	}
	return 0xff;
}

bool ArduinoDevice::isChanged() {
	return BB_READ(boolPack, AR_DEV_CHANGED);
}

void ArduinoDevice::setChanged(bool val) {
	if (val)
		BB_TRUE(boolPack,AR_DEV_CHANGED);
	else
		BB_FALSE(boolPack,AR_DEV_CHANGED);
}
