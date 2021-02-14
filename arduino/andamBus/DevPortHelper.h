#ifndef DEVPORTHELPER_H
#define DEVPORTHELPER_H

#include <stdint.h>

class ArduinoAndamBusUnit;

class DevPortHelper {
  friend class ArduinoAndamBusUnit;
  
  public:
	DevPortHelper();
	DevPortHelper(uint8_t pin, uint8_t portIdx);
	
	bool readValue(int16_t &val);
	bool setValue(int16_t val);
	bool setDevPort(uint8_t portId);
	bool setDevPort(uint8_t dPin, uint8_t portIdx);
	uint8_t getDevPortId();
	uint8_t getDevPin() { return pin; }
	uint8_t getDevPortIndex() { return portIdx; }
	
	bool hasTarget() { return pin!=0 || portIdx != 0xff; }
  private:
	static ArduinoAndamBusUnit *abu;
	uint8_t pin, idx, portIdx;
	
	/*
	pin holds information about primary pin of the device, 
		pin=0 means it is a direct pin without device in pins array, in that case idx = 0xff and portIdx references the index in pins array
		pin!=0 and idx == 0xff means a device lookup is needed
	portIdx holds the index of the port on the device
	idx holds the index of the device in devs array
	*/
	
	bool isPin();
	bool validatePairing();
	bool doDevicePairing();

};

#endif // DEVPORTHELPER_H