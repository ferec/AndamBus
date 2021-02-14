#include "ArduinoAndamBusUnit.h"
#include "DiffThsArduinoDevice.h"
#include "util.h"

DiffThsArduinoDevice::DiffThsArduinoDevice(ArduinoAndamBusUnit *_abu):DiffThsArduinoDevice(0,0,_abu) {
}

DiffThsArduinoDevice::DiffThsArduinoDevice(uint8_t _id, uint8_t _pin, ArduinoAndamBusUnit *_abu):ArduinoDevice(_id, _pin),TimeGuard(_pin),
//  th_addrh(0),th_addrl(0),tl_addrh(0),tl_addrl(0),
  //thBusIndex(0xff),tlBusIndex(0xff),thDevIndex(0xff),tlDevIndex(0xff),
  diffOn(DEFAULT_ON_TEMP_DIFF_CG),diffOff(DEFAULT_OFF_TEMP_DIFF_CG),tLimit(DEFAULT_MAX_TEMP_LIMIT_CG)
{
  setNoSwitchPeriod(DEFAULT_DIFFTHS_NOSWITCH_PERIOD_SEC);
}

bool DiffThsArduinoDevice::configMissing() {
	return tmHigh.configMissing() || tmLow.configMissing();
}

uint8_t DiffThsArduinoDevice::getPersistData(uint8_t data[], uint8_t maxlen)
{
  uint8_t cnt32=0, cnt16=0;
  uint8_t len = ArduinoDevice::getPersistData(data, maxlen);
  
  uint32_t *data32 = (uint32_t*)(data + len);

  LOG_U(F("persist diffths ") << iom::dec << len);
  
  data32[cnt32++] = tmHigh.getAddressH();
  data32[cnt32++] = tmHigh.getAddressL();
  data32[cnt32++] = tmLow.getAddressH();
  data32[cnt32++] = tmLow.getAddressL();

/*  LOG_U(F("th_addrh ") << iom::hex << th_addrh);
  LOG_U(F("th_addrl ") << iom::hex << th_addrl);
  LOG_U(F("tl_addrh ") << iom::hex << tl_addrh);
  LOG_U(F("tl_addrl ") << iom::hex << tl_addrl);*/

  uint16_t *data16 = (uint16_t*)(data32 + cnt32);

  data16[cnt16++] = diffOn;
  data16[cnt16++] = diffOff;
  data16[cnt16++] = tLimit;
  data16[cnt16++] = getNoSwitchPeriod();

/*  LOG_U(F("diffOn ") << iom::dec << diffOn);
  LOG_U(F("diffOff ") << iom::dec << diffOff);
  LOG_U(F("tLimit ") << iom::dec << tLimit);
  LOG_U(F("noSwitchPeriod ") << iom::dec << getNoSwitchPeriod());

  LOG_U(F("diffthslen ") << iom::dec << (len+cnt32*4+cnt16*2));*/

  return len+cnt32*4+cnt16*2;
}

uint8_t DiffThsArduinoDevice::restore(uint8_t data[], uint8_t length)
{
  uint8_t cnt32=0, cnt16=0;
  uint8_t used = ArduinoDevice::restore(data, length);

//  LOG_U(F("restoring diff ths dev ") << iom::dec << used);
  
  if (length < used+4*sizeof(uint32_t))
	  return used;

  uint32_t *data32 = (uint32_t*)(data + used);

/*  if (length >= used+cnt32*4+4)
    th_addrh = data32[cnt32++];
  if (length >= used+cnt32*4+4)
    th_addrl = data32[cnt32++];
  if (length >= used+cnt32*4+4)
    tl_addrh = data32[cnt32++];
  if (length >= used+cnt32*4+4)
    tl_addrl = data32[cnt32++];*/

  uint32_t ah = data32[cnt32++];
  uint32_t al = data32[cnt32++];
  tmHigh.setThermometerAddress(ah, al);

  ah = data32[cnt32++];
  al = data32[cnt32++];
  tmLow.setThermometerAddress(ah, al);

/*  LOG_U(F("restoring th_addrh ") << iom::hex << th_addrh);
  LOG_U(F("restoring th_addrl ") << iom::hex << th_addrl);
  LOG_U(F("restoring tl_addrh ") << iom::hex << tl_addrh);
  LOG_U(F("restoring tl_addrl ") << iom::hex << tl_addrl);*/

    if (length >= used+4*sizeof(uint32_t)+4*sizeof(uint16_t)) {
      uint16_t *data16 = (uint16_t*)(data32 + 4);
      diffOn = data16[cnt16++];
      diffOff = data16[cnt16++];
      tLimit = data16[cnt16++];
      uint16_t nsp = data16[cnt16++];
      setNoSwitchPeriod(nsp);

/*      LOG_U(F("restoring diffOn ") << iom::dec << diffOn);
      LOG_U(F("restoring diffOff ") << iom::dec << diffOff);
      LOG_U(F("restoring tLimit ") << iom::dec << tLimit);
      LOG_U(F("restoring noSwitchPeriod ") << iom::dec << nsp);*/
      }


  setTGPin(pin);
  
//  LOG_U(F("diffthslen restore ") << iom::dec << (used+cnt32*4+cnt16*2));

  return used+cnt32*4+cnt16*2;
}


