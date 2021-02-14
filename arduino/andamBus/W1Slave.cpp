#include "W1Slave.h"
#include "Util.h"

W1Slave::W1Slave():addr_hi(0),addr_lo(0),created(0),boolPack(0) {
//  LOG_U("construct w1 slave " << this);
}

void W1Slave::getAddress(DeviceAddress &deviceAddress) {
	uint32_t *da = (uint32_t*)deviceAddress;
	
	da[0] = addr_hi;
	da[1] = addr_lo;
}

bool W1Slave::hasAddress(DeviceAddress &deviceAddress) {
	uint32_t *da = (uint32_t*)deviceAddress;

//  LOG_U("hasAddress " << iom::hex << addr_hi << " vs " << da[0]);
	if (addr_hi == da[0] && addr_lo == da[1])
		return true;
	return false;
}

void W1Slave::activate(DeviceAddress &deviceAddress, uint8_t _id, uint16_t _created) {
  LOG_U(F("activating w1 slave ") << (int)_id);

	id = _id;
//	portId = 0;
	created = _created;
	
	uint32_t *da = (uint32_t*)deviceAddress;
	addr_hi = da[0];
	addr_lo = da[1];
	age = 0;
//	active = true;
	BB_TRUE(boolPack,AR_BDEV_ACTIVE);
	value = 0;
	
	LOG_U(F("created ") << (int) _id << " " << value);
}

uint8_t W1Slave::getPropertyList(ItemProperty propList[], uint8_t size) {
  if (size < 2)
    return 0;

  uint8_t cnt=0;
  propList[cnt].entityId = id;
  propList[cnt].type = AndamBusPropertyType::LONG_ADDRESS;
  propList[cnt].propertyId = 0;
  propList[cnt++].value = getAddressHigh();
  
  propList[cnt].entityId = id;
  propList[cnt].type = AndamBusPropertyType::LONG_ADDRESS;
  propList[cnt].propertyId = 1;
  propList[cnt++].value = getAddressLow();

  return cnt;
}

bool W1Slave::isActive() {
	return BB_READ(boolPack, AR_BDEV_ACTIVE);
}

bool W1Slave::isChanged() {
	return BB_READ(boolPack, AR_BDEV_CHANGED);
}

void W1Slave::setChanged(bool val) {
	if (val)
		BB_TRUE(boolPack,AR_BDEV_CHANGED);
	else
		BB_FALSE(boolPack,AR_BDEV_CHANGED);
}	
