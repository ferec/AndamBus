#include "TimerDevice.h"
#include "util.h"

TimerDevice::TimerDevice(uint8_t _id, uint8_t _pin, ArduinoAndamBusUnit *_abu):portId(0),ArduinoDevice(_id, _pin),switchTime(0),periodSec(DEFAULT_TIMER_PERIOD_SEC) {
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, HIGH);
}

uint8_t TimerDevice::getPortCount() {
  return 1;
}

uint8_t TimerDevice::getPortId(uint8_t idx) {
  return idx == 0?portId:0;
}

void TimerDevice::setPortId(uint8_t idx, uint8_t id) {
  if (idx == 0)
    portId = id;
}

VirtualPortType TimerDevice::getPortType(uint8_t idx) {
  return VirtualPortType::DIGITAL_OUTPUT;
}

void TimerDevice::setPortValue(uint8_t idx, int16_t value) {
  int val = digitalRead(pin);

  if (val == HIGH && value) {
    digitalWrite(pin, LOW);
    switchTime=millis();
	setChanged(true);
  }

  if (val == LOW && !value) {
    digitalWrite(pin, HIGH);
    switchTime=0;
	setChanged(true);
  }
}

int16_t TimerDevice::getPortValue(uint8_t idx) {
  return digitalRead(pin) == LOW;
}

void TimerDevice::doWork() {
  if (switchTime == 0 || digitalRead(pin) == HIGH)
    return;

  if (switchTime+(unsigned long)periodSec*1000 <= millis() || switchTime > millis()) {
    digitalWrite(pin, HIGH);
    switchTime = 0;
	setChanged(true);
  }
}


VirtualDeviceType TimerDevice::getType() { return VirtualDeviceType::TIMER; }

uint8_t TimerDevice::getPropertyList(ItemProperty propList[], uint8_t size) {
  int cnt = ArduinoDevice::getPropertyList(propList, size);

  if (cnt >= size)
    return size;

  if (periodSec != DEFAULT_TIMER_PERIOD_SEC) {
    propList[cnt].type = AndamBusPropertyType::PERIOD_SEC;
    propList[cnt].entityId = id;
    propList[cnt].propertyId = 0;
    propList[cnt++].value = periodSec;
  }

  return cnt;
}

bool TimerDevice::setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId) {
  if (type == AndamBusPropertyType::PERIOD_SEC && propertyId == 0) {
    periodSec = value;
  }
  return ArduinoDevice::setProperty(type, value, propertyId);
}

uint8_t TimerDevice::getPersistData(uint8_t data[], uint8_t maxlen)
{
  uint8_t cnt32=0, cnt16=0;
  uint8_t len = ArduinoDevice::getPersistData(data, maxlen);
  
  uint32_t *data32 = (uint32_t*)(data + len);

  uint16_t *data16 = (uint16_t*)(data32 + cnt32);

  data16[cnt16++] = periodSec;

  return len+cnt32*sizeof(uint32_t)+cnt16*sizeof(uint16_t);
}

uint8_t TimerDevice::restore(uint8_t data[], uint8_t length)
{
  uint8_t cnt32=0, cnt16=0;
  uint8_t used = ArduinoDevice::restore(data, length);

//  LOG_U(F("restoring diff ths dev ") << iom::dec << used);
  
  uint32_t *data32 = (uint32_t*)(data + used);

  if (length >= used+cnt32*sizeof(uint32_t)+1*sizeof(uint16_t)) {
    uint16_t *data16 = (uint16_t*)(data32 + cnt32);
    periodSec = data16[cnt16++];
  }

  return used+cnt32*4+cnt16*2;
}
