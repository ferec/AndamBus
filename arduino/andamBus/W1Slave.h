#ifndef W1SLAVE_H
#define W1SLAVE_H

#include <inttypes.h>
#include <DallasTemperature.h>
#include "AndamBusTypes.h"

#define AR_BDEV_ACTIVE       1
#define AR_BDEV_CHANGED       2

#define MAX_AGE 10

class W1Slave {
  public:
    W1Slave();

  	bool hasAddress(DeviceAddress &deviceAddress);
  	void activate(DeviceAddress &deviceAddress, uint8_t id, uint16_t created);
    void getAddress(DeviceAddress &addr);
    uint32_t getAddressHigh() { return addr_hi; }
    uint32_t getAddressLow() { return addr_lo; }
	
	uint8_t getStatus() { return age < 20 ? 0: 4; }
	
	bool isActive();

	bool isChanged();
	void setChanged(bool val);

    uint8_t getPropertyList(ItemProperty propList[], uint8_t size);
	
	bool isValid() { return age < MAX_AGE; }
    
    uint8_t id, portId,age;
    int16_t value;

  private:
  	uint16_t created;
  	uint32_t addr_hi, addr_lo;
//  	bool active;
	uint8_t boolPack;

};

#endif // W1SLAVE_H