uint8_t DiffThsArduinoDevice::getPropertyList(ItemProperty propList[], uint8_t size) {
  int cnt = ArduinoDevice::getPropertyList(propList, size);

  if (cnt >= size)
    return size;

  propList[cnt].type = AndamBusPropertyType::TEMPERATURE;
  propList[cnt].entityId = id;
  propList[cnt].propertyId = 0;
  propList[cnt++].value = static_cast<int32_t>(diffOn)*10;

  propList[cnt].type = AndamBusPropertyType::TEMPERATURE;
  propList[cnt].entityId = id;
  propList[cnt].propertyId = 1;
  propList[cnt++].value = static_cast<int32_t>(diffOff)*10;

  propList[cnt].type = AndamBusPropertyType::TEMPERATURE;
  propList[cnt].entityId = id;
  propList[cnt].propertyId = 2;
  propList[cnt++].value = static_cast<int32_t>(tLimit)*10;

  propList[cnt].type = AndamBusPropertyType::PERIOD_SEC;
  propList[cnt].entityId = id;
  propList[cnt].propertyId = 0;
  propList[cnt++].value = getNoSwitchPeriod();

//  W1Slave *dev = getBusDeviceByIndex(thBusIndex, thDevIndex);
  uint8_t tid = tmHigh.getThermometerId();
  
  if (tid != 0) {
    propList[cnt].type = AndamBusPropertyType::ITEM_ID;
    propList[cnt].entityId = id;
    propList[cnt].propertyId = 0;
    propList[cnt++].value = tid;
  }

//  dev = getBusDeviceByIndex(tlBusIndex, tlDevIndex);
  tid = tmLow.getThermometerId();

  if (tid != 0) {
    propList[cnt].type = AndamBusPropertyType::ITEM_ID;
    propList[cnt].entityId = id;
    propList[cnt].propertyId = 1;
    propList[cnt++].value = tid;
  }

  return cnt;
}

bool DiffThsArduinoDevice::setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId)
{
  if (type == AndamBusPropertyType::ITEM_ID && propertyId == 0) {
//    setThermometer(propertyId, value, th_addrh, th_addrl, thBusIndex, thDevIndex);
	tmHigh.setThermometer(value);
  }

  if (type == AndamBusPropertyType::ITEM_ID && propertyId == 1) {
//    setThermometer(propertyId, value, tl_addrh, tl_addrl, tlBusIndex, tlDevIndex);
	tmLow.setThermometer(value);
  }

  if (type == AndamBusPropertyType::TEMPERATURE && propertyId == 0) {
    diffOn=value/10;
  }
  if (type == AndamBusPropertyType::TEMPERATURE && propertyId == 1) {
    diffOff=value/10;
  }
  if (type == AndamBusPropertyType::TEMPERATURE && propertyId == 2) {
    tLimit=value/10;
  }
  if (type == AndamBusPropertyType::PERIOD_SEC && propertyId == 0) {
    setNoSwitchPeriod(value);
  }

  return ArduinoDevice::setProperty(type, value, propertyId);
}

/*bool DiffThsArduinoDevice::getHighTemp(int16_t &t) {
  return tmHigh.readTemp(t);
}*/

bool DiffThsArduinoDevice::getLowTemp(int16_t &t) {
  return tmLow.readTemp(t);
}

void DiffThsArduinoDevice::doWork() {
  int16_t tempH = 0,
    tempL = 0;

  // ensure do not switch too fast
  if (tgTooFast()) {
//    LOG_U(F("diffths too fast"));
    return;
  }

/*  if (tlBusIndex == 0xff && tl_addrh != 0 && tl_addrl != 0)
    if (!pairBusDev(tl_addrh, tl_addrl, tlBusIndex, tlDevIndex))
      return;

  if (thBusIndex == 0xff && th_addrh != 0 && th_addrl != 0)
    if (!pairBusDev(th_addrh, th_addrl, thBusIndex, thDevIndex))
      return;*/
    
  bool hasL = getLowTemp(tempL);
  bool hasH = tmHigh.readTemp(tempH);
  
  if (!hasL || !hasH)
    return;

  if (!isTGValid()) {
//    LOG_U(F("diff ths invalid pin"));
    return;
  }

  if (tLimit <= tempL) {
    if (isTGActive()) {
//      LOG_U(F("turning off pump - limit reached"));
      activateTGPin(false);
	  setChanged(true);
    }
    return;
  }

//  LOG_U(F("DEBUG diffths ") << isTGActive() << " " << diffOn << " " << diffOff);
  if (!isTGActive() && tempH-tempL>=(int)diffOn) {
    activateTGPin(true);
	setChanged(true);
  }
    
  if (isTGActive() && tempH-tempL<(int)diffOff) {
    activateTGPin(false);
	setChanged(true);
  }
}

uint8_t DiffThsArduinoDevice::getPortCount() {
  return 1;
}

int16_t DiffThsArduinoDevice::getPortValue(uint8_t idx) {
  return idx==0?isTGActive():0;
}

uint8_t DiffThsArduinoDevice::getPortId(uint8_t idx) {
//  LOG_U(F("getPortId ") << tgPortId);
  return tgPortId;
}

VirtualPortType DiffThsArduinoDevice::getPortType(uint8_t idx) {
  return VirtualPortType::DIGITAL_INPUT;
}

void DiffThsArduinoDevice::setPortId(uint8_t idx, uint8_t id) {
//  LOG_U(F("Setting port id ") << (int)idx << " to " << (int)id);
  if (idx == 0)
    tgPortId=id;
}
