#include "SolarThermostatDevice.h"
#include "util.h"

SolarThermostatDevice::SolarThermostatDevice(uint8_t _id, uint8_t _pin, ArduinoAndamBusUnit *_abu):DiffThsArduinoDevice(_id, _pin, _abu),
//tt_addrh(0), tt_addrl(0),ttBusIndex(0xff),ttDevIndex(0xff),
tLimitTank(DEFAULT_TEMP_LIMIT_TANK_CG),
secondary(false),
valve(0)
{
  
}

uint8_t SolarThermostatDevice::getPersistData(uint8_t data[], uint8_t maxlen)
{
  uint8_t cnt32=0, cnt16=0, cnt8 = 0;
  uint8_t len = DiffThsArduinoDevice::getPersistData(data, maxlen);

//  LOG_U(F("persist solarths ") << iom::dec << len);

  uint8_t *data8 = data+len;

  data8[cnt8++] = valve.getTGPin();

//  LOG_U(F("valve pin ") << iom::dec << valve.getTGPin());

  uint32_t *data32 = (uint32_t*)(data8 + cnt8);
  data32[cnt32++] = tmSwitch.getAddressH();
  data32[cnt32++] = tmSwitch.getAddressL();

//  LOG_U(F("tt_addrh ") << iom::hex << tt_addrh);
//  LOG_U(F("tt_addrl ") << iom::hex << tt_addrl);

  uint16_t *data16 = (uint16_t*)(data32 + cnt32);
  data16[cnt16++] = tLimitTank;
  data16[cnt16++] = valve.getNoSwitchPeriod();

/*  LOG_U(F("tLimitTank ") << iom::dec << tLimitTank);
  LOG_U(F("valve noSwitchPeriod ") << iom::dec << valve.getNoSwitchPeriod());
  LOG_U(F("solarths len ") << iom::dec << (len+cnt32*4+cnt16*2+cnt8));*/

  return len+cnt32*4+cnt16*2+cnt8;
}

uint8_t SolarThermostatDevice::restore(uint8_t data[], uint8_t length)
{
  uint8_t cnt32=0, cnt16=0, cnt8 = 0;
  uint8_t used = DiffThsArduinoDevice::restore(data, length);

//  LOG_U(F("restoring solar ths dev ") << iom::dec << used);

  uint8_t *data8 = data+used;

  valve.setTGPin(data8[cnt8++]);

  if (valve.isTGValid())
    BusDeviceHelper::getAbu()->blockPin(valve.getTGPin());

//  LOG_U(F("valve pin ") << iom::dec << valve.getTGPin());

  uint32_t *data32 = (uint32_t*)(data8 + cnt8);
/*  if (length >= used+cnt32*4+4)
    tt_addrh = data32[cnt32++];
  if (length >= used+cnt32*4+4)
    tt_addrl = data32[cnt32++];*/

  uint32_t ah = data32[cnt32++];
  uint32_t al = data32[cnt32++];
  tmSwitch.setThermometerAddress(ah, al);

//  LOG_U(F("restoring tmSwitch ah=") << iom::hex << ah);
//  LOG_U(F("restoring tmSwitch al=") << iom::hex << al);

  uint16_t *data16 = (uint16_t*)(data32 + cnt32);

  if (length >= used+cnt32*4+cnt16*2+2)
    tLimitTank = data16[cnt16++];
  if (length >= used+cnt32*4+cnt16*2+2)
    valve.setNoSwitchPeriod(data16[cnt16++]);
  
/*  LOG_U(F("restoring tLimitTank ") << iom::dec << tLimitTank);
  LOG_U(F("restoring valve noSwitchPeriod ") << iom::dec << valve.getNoSwitchPeriod());
  LOG_U(F("solarthslen restore ") << iom::dec << (used+cnt32*4+cnt16*2+cnt8));*/

  return used+cnt32*4+cnt16*2+cnt8;
}

void SolarThermostatDevice::deactivate() {
  uint8_t curpin = valve.getTGPin();
  if (curpin > 0 && curpin != 0xff)
    BusDeviceHelper::getAbu()->unblockPin(curpin);
  
  ArduinoDevice::deactivate();
}

