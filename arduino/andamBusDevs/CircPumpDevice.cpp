#include "CircPumpDevice.h"
#include "util.h"


CircPumpDevice::CircPumpDevice(uint8_t _id, uint8_t _pin, ArduinoAndamBusUnit *_abu): ArduinoDevice(_id, _pin), TimeGuard(_pin),
//  t_addrh(0), t_addrl(0),
//  tBusIndex(0xff), tDevIndex(0xff),
  runPeriod(DEFAULT_ON_PERIOD_SEC), idlePeriod(DEFAULT_IDLE_PERIOD_SEC), idlePeriodHigh(DEFAULT_IDLE_PERIOD_HIGH_SEC),
  tLimit(DEFAULT_TEMPERATURE_LIMIT_CG), tLimitH(DEFAULT_TEMPERATURE_LIMIT_HIGH_CG)
{
  setNoSwitchPeriod(DEFAULT_CIRCPUMP_NOSWITCH_PERIOD_SEC);
}

uint8_t CircPumpDevice::getPersistData(uint8_t data[], uint8_t maxlen)
{
  uint8_t cnt32 = 0, cnt16 = 0;
  uint8_t len = ArduinoDevice::getPersistData(data, maxlen);

  uint32_t *data32 = (uint32_t*)(data + len);
  data32[cnt32++] = tmLimit.getAddressH();
  data32[cnt32++] = tmLimit.getAddressL();

  uint16_t *data16 = (uint16_t*)(data32 + cnt32);
  data16[cnt16++] = tLimit;
  data16[cnt16++] = tLimitH;
  data16[cnt16++] = runPeriod;
  data16[cnt16++] = idlePeriod;
  data16[cnt16++] = idlePeriodHigh;
  data16[cnt16++] = getNoSwitchPeriod();

  return len + cnt32 * 4 + cnt16 * 2;
}

uint8_t CircPumpDevice::restore(uint8_t data[], uint8_t length)
{
  uint8_t cnt32 = 0, cnt16 = 0;
  uint8_t used = ArduinoDevice::restore(data, length);

  uint32_t *data32 = (uint32_t*)(data + used);
  if (length < used + cnt32 * 4 + 8)
	  return used;
  
/*    t_addrh = data32[cnt32++];
  if (length >= used + cnt32 * 4 + 4)
    t_addrl = data32[cnt32++];*/

  uint32_t ah = data32[cnt32++];
  uint32_t al = data32[cnt32++];
  tmLimit.setThermometerAddress(ah, al);


  uint16_t *data16 = (uint16_t*)(data32 + cnt32);

  if (length >= used + cnt32 * 4 + cnt16 * 2 + 2)
    tLimit = data16[cnt16++];
  if (length >= used + cnt32 * 4 + cnt16 * 2 + 2)
    tLimitH = data16[cnt16++];
  if (length >= used + cnt32 * 4 + cnt16 * 2 + 2)
    runPeriod = data16[cnt16++];
  if (length >= used + cnt32 * 4 + cnt16 * 2 + 2)
    idlePeriod = data16[cnt16++];
  if (length >= used + cnt32 * 4 + cnt16 * 2 + 2)
    idlePeriodHigh = data16[cnt16++];
  if (length >= used + cnt32 * 4 + cnt16 * 2 + 2)
    setNoSwitchPeriod(data16[cnt16++]);

  setTGPin(pin);

  return used + cnt32 * 4 + cnt16 * 2;
}

bool CircPumpDevice::configMissing() {
	return tmLimit.configMissing();
}

uint8_t CircPumpDevice::getPropertyList(ItemProperty propList[], uint8_t size) {
  int cnt = ArduinoDevice::getPropertyList(propList, size);

  if (cnt >= size)
    return size;

  propList[cnt].type = AndamBusPropertyType::TEMPERATURE;
  propList[cnt].entityId = id;
  propList[cnt].propertyId = 0;
  propList[cnt++].value = static_cast<int32_t>(tLimit) * 10;

  propList[cnt].type = AndamBusPropertyType::TEMPERATURE;
  propList[cnt].entityId = id;
  propList[cnt].propertyId = 1;
  propList[cnt++].value = static_cast<int32_t>(tLimitH) * 10;

  propList[cnt].type = AndamBusPropertyType::PERIOD_SEC;
  propList[cnt].entityId = id;
  propList[cnt].propertyId = 0;
  propList[cnt++].value = runPeriod;

  propList[cnt].type = AndamBusPropertyType::PERIOD_SEC;
  propList[cnt].entityId = id;
  propList[cnt].propertyId = 1;
  propList[cnt++].value = idlePeriod;

  propList[cnt].type = AndamBusPropertyType::PERIOD_SEC;
  propList[cnt].entityId = id;
  propList[cnt].propertyId = 2;
  propList[cnt++].value = idlePeriodHigh;

  propList[cnt].type = AndamBusPropertyType::PERIOD_SEC;
  propList[cnt].entityId = id;
  propList[cnt].propertyId = 3;
  propList[cnt++].value = getNoSwitchPeriod();

//  W1Slave *dev = getBusDeviceByIndex(tBusIndex, tDevIndex);

  uint8_t tid = tmLimit.getThermometerId();

  if (tid != 0) {
    propList[cnt].type = AndamBusPropertyType::ITEM_ID;
    propList[cnt].entityId = id;
    propList[cnt].propertyId = 0;
    propList[cnt++].value = tid;
  }


  return cnt;
}

