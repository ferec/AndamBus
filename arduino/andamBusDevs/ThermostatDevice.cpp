#include "ThermostatDevice.h"
#include "util.h"

ThermostatDevice::ThermostatDevice(uint8_t _id, uint8_t _pin, ArduinoAndamBusUnit *_abu):portId(0),ArduinoDevice(_id, _pin),tempSet(THERMOSTAT_DEFAULT_TEMPERATURE),tempHyst(THERMOSTAT_DEFAULT_HYSTERESIS),highLogic(false) {
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, HIGH);
}

uint8_t ThermostatDevice::getPortCount() {
  return 1;
}

uint8_t ThermostatDevice::getPortId(uint8_t idx) {
  return idx == 0?portId:0;
}

void ThermostatDevice::setPortId(uint8_t idx, uint8_t id) {
  if (idx == 0)
    portId = id;
}

VirtualPortType ThermostatDevice::getPortType(uint8_t idx) {
  return VirtualPortType::ANALOG_INPUT;
}

void ThermostatDevice::setPortValue(uint8_t idx, int16_t value) {
  if (idx != 0)
	  return;

  tempSet = value/10;
}

int16_t ThermostatDevice::getPortValue(uint8_t idx) {
  return idx==0?tempSet*10:0;
}

VirtualDeviceType ThermostatDevice::getType() { return VirtualDeviceType::THERMOSTAT; }


uint8_t ThermostatDevice::getPropertyList(ItemProperty propList[], uint8_t size) {
  int cnt = ArduinoDevice::getPropertyList(propList, size);

  if (cnt >= size)
    return size;

  uint8_t tid = tmAct.getThermometerId();
  
  if (tid != 0) {
    propList[cnt].type = AndamBusPropertyType::ITEM_ID;
    propList[cnt].entityId = id;
    propList[cnt].propertyId = 0;
    propList[cnt++].value = tid;
  }
  
  propList[cnt].type = AndamBusPropertyType::TEMPERATURE;
  propList[cnt].entityId = id;
  propList[cnt].propertyId = 0;
  propList[cnt++].value = static_cast<int32_t>(tempSet)*10;

  propList[cnt].type = AndamBusPropertyType::TEMPERATURE;
  propList[cnt].entityId = id;
  propList[cnt].propertyId = 1;
  propList[cnt++].value = static_cast<int32_t>(tempHyst)*10;

  propList[cnt].type = AndamBusPropertyType::HIGHLOGIC;
  propList[cnt].entityId = id;
  propList[cnt].propertyId = 0;
  propList[cnt++].value = highLogic;
  
  return cnt;
}


bool ThermostatDevice::setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId) {
  if (type == AndamBusPropertyType::ITEM_ID && propertyId == 0) {
	tmAct.setThermometer(value);
  }
  if (type == AndamBusPropertyType::TEMPERATURE && propertyId == 0) {
    tempSet = value/10;
  }
  if (type == AndamBusPropertyType::TEMPERATURE && propertyId == 1) {
    tempHyst = value/10;
  }
  if (type == AndamBusPropertyType::HIGHLOGIC && propertyId == 0) {
	  
	bool hl = value!=0;
	if (hl != highLogic)
		turnOff();

    highLogic = hl;
	
  }
  return ArduinoDevice::setProperty(type, value, propertyId);
}

uint8_t ThermostatDevice::getPersistData(uint8_t data[], uint8_t maxlen)
{
  uint8_t cnt32=0, cnt16=0, cnt8=0;
  uint8_t len = ArduinoDevice::getPersistData(data, maxlen);
  
  uint32_t *data32 = (uint32_t*)(data + len);

  data32[cnt32++] = tmAct.getAddressH();
  data32[cnt32++] = tmAct.getAddressL();

  uint16_t *data16 = (uint16_t*)(data32 + cnt32);

  data16[cnt16++] = tempSet;
  data16[cnt16++] = tempHyst;

  uint8_t *data8 = (uint8_t*)(data16 + cnt16);
  data8[cnt8++]=highLogic;

  return len+cnt32*sizeof(uint32_t)+cnt16*sizeof(uint16_t)+cnt8;
}

uint8_t ThermostatDevice::restore(uint8_t data[], uint8_t length)
{
  uint8_t cnt32=0, cnt16=0, cnt8=0;
  uint8_t used = ArduinoDevice::restore(data, length);

//  LOG_U(F("restoring diff ths dev ") << iom::dec << used);
  
  uint32_t *data32 = (uint32_t*)(data + used);

  uint32_t ah = data32[cnt32++];
  uint32_t al = data32[cnt32++];
  tmAct.setThermometerAddress(ah, al);

  if (length >= used+cnt32*sizeof(uint32_t)+1*sizeof(uint16_t)) {
    uint16_t *data16 = (uint16_t*)(data32 + cnt32);
    tempSet = data16[cnt16++];
  }

  uint16_t *data16 = (uint16_t*)(data32 + cnt32);
  if (length >= used+cnt32*sizeof(uint32_t)+(cnt16+1)*sizeof(uint16_t)) {
    tempHyst = data16[cnt16++];
  }

  uint8_t *data8 = (uint8_t*)(data16 + cnt16);
  if (length >= used+cnt32*sizeof(uint32_t)+cnt16*sizeof(uint16_t)+1) {
	highLogic = data8[cnt8++];
	turnOff();
  }

  return used+cnt32*4+cnt16*2+cnt8;
}

void ThermostatDevice::doWork() {
  int16_t tempAct;
  if (!tmAct.readTemp(tempAct))
	  return;
  
  if (isRunning() && tempAct > tempSet) {
	  turnOff();
	  return;
  }
  if (!isRunning() && tempAct < tempSet && tempSet - tempAct > tempHyst) {
	  turnOn();
	  return;
  }
}

bool ThermostatDevice::isRunning() {
  int val = highLogic?HIGH:LOW;
  return digitalRead(pin) == val;
}

void ThermostatDevice::turnOff() {
  int val = highLogic?LOW:HIGH;
  digitalWrite(pin, val);
}

void ThermostatDevice::turnOn() {
  int val = highLogic?HIGH:LOW;
  digitalWrite(pin, val);
}

bool ThermostatDevice::configMissing() {
	return tmAct.configMissing();
}
