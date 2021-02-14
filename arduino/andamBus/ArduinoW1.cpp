#include "ArduinoW1.h"
#include "ArduinoAndamBusUnit.h"
#include "Util.h"

//   OneWire w1x(10);
//   DallasTemperature sensorsx(&w1x);

ArduinoAndamBusUnit *ArduinoW1::abu = nullptr;

ArduinoW1::ArduinoW1():active(false),inConversion(false),lastWork(0),convertSeconds(DEFAULT_CONVERT_SECONDS),devCount(0) {
//  LOG_U("construct bus " << this);
}

void ArduinoW1::setPin(uint8_t _pin) {
  w1.setPin(_pin);
  pin = _pin;
  sensors.setOneWire(&w1);
  sensors.setWaitForConversion(false);
}

void ArduinoW1::doWork() {
  unsigned long now = millis()/1000;
//  LOG_U("bus " << iom::dec << (int)pin << " doWork");

//  LOG_U("inConversion " << inConversion << " isConversionComplete " << sensors.isConversionComplete() << " " << this);
  if (inConversion && sensors.isConversionComplete())
	refreshValues();

  if (lastWork+convertSeconds <= now || lastWork > now) {

    uint8_t cnt = getDeviceCount();
    if (isDeviceMissing()) { // devCount > cnt)
      LOG_U(F("running detect ") << (int)devCount << " vs " << (int)cnt);
      runDetect();
    }
    
    LOG_U("bus " << iom::dec << (int)pin << " convert");


	if (getDeviceCount()>0)
      convert();

      
    lastWork = now;
  }
}

void ArduinoW1::runDetect() {
  sensors.begin();
  
  LOG_U(F("w1 detect ") << sensors.getDeviceCount() << " " << sensors.getDS18Count());

  DeviceAddress addr;
  for (int i=0;i<sensors.getDS18Count();i++) {
    if (sensors.getAddress(addr, i))
      addSlave(addr);
  }
}

void ArduinoW1::refreshValues() {
  if (!inConversion || !sensors.isConversionComplete()) // no need to refresh if no convert was issued or is not complete yet
    return;
	
  for(int i=0;i<ANDAMBUS_MAX_BUS_DEVS;i++) {
//	unsigned long start = millis();
    if (slaves[i].isActive()) {
      DeviceAddress addr;
      slaves[i].getAddress(addr);
      float val = sensors.getTempC(addr);
	  
//	  LOG_U(F("w1 refreshValues ") << (millis()-start));

      if (val != DEVICE_DISCONNECTED_C) {
		int16_t valx = val*100;
		
		if (slaves[i].value != valx) {
			slaves[i].value = valx;
			slaves[i].setChanged(true);
		}
		
        slaves[i].age=0;
//        LOG_U("temp address " << iom::hex << htonl(((uint32_t*)addr)[0]) << htonl(((uint32_t*)addr)[1]) << " = " << iom::dec << slaves[i].value);
      } else {
        if (slaves[i].age < 0xff) {
          slaves[i].age++;
		  slaves[i].setChanged(true);
          LOG_U(F("error reading temperature for ") << iom::hex << htonl(((uint32_t*)addr)[0]) << htonl(((uint32_t*)addr)[1]));
        }
      }
    }
  }
  inConversion = false;
}

void ArduinoW1::convert() {
  unsigned long start = millis();
  sensors.requestTemperatures();
  inConversion = true;

//  LOG_U(F("convert ") << (millis()-start));
}

void ArduinoW1::runCommand(OnewireBusCommand cmd) {
  convert();
}

void ArduinoW1::addSlave(DeviceAddress &addr) {
  LOG_U("adding address " << iom::hex << htonl(((uint32_t*)addr)[0]) << htonl(((uint32_t*)addr)[1]));
//  printAddress(addr);

  uint8_t idx;
  W1Slave *s = findSlaveByAddress(addr, idx);

  if (s==nullptr)
    for(int i=0;i<ANDAMBUS_MAX_BUS_DEVS;i++)
      if (!slaves[i].isActive()) {
        uint32_t *da = (uint32_t*)addr;

        uint8_t id = abu->getID();

        if (id <= UNIT_MAX_ID) {
          slaves[i].activate(addr, id, abu->getCounter());
          slaves[i].portId = abu->getID();
        }

        break;
      }
}

W1Slave* ArduinoW1::findSlaveByAddress(DeviceAddress &deviceAddress, uint8_t &devIndex) {
  for (int i=0;i<ANDAMBUS_MAX_BUS_DEVS;i++)
    if (slaves[i].isActive() && slaves[i].hasAddress(deviceAddress)) {
      devIndex = i;
      return slaves+i;
    }
  return nullptr;
}

void ArduinoW1::printAddress(DeviceAddress &deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) LOG_U("0" << iom::hex);
    LOG_U(deviceAddress[i] << iom::dec);
  }
}

uint8_t ArduinoW1::getDeviceIndexById(uint8_t id) {
  for (int i=0;i<ANDAMBUS_MAX_BUS_DEVS;i++)
    if (slaves[i].isActive() && slaves[i].id == id)
      return i;
  return 0xff;
}


W1Slave* ArduinoW1::getDeviceById(uint8_t id, uint8_t &devIndex) {
  uint8_t idx = getDeviceIndexById(id);
  if (idx < 0xff) {
    devIndex = idx;
    return slaves+idx;
  }
  return nullptr;
}

uint8_t ArduinoW1::getDeviceCount() {
  uint8_t cnt = 0;
  for (int i=0;i<ANDAMBUS_MAX_BUS_DEVS;i++)
    if (slaves[i].isActive())
      cnt++;
  return cnt;
}

W1Slave* ArduinoW1::getPortById(uint8_t id) {
  for (int i=0;i<ANDAMBUS_MAX_BUS_DEVS;i++)
    if (slaves[i].isActive() && slaves[i].portId == id)
      return slaves+i;
  return nullptr;
}

bool ArduinoW1::setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId) {
  if (type == AndamBusPropertyType::PERIOD_SEC && propertyId == 0 && value > 0 && value < 0x7fff)
    convertSeconds = value;
}

uint8_t ArduinoW1::getPropertyList(ItemProperty propList[], uint8_t size) {
  int cnt = 0;
  if (abu==nullptr)
	  return 0;
  uint8_t id = abu->getBusIdByPin(pin);
  
  if (id > 0 && id != 0xff) {
	  propList[cnt].entityId = id;
	  propList[cnt].type = AndamBusPropertyType::PERIOD_SEC;
	  propList[cnt].propertyId = 0;
	  propList[cnt++].value = getConvertSeconds();
  }

  return cnt;
}

uint8_t ArduinoW1::getPersistData(uint8_t data[], uint8_t maxlen) {
  data[0]=pin;
  data[1]=getDeviceCount();

  uint8_t *dd = reinterpret_cast<uint8_t*>(&convertSeconds);
  data[2]= dd[0];
  data[3]= dd[1];
  data[4]= dd[2];
  data[5]= dd[3];
  
  return 6;
}

uint8_t ArduinoW1::restore(uint8_t data[], uint8_t length) {
  if (length == 6) {
    setPin(data[0]);
    devCount=data[1];

    uint8_t *dd = reinterpret_cast<uint8_t*>(&convertSeconds);
    dd[0] = data[2];
    dd[1] = data[3];
    dd[2] = data[4];
    dd[3] = data[5];
    
    active=true;
  }
  
  return 6;
}

uint8_t ArduinoW1::typeId() {
  return static_cast<uint8_t>(Persistent::Type::W1Bus);
}