uint8_t SolarThermostatDevice::getPropertyList(ItemProperty propList[], uint8_t size) {
  int cnt = DiffThsArduinoDevice::getPropertyList(propList, size);
  
  if (cnt >= size)
    return size;

  propList[cnt].type = AndamBusPropertyType::TEMPERATURE;
  propList[cnt].entityId = id;
  propList[cnt].propertyId = 3;
  propList[cnt++].value = static_cast<int32_t>(tLimitTank)*10;

  if (cnt >= size)
    return size;

  propList[cnt].type = AndamBusPropertyType::PERIOD_SEC;
  propList[cnt].entityId = id;
  propList[cnt].propertyId = 1;
  propList[cnt++].value = valve.getNoSwitchPeriod();

  if (cnt >= size)
    return size;

  if (valve.getTGPin() > 0 && valve.getTGPin() < 0xff) {
    propList[cnt].type = AndamBusPropertyType::PIN;
    propList[cnt].entityId = id;
    propList[cnt].propertyId = 0;
    propList[cnt++].value = valve.getTGPin();
  }

  if (cnt >= size)
    return size;

  uint8_t tid = tmSwitch.getThermometerId();
//  LOG_U("tid=" << (int)tid << "," << tmSwitch.configMissing());
  
//  W1Slave *dev = dev = getBusDeviceByIndex(ttBusIndex, ttDevIndex);
  if (tid != 0) {
    propList[cnt].type = AndamBusPropertyType::ITEM_ID;
    propList[cnt].entityId = id;
    propList[cnt].propertyId = 2;
    propList[cnt++].value = tid;
  }

  return cnt;
}

bool SolarThermostatDevice::setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId)
{
  if (type == AndamBusPropertyType::ITEM_ID && propertyId == 2) {
//    LOG_U(F("setting bus dev T to ") << iom::hex << "0x" << (uint32_t)value);
//    setThermometer(propertyId, value, tt_addrh, tt_addrl, ttBusIndex, ttDevIndex);
	tmSwitch.setThermometer(value);
    return true;
  }

  if (type == AndamBusPropertyType::TEMPERATURE && propertyId == 3) {
    tLimitTank=value/10;
    return true;
  }

  if (type == AndamBusPropertyType::PERIOD_SEC && propertyId == 1) {
    valve.setNoSwitchPeriod(value);
    return true;
  }

  if (type == AndamBusPropertyType::PIN && propertyId == 0) {
	ArduinoAndamBusUnit *abu = BusDeviceHelper::getAbu();
    if (value > 0 && value <0xff && abu->pinAvailable(value)) {

      uint8_t curpin = valve.getTGPin();
      if (curpin > 0 && curpin != 0xff)
        abu->unblockPin(curpin);
        
      valve.setTGPin(value);
      abu->blockPin(value);
      return true;
    } else
      return false;
  }
  
  return DiffThsArduinoDevice::setProperty(type, value, propertyId);
}

bool SolarThermostatDevice::getLowTemp(int16_t &t) {
  if (valve.isTGActive())
    return tmSwitch.readTemp(t);

  return DiffThsArduinoDevice::getLowTemp(t);
}

void SolarThermostatDevice::doWork() {
  int16_t tempL = 0;
  
/*  if (ttBusIndex == 0xff && tt_addrh != 0 && tt_addrl != 0)
    pairBusDev(tt_addrh, tt_addrl, ttBusIndex, ttDevIndex);*/

//  LOG_U("isLinked=" << tmSwitch.isLinked() << ",isValid=" << valve.isTGValid());
// only switch if both valve and thermometer is active
  if (tmSwitch.isLinked() && valve.isTGValid()) {
    if (tgTooFast() || valve.tgTooFast()) {
//      LOG_U(F("solar too fast"));
      return;
    }
  
    bool hasL = DiffThsArduinoDevice::getLowTemp(tempL);

//    LOG_U("hasL=" << hasL << ",tLimitTank=" << tLimitTank << ",tempL=" <<tempL);

    if (hasL && tempL > tLimitTank) {
      valve.activateTGPin(true);
	  setChanged(true);
//      LOG_U(F("solar switched to secondary"));
    }

    // turn back on 5 degrees drop 
    if (hasL && valve.isTGActive() && tempL < tLimitTank - 500) {
      valve.activateTGPin(false);
	  setChanged(true);
//      LOG_U(F("solar switched to primary"));
    }
  } else {
//      LOG_U(F("solar secondary inactive"));
  }


  DiffThsArduinoDevice::doWork();
}

uint8_t SolarThermostatDevice::getPortCount() {
  return DiffThsArduinoDevice::getPortCount() + 1;
}

int16_t SolarThermostatDevice::getPortValue(uint8_t idx) {
  if (idx==1)
    return valve.isTGActive(); 
  return DiffThsArduinoDevice::getPortValue(idx);
}

uint8_t SolarThermostatDevice::getPortId(uint8_t idx) {
  if (idx==1)
    return valve.getPortId();
  return DiffThsArduinoDevice::getPortId(idx);
}

void SolarThermostatDevice::setPortId(uint8_t idx, uint8_t id) {
  if (idx == 1)
    valve.setPortId(id);
  else 
    DiffThsArduinoDevice::setPortId(idx, id);
}

bool SolarThermostatDevice::configMissing() {
	return tmSwitch.configMissing() || DiffThsArduinoDevice::configMissing() || !valve.isTGValid();
	
}