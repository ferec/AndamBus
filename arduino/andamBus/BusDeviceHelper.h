#ifndef BUSDEVICEHELPER_H
#define BUSDEVICEHELPER_H

#include "ArduinoDevice.h"
#include "ArduinoW1.h"

class BusDeviceHelper {
  friend class ArduinoAndamBusUnit;

  public:
    BusDeviceHelper();
    BusDeviceHelper(uint32_t addrh, uint32_t addrl);

    bool readTemp(int16_t &temp);
    void setThermometer(uint8_t id);
    void setThermometerAddress(uint32_t ah, uint32_t al);
    uint8_t getThermometerId();
	uint32_t getAddressH() {return ah;}
	uint32_t getAddressL() {return al;}
	bool configMissing();
	bool isLinked();

	static ArduinoAndamBusUnit* getAbu() { return abu; }

  protected:
	static ArduinoAndamBusUnit *abu;
	uint32_t ah, al;
	uint8_t bi, di;

    bool pairBusDev();
};


#endif //BUSDEVICEHELPER_H
