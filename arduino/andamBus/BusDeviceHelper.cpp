#include "BusDeviceHelper.h"
#include "ArduinoAndamBusUnit.h"
#include "util.h"

ArduinoAndamBusUnit* BusDeviceHelper::abu = nullptr;

BusDeviceHelper::BusDeviceHelper():BusDeviceHelper(0,0) {
}

BusDeviceHelper::BusDeviceHelper(uint32_t _addrh, uint32_t _addrl):ah(_addrh),al(_addrl),bi(0xff),di(0xff) {
}

void BusDeviceHelper::setThermometer(uint8_t id) {
  if (abu == nullptr) {
	  LOG_U(F("abu null"));
	  return;
  }
  
  if (id == 0) {
    ah = 0;
    al = 0;
	bi=0xff;
	di=0xff;
  }
  
  W1Slave *dev = abu->getBusDeviceById(id, bi, di);

  if (dev != nullptr) {
          ah = dev->getAddressHigh();
          al = dev->getAddressLow();
  } else {
	  LOG_U(F("thermo by addr not found"));
  }
}

bool BusDeviceHelper::isLinked() { 
  return bi < MAX_SECONDARY_BUSES && di != 0xff;
}

void BusDeviceHelper::setThermometerAddress(uint32_t _ah, uint32_t _al) {
 ah = _ah;
 al = _al;
}

uint8_t BusDeviceHelper::getThermometerId() {
  if (abu == nullptr)
	  return 0;

  if (bi == 0xff && !configMissing())
    pairBusDev();
  
  W1Slave *dev = abu->getBusDeviceByIndex(bi, di);
  
//  LOG_U("getThermometerId=" << dev << " bi=" << (int)bi << ",di=" <<(int)di);
  if (dev != nullptr)
	  return dev->id;

  return 0;
}


bool BusDeviceHelper::readTemp(int16_t &temp) {
  if (abu == nullptr)
	  return false;
  
  if (bi == 0xff && !configMissing())
    pairBusDev();

  if (bi == 0xff || di == 0xff)
	  return false;
	
  W1Slave *dev = abu->getBusDeviceByIndex(bi, di);
  if (dev == nullptr) {
    if (bi != 0xff || di != 0xff) {
      bi = 0xff;
      di = 0xff;
//	  LOG_U(F("Bus device unpaired ") << iom::hex << ah << al);
     }
  } else {
    ArduinoW1 *w1 = abu->getBusByIndex(bi);

    if (w1 == nullptr)
      return false;
  
    if (!dev->isValid())
      return false;
      
    temp = dev->value;
    return true; 
  }
  return false;
}

bool BusDeviceHelper::pairBusDev() {
  if (configMissing())
    return false;

//  LOG_U("pairBusDev " << iom::hex << addrh << " " << addrl);
  DeviceAddress addr;
  uint32_t *da = (uint32_t*)addr;
  da[0] = ah;
  da[1] = al;

  if (abu->getBusDeviceByAddress(addr, bi, di) != nullptr) {
    return true;
  }
  
  return false;
}

bool BusDeviceHelper::configMissing() {
	return ah == 0 && al == 0;
}