bool CircPumpDevice::setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId)
{
//  LOG_U(F("setting prop ") << (int)type << " " << (int)propertyId << iom::hex << " 0x" << (uint32_t)value);
  if (type == AndamBusPropertyType::ITEM_ID && propertyId == 0) {
//    LOG_U(F("setting bus dev to ") << iom::hex << "0x" << (uint32_t)value);
//    setThermometer(propertyId, value, t_addrh, t_addrl, tBusIndex, tDevIndex);
	tmLimit.setThermometer(value);
  }

  if (type == AndamBusPropertyType::TEMPERATURE && propertyId == 0)
    tLimit = value / 10;
  if (type == AndamBusPropertyType::TEMPERATURE && propertyId == 1)
    tLimitH = value / 10;

  if (type == AndamBusPropertyType::PERIOD_SEC && propertyId == 0)
    runPeriod = value;
  if (type == AndamBusPropertyType::PERIOD_SEC && propertyId == 1)
    idlePeriod = value;
  if (type == AndamBusPropertyType::PERIOD_SEC && propertyId == 2)
    idlePeriodHigh = value;
  if (type == AndamBusPropertyType::PERIOD_SEC && propertyId == 3)
    setNoSwitchPeriod(value);

  return ArduinoDevice::setProperty(type, value, propertyId);
}

void CircPumpDevice::doWork() {
  int16_t temp = 0;

  // ensure do not switch too fast
  if (tgTooFast()) {
    //LOG_U(F("circ Too fast"));
    return;
  }

//  LOG_U(F("circ tBusIndex=") << iom::dec << tBusIndex << " t_addrh=" << t_addrh);

/*  if (tBusIndex == 0xff && t_addrh != 0 && t_addrl != 0)
    if (!pairBusDev(t_addrh, t_addrl, tBusIndex, tDevIndex)) {
      LOG_U(F("Thermometer not paired"));
    }*/

  // if pump pin not set, do nothing
  if (!isTGValid()) {
//    LOG_U(F("circ pump invalid"));
    return;
  }

  // read temperature
  bool hasT = tmLimit.readTemp(temp);

  LOG_U(F("circ temp=") << iom::dec << temp << " " << hasT << " " << tLimit);

  // if temperature sensor available and under limit, stop running
  if (hasT && temp < tLimit) {
	  
	if (isTGActive()) {
		activateTGPin(false);
		setChanged(true);
	}
//    LOG_U(F("circ temp below limit"));
    return;
  }

  uint16_t iper = idlePeriod;

  // if temperature available and above upper limit, use idle period for very hot water
  if (hasT && temp > tLimitH) {
//    LOG_U(F("circ temp above High limit"));
    iper = idlePeriodHigh;
  }

  //  LOG_U("work " << pumpRunning() << " " << lastSwitch/1000 << " " << iper << " " << now/1000);
  if (isTGActive()) {
    // if running and last switch was before runPeriod seconds, turn off
    if (getSecondsFromLastSwitch() >= runPeriod) {
      activateTGPin(false);
	  setChanged(true);
	}
  } else {
    // if not running and last switch was before idlePeriod seconds, turn on
    if (getSecondsFromLastSwitch() >= iper) {
	  LOG_U(F("act1"));
      activateTGPin(true);
	  setChanged(true);
	}
  }


  /*  if (!hasT)
      return;*/

}

uint8_t CircPumpDevice::getPortCount() {
  return 1;
}

int16_t CircPumpDevice::getPortValue(uint8_t idx) {
  return idx == 0 ? isTGActive() : 0;
}

void CircPumpDevice::setPortValue(uint8_t idx, int16_t val) {
  if (idx == 0 && !isTGActive() && val!=0) {
	LOG_U(F("act2"));
    activateTGPin(true);
	setChanged(true);
  }
}

uint8_t CircPumpDevice::getPortId(uint8_t idx) {
//  LOG_U(F("getPortId ") << tgPortId);
  return tgPortId;
}

VirtualPortType CircPumpDevice::getPortType(uint8_t idx) {
  return VirtualPortType::DIGITAL_OUTPUT;
}

void CircPumpDevice::setPortId(uint8_t idx, uint8_t id) {
//  LOG_U(F("Setting port id ") << (int)idx << " to " << (int)id);
  if (idx == 0)
    tgPortId = id;
}
