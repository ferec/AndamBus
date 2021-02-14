#include "DevPortHelper.h"
#include "ArduinoAndamBusUnit.h"
#include "ArduinoDevice.h"
#include "util.h"

ArduinoAndamBusUnit* DevPortHelper::abu = nullptr;

DevPortHelper::DevPortHelper():pin(0),idx(0xff),portIdx(0xff) {
}

DevPortHelper::DevPortHelper(uint8_t _pin, uint8_t _portIdx):pin(_pin),idx(0xff),portIdx(_portIdx) {
}

bool DevPortHelper::isPin() {
  return pin == 0 && idx == 0xff;
}

bool DevPortHelper::validatePairing() {
//  LOG_U("validatePairing pin=" << iom::dec << (int)pin << " idx=" << (int)idx << " portIdx=" << (int)portIdx);
  if (abu == nullptr)
		return false;
  if (isPin())
	return portIdx < MAX_VIRTUAL_PORTS && abu->pins[portIdx].isActive();
  else { //is virtual device
	if (idx ==0xff) // if dev not paired but has to be
		return doDevicePairing();
	return idx < MAX_VIRTUAL_DEVICES && portIdx != 0xff && abu->devs[idx] != nullptr || abu->devs[idx]->isActive();
  }
}

bool DevPortHelper::doDevicePairing() {
  if (pin == 0 || pin >= MAX_VIRTUAL_PORTS)
	  return false;
  
  idx = abu->getDeviceIndexByPin(pin);
  
  if (idx >= MAX_VIRTUAL_DEVICES || abu->devs[idx] == nullptr || !abu->devs[idx]->isActive()) {
	idx = 0xff;
	pin = 0;
    return false;
  }

  return portIdx!= 0xff;
}

bool DevPortHelper::readValue(int16_t &val) {
  if(!validatePairing()) {
//	  if (!doPairing())
//	LOG_U("pairing invalid " << (int)pin);
    return false;
  }

  if (isPin())
    val = abu->pins[portIdx].getValue();
  else
	val = abu->devs[idx]->getPortValue(portIdx);

  return true;
}

bool DevPortHelper::setValue(int16_t val) {
  if(!validatePairing())
//	  if (!doPairing())
    return false;

  if (isPin())
    abu->pins[portIdx].setValue(val);
  else
    abu->devs[idx]->setPortValue(portIdx, val);
  return true;
}

bool DevPortHelper::setDevPort(uint8_t portId) {
	if (abu == nullptr)
		return false;
	
	if (portId < MAX_VIRTUAL_PORTS) {
		pin = 0;
		idx = 0xff;
		portIdx = portId;
		return abu->pins[portIdx].isActive();
	}
		
	uint8_t idx = abu->getDeviceIndexOwningPort(portId);
//	LOG_U("setDevPort " << (int)portId << " " << idx);
	if (idx >= MAX_VIRTUAL_DEVICES) {
		pin = 0;
		portIdx = 0xff;
		return false;
	}

	pin = abu->devs[idx]->getPin();
	portIdx = abu->devs[idx]->getPortIndex(portId);

	return true;
}

bool DevPortHelper::setDevPort(uint8_t dPin, uint8_t _portIdx) {
	if (abu == nullptr)
		return false;

	if (dPin == 0 && _portIdx < MAX_VIRTUAL_PORTS) {
		pin = 0;
		idx = 0xff;
		portIdx = _portIdx;
		return abu->pins[_portIdx].isActive();
	}
	if (dPin == 0xff || _portIdx == 0xff) {
		pin = 0xff;
		portIdx = 0xff;
		return false;
	}
	
	pin = dPin;
	idx = 0xff;
	portIdx = _portIdx;
	return true;
}

uint8_t DevPortHelper::getDevPortId() {
//  LOG_U("getDevPortId " << idx);
  if(!validatePairing())
//	  if (!doPairing())
    return false;
/*  if(needsPairing())
	  if (!doPairing())
		  return 0;*/

  if (isPin())
	return portIdx;

//  LOG_U("active " << abu->devs[idx]->isActive());
	
  return abu->devs[idx]->getPortId(portIdx);
}
