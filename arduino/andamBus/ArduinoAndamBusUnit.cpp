#include "ArduinoAndamBusUnit.h"
#include "Util.h"
#include <EEPROM.h>

#include "ArduinoDevice.h"
#include "ArduinoDevFact.h"
#include "DevPortHelper.h"
#include "BusDeviceHelper.h"

ArduinoAndamBusUnit::ArduinoAndamBusUnit(HardwareSerial &ser, uint8_t pinTransmit, uint16_t unitAddress): bs(ser, pinTransmit), AndamBusUnit(bs, unitAddress), lastBusWork(0), lastDevWork(0), lastWork(0),maxDuration(0),maxDurationBus(0),maxDurationDev(0),maxDurationOvr(0),maxDurationParent(0),
	buses{0}, devs{0},
	uptimeSec(0),iterations(0),
	durationRange0(0),durationRange1(0), durationRange2(0), durationRange3(0), durationRange4(0), durationRange5(0),
	interrupt(0),refreshed(false)
	{
  ArduinoW1::abu = this;
  DevPortHelper::abu = this;
  BusDeviceHelper::abu = this;
  blockPin(ARDUINO_SERIAL_RX_PIN);
  blockPin(ARDUINO_SERIAL_TX_PIN);
  blockPin(pinTransmit);

  for (int i = 0; i < MAX_VIRTUAL_PORTS; i++) {
    pins[i].pin = i;
    pins[i].setActive(false);
  }
  
}

void ArduinoAndamBusUnit::init()
{
  restoreUnit();
}

void ArduinoAndamBusUnit::doWork()
{
  iterations++;

  unsigned long start = millis();
  if (lastWork/1000 != start/1000)
	  uptimeSec++;

// high precision timing devices iterations
  for (int i = 0; i < MAX_VIRTUAL_DEVICES; i++) {
    if (devs[i] != nullptr && devs[i]->isActive() && devs[i]->isHighPrec())
      devs[i]->doWorkHighPrec();
  }

  long int dur = millis()-start;
  if (maxDurationDev < dur)
    maxDurationDev = dur; // 3

  AndamBusUnit::doWork();

  dur = millis()-start;
  if (maxDurationParent < dur)
    maxDurationParent = dur; // 4
  
// skip this iteration if the delay is already too high
  if (millis()-start > 50) {
	  LOG_U(F("skipped ") << (millis()-lastWork) << " " << millis()-start);
	  return;
  }
  
// 1-wire bus work
  if (lastBusWork + BUS_WORK_PERIOD_MILLIS <= start || lastBusWork > start) {
    for (int i = 0; i < MAX_SECONDARY_BUSES; i++) {
      ArduinoW1 *bus = buses[i];
      if (bus != nullptr && bus->active)
        bus->doWork();
    }
    lastBusWork = start;
  }

  dur = millis()-start;
  if (maxDurationBus < dur)
    maxDurationBus = dur; // 2

  if (lastDevWork + DEV_WORK_PERIOD_MILLIS <= start || lastDevWork > start) {
    //    LOG_U("abu buses " << iom::hex << buses);

// low precision (5 sec) timing devices iterations
    for (int i = 0; i < MAX_VIRTUAL_DEVICES; i++)
      if (devs[i] != nullptr && devs[i]->isActive())
        devs[i]->doWork();
    lastDevWork = start;
  }

  dur = millis()-start;
  
  if (maxDuration < dur)
    maxDuration = dur; // 1

  dur = millis()-lastWork;
  if (maxDurationOvr < dur)
    maxDurationOvr = dur; // 0

  lastWork = start;

  if (dur >= 10)
	  durationRange0++;
  if (dur >= 30)
	  durationRange1++;
  if (dur >= 50)
	  durationRange2++;
  if (dur >= 100)
	  durationRange3++;
  if (dur >= 150)
	  durationRange4++;
  if (dur >= 200)
	  durationRange5++;
}

void ArduinoAndamBusUnit::createTestItems() {
//  VirtualPort port;
  ItemProperty props[10];
  uint8_t propCount;
//  createPort(13, VirtualPortType::DIGITAL_OUTPUT, port, props, propCount);
//  createPort(14, VirtualPortType::DIGITAL_INPUT_PULLUP, port, props, propCount);

///  SecondaryBus bus;
//  createSecondaryBus(bus, 10);

  VirtualDevice vdev;
  VirtualPort ports[2];
  uint8_t portCount;

/*  AndamBusCommandError ret = createDevice(54, VirtualDeviceType::DIGITAL_POTENTIOMETER, vdev, ports, portCount, props, propCount);

  LOG_U(F("create test dev ") << (int)ret );
  LOG_U(F("port cnt ") << (int)portCount );
  LOG_U(F("port0 ID ") << (int)ports[0].id );
  LOG_U(F("port1 ID ") << (int)ports[1].id );
  
  uint8_t di = getDeviceIndex(vdev.id);
  
  if (di < MAX_VIRTUAL_DEVICES) {
	ArduinoDevice *adev = devs[di];
	adev->setPortValue(0, 20);  
	adev->setPortValue(1, 190);  
  }*/


}

uint8_t ArduinoAndamBusUnit::getSwVersionMajor() {
  return ANDAMBUS_UNIT_SW_VERSION_MAJOR;
}

uint8_t ArduinoAndamBusUnit::getSwVersionMinor() {
  return ANDAMBUS_UNIT_SW_VERSION_MINOR;
}

SlaveHwType ArduinoAndamBusUnit::getHwType() {
  return SlaveHwType::ARDUINO_MEGA;
}

uint8_t ArduinoAndamBusUnit::getSecondaryBusList(SecondaryBus busList[]) {
//  LOG_U("BUS LIST");
  uint8_t cnt = 0;
  for (int i = 0; i < MAX_SECONDARY_BUSES; i++) {
    if (buses[i] != nullptr && buses[i]->active) {
      busList[cnt].id = getBusId(i);
      busList[cnt].status = buses[i]->getStatus();
      busList[cnt++].pin = buses[i]->getPin();
    }
  }

  refreshed = true;
  return cnt;
}

AndamBusCommandError ArduinoAndamBusUnit::createSecondaryBus(SecondaryBus &bus, uint8_t pin) {
  uint8_t busIndex = 0;

  if (!pinAvailable(pin))
    return AndamBusCommandError::PIN_ALREADY_USED;

  blockPin(pin);

  while (busIndex < MAX_SECONDARY_BUSES && buses[busIndex] != nullptr && buses[busIndex]->active) {
    busIndex++;
  }

  if (busIndex >= MAX_SECONDARY_BUSES)
    return AndamBusCommandError::ITEM_LIMIT_REACHED;

  buses[busIndex] = new ArduinoW1();
  buses[busIndex]->active = true;
  buses[busIndex]->setPin(pin);

  bus.id = getBusId(busIndex);
  bus.pin = pin;
  bus.status = buses[busIndex]->getStatus();

  LOG_U(F("Bus created with id=") << (int)bus.id);
  return AndamBusCommandError::OK;
}

void ArduinoAndamBusUnit::secondaryBusDetect(uint8_t busId) {
  uint8_t busIndex = getBusIndex(busId);

  if (busIndex != 0xff)
    buses[busIndex]->runDetect();
}

bool ArduinoAndamBusUnit::secondaryBusCommand(uint8_t busId, OnewireBusCommand busCmd) {
  uint8_t busIndex = getBusIndex(busId);

  LOG_U(F("Convert on ") << (int)busIndex);
//  delay(100);
  if (busIndex != 0xff)
    buses[busIndex]->runCommand(busCmd);
  return true;
}

uint8_t ArduinoAndamBusUnit::getVirtualDeviceList(VirtualDevice devList[]) {
//  LOG_U("DEV LIST");
  return getVirtualDevicesOnBus(devList, 0);
}

void ArduinoAndamBusUnit::startInterrupt() {
	if (pinValid(interrupt)) {
//		LOG_U(F("interrupt started"));
		pinMode(interrupt, OUTPUT);
		digitalWrite(interrupt, LOW);
	}
}

void ArduinoAndamBusUnit::endInterrupt() {
	if (pinValid(interrupt)) {
//		LOG_U(F("interrupt finished"));
		pinMode(interrupt, INPUT);
		digitalWrite(interrupt, LOW);
	}
}

void ArduinoAndamBusUnit::setInterruptPin(uint8_t pin) {
	if (pinValid(pin)) {
		interrupt = pin;
		pinMode(interrupt, INPUT);
		blockPin(interrupt);
		
		endInterrupt();
	}
}

bool ArduinoAndamBusUnit::setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId) {
  switch (type) {
    case AndamBusPropertyType::SLAVE_ADDRESS:
      LOG_U(F("Setting slave address to ") << value);
      if (propertyId == 0 && value > 0 && value < 0x20)
        setAddress(value);
      break;
    case AndamBusPropertyType::PIN:
      LOG_U(F("Setting interrupt pin to ") << value);
	  if (propertyId == 0 && pinAvailable(value)) {
		if (interrupt != 0)
		  unblockPin(interrupt);
	 
	    setInterruptPin(value);
	  }
      break;
    default:
      LOG_U(F("Unknown slave property ") << (int)type << F(" value=") << value);
  }
}

AndamBusCommandError ArduinoAndamBusUnit::setData(uint8_t id, const char *data, uint8_t size) {
  uint8_t idx = getDeviceIndex(id);
  
//  LOG_U(F("setdata idx=") << id << " " << idx);
	
  if (idx >= MAX_VIRTUAL_DEVICES)
    return AndamBusCommandError::ITEM_DOES_NOT_EXIST;

  ArduinoDevice *dev = devs[idx];
  dev->setData(data, size);
  
  return AndamBusCommandError::OK;
}	

AndamBusCommandError ArduinoAndamBusUnit::setPropertyList(ItemProperty propList[], uint8_t count) {

  for (int i = 0; i < count; i++) {
    ItemProperty &prop = propList[i];
    if (prop.entityId == 0) {
      setProperty(prop.type, prop.value, prop.propertyId);
      continue;
    }

    uint8_t bi = getBusIndex(prop.entityId);
    if (bi >= 0 && bi < MAX_SECONDARY_BUSES && buses[bi] != nullptr && buses[bi]->active) {
      buses[bi]->setProperty(prop.type, prop.value, prop.propertyId);
      continue;
    }

    ArduinoDevice *dev = getDeviceById(prop.entityId);
    if (dev != nullptr) {
      dev->setProperty(prop.type, prop.value, prop.propertyId);
      continue;
    }
  }
  return AndamBusCommandError::OK;
}

uint8_t ArduinoAndamBusUnit::getVirtualDevicesOnBus(VirtualDevice devList[], uint8_t busId) {
  uint8_t busIndex = getBusIndex(busId);
  if (busIndex != 0xff)
    return 0;
      
  uint8_t cnt = 0;
  for (int i = 0; i < MAX_VIRTUAL_DEVICES; i++) {
    if (devs[i] != nullptr && devs[i]->isActive() && busId == 0) {
      devList[cnt].id = getDeviceId(i);
      devList[cnt].pin = devs[i]->getPin();
      devList[cnt].type = devs[i]->getType();
      devList[cnt].busId = 0;
      devList[cnt].status = devs[i]->getStatus();

	  devs[i]->setChanged(false);
      cnt++;
    }
  }
  for (int i = 0; i < MAX_SECONDARY_BUSES; i++) {
    ArduinoW1 *bus = buses[i];
    if (bus != nullptr && bus->active && (busId == 0 || busId == getBusId(i)))
      for (int j = 0; j < ANDAMBUS_MAX_BUS_DEVS; j++) {
        W1Slave &wsl = bus->slaves[j];
        if (wsl.isActive()) {
          devList[cnt].id = wsl.id;
          devList[cnt].pin = bus->getPin();
          devList[cnt].type = VirtualDeviceType::THERMOMETER;
          devList[cnt].busId = getBusId(i);
		  devList[cnt].status = wsl.getStatus();
		  
	  	  wsl.setChanged(false);

          cnt++;
        }
      }
  }
  return cnt;
}


AndamBusCommandError ArduinoAndamBusUnit::createDevice(uint8_t pin, VirtualDeviceType type, VirtualDevice &vdev, VirtualPort ports[], uint8_t &portCount, ItemProperty props[], uint8_t &propCount) {
  uint8_t devIndex = 0;

  if (!pinAvailable(pin))
    return AndamBusCommandError::PIN_ALREADY_USED;

  blockPin(pin);

  while (devIndex < MAX_VIRTUAL_DEVICES && devs[devIndex] != nullptr && devs[devIndex]->isActive()) {
    devIndex++;
  }

  if (devIndex >= MAX_VIRTUAL_DEVICES)
    return AndamBusCommandError::ITEM_LIMIT_REACHED;

  ArduinoDevice *dev;
/*  if (type == VirtualDeviceType::DIFF_THERMOSTAT)
    dev = new DiffThsArduinoDevice(getID(), pin, this);
  else
    dev = new ArduinoDevice(getID(), pin, type);*/

  dev = ArduinoDevFact::getDeviceByType(type, getID(), pin, this);
  
  if (dev == nullptr) {
	  unblockPin(pin);
	  return AndamBusCommandError::UNKNOWN_DEVICE_TYPE;
  }

  devs[devIndex] = dev;

  portCount = dev->getPortCount();
  for(int i=0;i<portCount;i++) {
	uint8_t pid = getID(); 
	ports[i].id = pid;
	ports[i].deviceId = dev->getId();
	ports[i].type = dev->getPortType(i);
	ports[i].pin = pin;
	ports[i].value = dev->getPortValue(i);
    dev->setPortId(i, pid);
  }

  vdev.id = dev->getId();
  vdev.pin = pin;
  vdev.busId = 0;
  vdev.type = type;
  
  vdev.status = dev->getStatus();

  propCount = dev->getPropertyList(props, ANDAMBUS_MAX_ITEM_COUNT);
  return AndamBusCommandError::OK;
}

bool ArduinoAndamBusUnit::removeSecondaryBus(uint8_t busId) {
  uint8_t busIndex = getBusIndex(busId);
  if (busIndex == 0xff)
    return false;

  buses[busIndex]->active = false;
  unblockPin(buses[busIndex]->getPin());

  LOG_U(F("BUS deleted ") <<  (int)busIndex);
  delete buses[busIndex];

  buses[busIndex] = nullptr;
  return true;
}

bool ArduinoAndamBusUnit::removeVirtualDevice(uint8_t devId) {
  uint8_t devIndex = getDeviceIndex(devId);

  if (devIndex >= MAX_VIRTUAL_DEVICES)
    return false;


  unblockPin(devs[devIndex]->getPin());
  devs[devIndex]->deactivate();
  delete devs[devIndex];
  devs[devIndex] = nullptr;
  return true;
}

AndamBusCommandError ArduinoAndamBusUnit::createPort(uint8_t pin, VirtualPortType type, VirtualPort &port, ItemProperty props[], uint8_t &propCount) {
  //  LOG_U("createport:" << (int)pin);

  if (pin >= MAX_VIRTUAL_PORTS)
    return AndamBusCommandError::INVALID_PIN;

  if (!pinAvailable(pin))
    return AndamBusCommandError::PIN_ALREADY_USED;

  pins[pin].setType(type);
  pins[pin].setActive(true);
  pins[pin].setValue(0);
//  pins[pin].value = pins[pin].getValue();

  port.pin = pin;
  port.id = getPortId(pin);
  port.type = type;
  port.deviceId = 0;
  port.ordinal = 0;
  port.value = pins[pin].getValue();

  propCount = 0;

  return AndamBusCommandError::OK;
}

AndamBusCommandError ArduinoAndamBusUnit::setPortValue(uint8_t id, int32_t value) {
  uint8_t portIndex = getPortIndex(id);
  
  if (portIndex < MAX_VIRTUAL_PORTS) {
	  LOG_U(F("setPortValue port"));
	  if (!pins[portIndex].isActive())
		return AndamBusCommandError::ITEM_DOES_NOT_EXIST;
	  if (pins[portIndex].type != VirtualPortType::DIGITAL_OUTPUT && pins[portIndex].type != VirtualPortType::DIGITAL_OUTPUT_REVERSED && pins[portIndex].type != VirtualPortType::ANALOG_OUTPUT && pins[portIndex].type != VirtualPortType::ANALOG_OUTPUT_PWM)
		return AndamBusCommandError::PIN_DIRECTION;

	  pins[portIndex].setValue(value);
  } else {
	  LOG_U(F("setPortValue dev"));
	  uint8_t devIndex = getDeviceIndexOwningPort(id);
	  LOG_U(F("devIndex=") << devIndex);
	  
	  if (devIndex < MAX_VIRTUAL_DEVICES) {
		if (devs[devIndex] != nullptr && devs[devIndex]->isActive()) {
			portIndex = devs[devIndex]->getPortIndex(id);
			LOG_U(F("portIndex=") << portIndex);
			
			devs[devIndex]->setPortValue(portIndex, value);
		}		
		else
		  return AndamBusCommandError::ITEM_DOES_NOT_EXIST;
	  } else
		  return AndamBusCommandError::ITEM_DOES_NOT_EXIST;
  }

  return AndamBusCommandError::OK;
}

uint8_t ArduinoAndamBusUnit::getChangedValues(ItemValue values[], bool full) {
  int cnt = 0;
  for (int i = 0; i < MAX_VIRTUAL_PORTS; i++) {
    if (pins[i].isActive() && (full || pins[i].isChanged())) {
      values[cnt].id = getPortId(i);
      values[cnt].age = 0;
      values[cnt++].value = pins[i].getValue();
	  
	  pins[i].setChanged(false);
    }
  }
  for (int i = 0; i < MAX_VIRTUAL_DEVICES; i++) {
    if (devs[i] != nullptr && devs[i]->isActive() && (full || devs[i]->isChanged())) {
      for(int j=0;j<devs[i]->getPortCount();j++) {
        values[cnt].id = devs[i]->getPortId(j);
        values[cnt].age = 0;
        values[cnt++].value = (int32_t)devs[i]->getPortValue(j);
//		LOG_U(F("port value=") << (int32_t)devs[i]->getPortValue(j));
      }
	  devs[i]->setChanged(false);
	}
  }
  
  for (int i = 0; i < MAX_SECONDARY_BUSES; i++) {
    ArduinoW1 *bus = buses[i];
    if (bus != nullptr && bus->active) {
//      bus->refreshValues();
      for (int j = 0; j < ANDAMBUS_MAX_BUS_DEVS; j++) {
		W1Slave &ws = bus->slaves[j];	
        if (ws.isActive() && (full || ws.isChanged())) {
          //              LOG_U("value for " << bus.slaves[j].portId << " = " << bus.slaves[j].value);

          values[cnt].id = ws.portId;
          values[cnt].age = ws.age;
          values[cnt++].value = ((int32_t)ws.value) * 10;
		  ws.setChanged(false);
        }
	  }
    }
  }
  return cnt;
}

uint8_t ArduinoAndamBusUnit::getVirtualPortList(VirtualPort portList[]) {
//  LOG_U("PORT LIST");
  return getVirtualPortsOnDevice(portList, 0);
}

uint8_t ArduinoAndamBusUnit::getVirtualPortsOnDevice(VirtualPort portList[], uint8_t devId) {
  uint8_t cnt = 0;

  if (devId == 0) {
    for (int i = 0; i < MAX_VIRTUAL_PORTS; i++) {
      if (pins[i].isActive()) {
        portList[cnt].id = getPortId(i);
        portList[cnt].pin = pins[i].pin;
        portList[cnt].type = pins[i].type;

        portList[cnt].value = pins[i].getValue();
        portList[cnt].ordinal = i;
        portList[cnt].deviceId = 0;

        pins[i].setChanged(false);
        cnt++;
      }
    }
  }

  for (int i = 0; i < MAX_VIRTUAL_DEVICES; i++) {
    ArduinoDevice *dev = devs[i];

    if (dev != nullptr && dev->isActive()) {
//      LOG_U(F("port count on device ") << dev->getPortCount());
      for (int j=0;j<dev->getPortCount();j++) {
        portList[cnt].id=dev->getPortId(j);
        portList[cnt].pin=dev->getPin();
        portList[cnt].type = dev->getPortType(j);
        portList[cnt].value = dev->getPortValue(j);
        portList[cnt].deviceId = dev->getId();
        portList[cnt].ordinal = j;
        cnt++;
      }
    }
  }

  for (int i = 0; i < MAX_SECONDARY_BUSES; i++) {
    ArduinoW1 *bus = buses[i];
    if (bus != nullptr && bus->active) {
//      bus->refreshValues();
      for (int j = 0; j < ANDAMBUS_MAX_BUS_DEVS; j++) {
        W1Slave &wsl = bus->slaves[j];
        if (wsl.isActive()) {
          portList[cnt].id = wsl.portId;
          portList[cnt].pin = bus->getPin();
          portList[cnt].type = VirtualPortType::ANALOG_INPUT;
          portList[cnt].value = ((int32_t)wsl.value) * 10;
		  portList[cnt].ordinal = 0;
          portList[cnt].deviceId = wsl.id;
          cnt++;
        }
      }
    }
  }

  return cnt;
}

uint8_t ArduinoAndamBusUnit::getNewDevices(VirtualDevice devList[]) {
  uint8_t cnt = 0;

 for (int i = 0; i < MAX_SECONDARY_BUSES; i++) {
    ArduinoW1 *bus = buses[i];
    if (bus != nullptr && bus->active)
      for (int j = 0; j < ANDAMBUS_MAX_BUS_DEVS; j++) {
        W1Slave &s = bus->slaves[j];
        if (s.isActive()) {
          devList[cnt].id = s.id;
          devList[cnt].busId = getBusId(i);
          devList[cnt].pin = bus->getPin();
          devList[cnt].status = s.getStatus();
          devList[cnt++].type = VirtualDeviceType::THERMOMETER;
        }
      }
  }

  return cnt;
}

uint8_t ArduinoAndamBusUnit::getNewPorts(VirtualPort portList[]) {
  uint8_t cnt = 0;
  for (int i = 0; i < MAX_SECONDARY_BUSES; i++) {
    ArduinoW1 *bus = buses[i];
    if (bus != nullptr && bus->active)
      for (int j = 0; j < ANDAMBUS_MAX_BUS_DEVS; j++) {
        W1Slave &s = bus->slaves[j];
        if (s.isActive()) {
          portList[cnt].id = s.portId;
          portList[cnt].deviceId = s.id;
          portList[cnt].pin = bus->getPin();
          portList[cnt].value = ((int32_t)s.value) * 10;
          portList[cnt++].type = VirtualPortType::ANALOG_INPUT;
        }
      }
  }

  return cnt;
}

uint8_t ArduinoAndamBusUnit::getNewProperties(ItemProperty propertyList[], VirtualDevice devices[], uint8_t devCount, VirtualPort ports[], uint8_t portCount) {
  int cnt = 0;

  for (int i = 0; i < devCount; i++) {
    uint8_t idx;
    W1Slave *bd = getBusDeviceById(devices[i].id, idx, idx);

    if (bd != nullptr) {
      propertyList[cnt].entityId = bd->id;
      propertyList[cnt].type = AndamBusPropertyType::LONG_ADDRESS;
      propertyList[cnt].propertyId = 0;
      propertyList[cnt++].value = bd->getAddressHigh();

      propertyList[cnt].entityId = bd->id;
      propertyList[cnt].type = AndamBusPropertyType::LONG_ADDRESS;
      propertyList[cnt].propertyId = 1;
      propertyList[cnt++].value = bd->getAddressLow();
    }
  }
  return cnt;
}


bool ArduinoAndamBusUnit::removePort(uint8_t id) {
  uint8_t portIndex = getPortIndex(id);
  if (!pins[portIndex].isActive())
    return false;

  pins[portIndex].setActive(false);
  return true;
}


ArduinoDevice* ArduinoAndamBusUnit::getDeviceById(uint8_t id) {
  for (int i = 0; i < MAX_VIRTUAL_DEVICES; i++)
    if (devs[i] != nullptr && devs[i]->isActive() && devs[i]->getId() == id)
      return devs[i];
  return nullptr;
}

W1Slave* ArduinoAndamBusUnit::getBusDeviceByAddress(DeviceAddress &deviceAddress, uint8_t &busIndex, uint8_t &devIndex) {
  for (int i = 0; i < MAX_SECONDARY_BUSES; i++) {
    ArduinoW1 *bus = buses[i];
    if (bus != nullptr && bus->active) {
      W1Slave *s = bus->findSlaveByAddress(deviceAddress, devIndex);
      if (s != nullptr) {
        busIndex = i;
        return s;
      }
    }
  }
  return nullptr;  
}

W1Slave* ArduinoAndamBusUnit::getBusDeviceByIndex(uint8_t busIndex, uint8_t devIndex) {
  if (busIndex < MAX_SECONDARY_BUSES && devIndex < 0xff && buses[busIndex] != nullptr && buses[busIndex]->active && buses[busIndex]->slaves[devIndex].isActive())
    return &buses[busIndex]->slaves[devIndex];
  return nullptr;
}

W1Slave* ArduinoAndamBusUnit::getBusDeviceById(uint8_t id, uint8_t &busIndex, uint8_t &devIndex) {
  for (int i = 0; i < MAX_SECONDARY_BUSES; i++) {
    ArduinoW1 *bus = buses[i];
    if (bus != nullptr && bus->active) {
      W1Slave *s = bus->getDeviceById(id, devIndex);
      if (s != nullptr) {
        busIndex = i;
        return s;
      }
    }
  }
  return nullptr;
}

bool ArduinoAndamBusUnit::checkBusActive(uint8_t id) {
  uint8_t busIndex = getBusIndex(id);
  return busIndex < MAX_SECONDARY_BUSES && buses[busIndex] != nullptr && buses[busIndex]->active;
}

DevicePort* ArduinoAndamBusUnit::getDevicePortById(uint8_t id) {
  for (int i = 0; i < MAX_VIRTUAL_DEVICES; i++) {
    ArduinoDevice *dev = devs[i];
    if (dev != nullptr && dev->isActive()) {
      DevicePort *p = dev->getPortById(id);
      if (p != nullptr)
        return p;
    }
  }
  return nullptr;
}

W1Slave* ArduinoAndamBusUnit::getBusDevicePortById(uint8_t id) {
  for (int i = 0; i < MAX_SECONDARY_BUSES; i++) {
    ArduinoW1 *bus = buses[i];
    if (bus != nullptr && bus->active) {
      W1Slave *s = bus->getPortById(id);
      if (s != nullptr)
        return s;
    }
  }
  return nullptr;
}

uint8_t ArduinoAndamBusUnit::getID() {
  uint8_t idx;
  for (int id = SECONDARY_BUS_OFFSET + MAX_SECONDARY_BUSES; id <= UNIT_MAX_ID; id++) {
    if (getDeviceById(id) != nullptr)
      continue;
    if (getBusDeviceById(id,idx,idx) != nullptr)
      continue;
    if (getDevicePortById(id) != nullptr)
      continue;
    if (getBusDevicePortById(id) != nullptr)
      continue;

    LOG_U(F("new ID ") << (int)id);
    return id;
  }

  return 0xFF;
}

bool ArduinoAndamBusUnit::pinAvailable(uint8_t pin) {
  if (pin == 0 || pin >= MAX_VIRTUAL_PORTS || pins[pin].isActive() || pins[pin].isBlocked())
    return false;

  return true;
}

uint8_t ArduinoAndamBusUnit::getMetadataList(UnitMetadata mdl[], uint8_t count) {
    int i=0;
    mdl[i].type = MetadataType::DIGITAL_INPUT_PORTS;
    mdl[i].propertyId = 0;
    mdl[i++].value = 0b11111111111111001111111111111000;

    mdl[i].type = MetadataType::DIGITAL_INPUT_PORTS;
    mdl[i].propertyId = 1;
    mdl[i++].value = 0b1111111111111111111;

    mdl[i].type = MetadataType::DIGITAL_OUTPUT_PORTS;
    mdl[i].propertyId = 0;
    mdl[i++].value = 0b11111111111111001111111111111000;

    mdl[i].type = MetadataType::DIGITAL_OUTPUT_PORTS;
    mdl[i].propertyId = 1;
    mdl[i++].value = 0b1111111111111111111;

    mdl[i].type = MetadataType::ANALOG_INPUT_PORTS;
    mdl[i].propertyId = 1;
    mdl[i++].value = 0b11111111110000000000000000000000;

    mdl[i].type = MetadataType::ANALOG_INPUT_PORTS;
    mdl[i].propertyId = 2;
    mdl[i++].value = 0b111111;

    mdl[i].type = MetadataType::ANALOG_OUTPUT_PORTS;
    mdl[i].propertyId = 0;
    mdl[i++].value = 0b11111111111000;

    mdl[i].type = MetadataType::FREE_MEMORY;
    mdl[i].propertyId = 0;
    mdl[i++].value = freeMemory();

    mdl[i].type = MetadataType::UPTIME;
    mdl[i].propertyId = 0;
    mdl[i++].value = uptimeSec;

    mdl[i].type = MetadataType::TICK;
    mdl[i].propertyId = 0;
    mdl[i++].value = iterations;

    mdl[i].type = MetadataType::MAX_CYCLE_DURATION;
    mdl[i].propertyId = 0;
    mdl[i++].value = maxDurationOvr;

    mdl[i].type = MetadataType::MAX_CYCLE_DURATION;
    mdl[i].propertyId = 1;
    mdl[i++].value = maxDuration;

    mdl[i].type = MetadataType::MAX_CYCLE_DURATION;
    mdl[i].propertyId = 2;
    mdl[i++].value = maxDurationBus;

    mdl[i].type = MetadataType::MAX_CYCLE_DURATION;
    mdl[i].propertyId = 3;
    mdl[i++].value = maxDurationDev;

    mdl[i].type = MetadataType::MAX_CYCLE_DURATION;
    mdl[i].propertyId = 4;
    mdl[i++].value = maxDurationParent;


    mdl[i].type = MetadataType::CYCLE_DURATION_RANGE;
    mdl[i].propertyId = 0;
    mdl[i++].value = durationRange0;
    mdl[i].type = MetadataType::CYCLE_DURATION_RANGE;
    mdl[i].propertyId = 1;
    mdl[i++].value = durationRange1;
    mdl[i].type = MetadataType::CYCLE_DURATION_RANGE;
    mdl[i].propertyId = 2;
    mdl[i++].value = durationRange2;
    mdl[i].type = MetadataType::CYCLE_DURATION_RANGE;
    mdl[i].propertyId = 3;
    mdl[i++].value = durationRange3;
    mdl[i].type = MetadataType::CYCLE_DURATION_RANGE;
    mdl[i].propertyId = 4;
    mdl[i++].value = durationRange4;
    mdl[i].type = MetadataType::CYCLE_DURATION_RANGE;
    mdl[i].propertyId = 5;
    mdl[i++].value = durationRange5;


	LOG_U(F("uptime: ") << uptimeSec << " vs " << millis()/1000);
	
	return i;
}

uint8_t ArduinoAndamBusUnit::getPropertyList(ItemProperty propList[], uint8_t size) {
//  LOG_U("PROP LIST");
  return getPropertyList(propList, size, 0xff);
}

uint8_t ArduinoAndamBusUnit::getPropertyList(ItemProperty propList[], uint8_t size, uint8_t id) {
  int cnt = 0;

  if (id == 0xff) { // all item properties
    for (int i = 0; i < MAX_SECONDARY_BUSES; i++) {
      ArduinoW1 *bus = buses[i];
      if (bus != nullptr && bus->active) {
  
/*        if (bus->getConvertSeconds() != DEFAULT_CONVERT_SECONDS) {
          propList[cnt].entityId = getBusId(i);
          propList[cnt].type = AndamBusPropertyType::SECONDARY_BUS_REFRESH;
          propList[cnt].propertyId = 0;
          propList[cnt++].value = bus->getConvertSeconds();
          if (cnt >= size)
            return size;
        }*/
  
        cnt += bus->getPropertyList(propList+cnt, size-cnt);
        for (int j = 0; j < ANDAMBUS_MAX_BUS_DEVS; j++) {
          W1Slave &w1s = bus->slaves[j];
          if (w1s.isActive()) {
            cnt += w1s.getPropertyList(propList+cnt, size-cnt);

            if (cnt >= size)
              return size;
          }
        }
      }
    }
    
    for(int i=0;i<MAX_VIRTUAL_DEVICES;i++) {
      if (devs[i] != nullptr && devs[i]->isActive())
        cnt += devs[i]->getPropertyList(propList+cnt, size-cnt);
    }

    return cnt;
  } 

  // properties for item with ID=id
  uint8_t bi = getBusIndex(id);
  if (bi != 0xff && buses[bi] != nullptr && buses[bi]->active)
    return buses[bi]->getPropertyList(propList+cnt, size);
  
  ArduinoDevice *dev = getDeviceById(id);
  if (dev != nullptr)
    return dev->getPropertyList(propList+cnt, size-cnt);
}

void ArduinoAndamBusUnit::blockPin(uint8_t pin) {
  pins[pin].setBlocked(true);
  LOG_U("pin " << (int)pin << " blocked");
}

void ArduinoAndamBusUnit::unblockPin(uint8_t pin) {
  pins[pin].setBlocked(false);
  LOG_U("pin " << (int)pin << " unblocked");
}

// ID vs index management
uint8_t ArduinoAndamBusUnit::getBusId(uint8_t index) {
  if (index < MAX_SECONDARY_BUSES)
    return index + SECONDARY_BUS_OFFSET;
  return 0;
}

uint8_t ArduinoAndamBusUnit::getBusIndex(uint8_t id) {
  if (id >= SECONDARY_BUS_OFFSET && id < SECONDARY_BUS_OFFSET+MAX_SECONDARY_BUSES)
    return id - SECONDARY_BUS_OFFSET;
  return 0xff;
}

uint8_t ArduinoAndamBusUnit::getDeviceId(uint8_t index) {
  return devs[index]->getId();
}

uint8_t ArduinoAndamBusUnit::getDeviceIndexByPin(uint8_t pin) {
  for (int i = 0; i < MAX_VIRTUAL_DEVICES; i++)
    if (devs[i] != nullptr && devs[i]->getPin() == pin && devs[i]->isActive())
      return i;

  return MAX_VIRTUAL_DEVICES;
}

uint8_t ArduinoAndamBusUnit::getDeviceIndex(uint8_t id) {
  for (int i = 0; i < MAX_VIRTUAL_DEVICES; i++)
    if (devs[i] != nullptr && devs[i]->getId() == id && devs[i]->isActive())
      return i;

  return MAX_VIRTUAL_DEVICES;
}

uint8_t ArduinoAndamBusUnit::getDeviceIndexOwningPort(uint8_t id) {
  for (int i = 0; i < MAX_VIRTUAL_DEVICES; i++)
    if (devs[i] != nullptr && devs[i]->isActive() && devs[i]->getPortIndex(id) != 0xff)
      return i;

  return MAX_VIRTUAL_DEVICES;
}

uint8_t ArduinoAndamBusUnit::getPortId(uint8_t index) {
  return index;
}

uint8_t ArduinoAndamBusUnit::getPortIndex(uint8_t id) {
  return id;
}

void ArduinoAndamBusUnit::restoreConfig(const uint8_t *data, uint8_t size, uint8_t page, bool finish) {
//  CRC32 crcep;
  uint16_t pagesize = (UNIT_BUFFER_SIZE/2);
  LOG_U("restoreConfig " << (int)size << " " << (int)page << " " << finish);
  if (page == 0) {
    EEPROM.put(0, 0);
//	crcep.reset();
  }

//  crcep.update(data, size);
		
  hexdump(data, size);

  unsigned long start = millis();
  for(int i=0;i<size;i++)
	  EEPROM[i+page*pagesize+sizeof(EEPROM_MAGICWORD)]=data[i];
  
  LOG_U("EEPROM write " << (millis()-start));

  if (finish) {
    EEPROM.put(0, EEPROM_MAGICWORD);
//	LOG_U("CRC=" << crcep.finalize());
  }
}

void ArduinoAndamBusUnit::doPersist() {
  int adr = 0;
  EEPROM.put(adr, EEPROM_MAGICWORD);

  adr += sizeof(EEPROM_MAGICWORD);

// magic[4],cfgSize[2],sAddr[4],versoin[2],interrupt[1]
// cfgSize = 2 + 4 + 2 + 1

  uint16_t cfgSize = 9;
  EEPROM.put(adr, cfgSize);
  adr += sizeof(cfgSize);

  uint32_t sadr = getAddress();
  EEPROM.put(adr, sadr);
  adr += sizeof(sadr);

  EEPROM.put(adr, (uint8_t)ANDAMBUS_UNIT_SW_VERSION_MAJOR);
  adr++;
  EEPROM.put(adr, (uint8_t)ANDAMBUS_UNIT_SW_VERSION_MINOR);
  adr++;
  EEPROM.put(adr, interrupt);
  adr++;

  uint8_t data[MAX_PERSIST_ITEM_SIZE];
  for (int i = 0; i < MAX_SECONDARY_BUSES; i++) {
    if (buses[i] != nullptr && buses[i]->active) {
      ArduinoW1 &bus = *buses[i];
      LOG_U(F("PERSIST bus ") << bus.getPin() << "," << (int)bus.getDeviceCount());
      uint8_t l = bus.getPersistData(data, MAX_PERSIST_ITEM_SIZE);

  
	  if (l>MAX_PERSIST_ITEM_SIZE) {
		  LOG_U(F("Error persisting bus, length=") << (int)l);
		  return;
	  }

      EEPROM.put(adr++, l);
      EEPROM.put(adr++, bus.typeId());
      for (int i = 0; i < l; i++)
        EEPROM.put(adr++, data[i]);

    }
  }
  
  for (int i = 0; i < MAX_VIRTUAL_DEVICES; i++) {
    ArduinoDevice *dev = devs[i];
	
	if (adr + MAX_PERSIST_ITEM_SIZE +2 > EEPROM.length()) {
		
		LOG_U(F("EEPROM is full"));
		return;
	}
	
    if (dev != nullptr && dev->isActive()) {
      uint8_t l = dev->getPersistData(data, MAX_PERSIST_ITEM_SIZE);
      LOG_U(F("PERSIST device ") << (int)dev->getPin() << "," << (int)dev->getId() << "," << (int)dev->getType() << "," << l);

	  if (l>MAX_PERSIST_ITEM_SIZE) {
		  LOG_U(F("Error persisting device, length=") << (int)l);
		  return;
	  }

      EEPROM.put(adr++, l);
      EEPROM.put(adr++, dev->typeId());

      LOG_U(F("PERSIST data ") << l << " " << iom::hex << (int)data[0]);

      for (int i = 0; i < l; i++)
        EEPROM.put(adr++, data[i]);
    }
  }

  for (int i = 0; i < MAX_VIRTUAL_PORTS; i++) {
	if (adr + MAX_PERSIST_ITEM_SIZE + 2 > EEPROM.length()) {
		LOG_U(F("EEPROM is full"));
		return;
	}

    if (pins[i].isActive()) {
      LOG_U(F("PERSIST pins ") << (int)pins[i].pin << "," << (int)pins[i].type);
      int l = pins[i].getPersistData(data, MAX_PERSIST_ITEM_SIZE);

	  if (l>MAX_PERSIST_ITEM_SIZE) {
		  LOG_U(F("Error persisting pin, length=") << (int)l);
		  return;
	  }

      EEPROM.put(adr++, l);
      EEPROM.put(adr++, pins[i].typeId());

      LOG_U(F("PERSIST data ") << l << " " << iom::hex << (int)data[0]);
      for (int i = 0; i < l; i++)
        EEPROM.put(adr++, data[i]);
    }
  }

  // marking end of cfg part
  EEPROM.put(adr++, 0);
  
  LOG_U(F("persist mem size ") << adr);
}

void ArduinoAndamBusUnit::restoreDevice(uint8_t data[], uint8_t size)
{
  for (int i = 0; i < MAX_VIRTUAL_DEVICES; i++) {
    if (devs[i] == nullptr) {
/*      if (type == Persistent::Type::DiffThermostat)
        devs[i] = new DiffThsArduinoDevice(this);
      else
        devs[i] = new ArduinoDevice();*/

      devs[i] = ArduinoDevFact::restoreDevice(data, size, this);
	  
	  if (devs[i] == nullptr)
		  return;

      blockPin(devs[i]->getPin());

      for(int j=0;j<devs[i]->getPortCount();j++) {
        devs[i]->setPortId(j, getID());
      }
      
/*      devs[i]->restore(getID(), data, size);

      for(int j=0;j<devs[i]->getPortCount();j++) {
        devs[i]->setPortId(j, getID());
      }*/

      break;
    }
  }
}

void ArduinoAndamBusUnit::restoreBus(uint8_t data[], uint8_t size)
{
  for (int i = 0; i < MAX_SECONDARY_BUSES; i++) {
    if (buses[i] == nullptr) {
      buses[i] = new ArduinoW1();
      buses[i]->restore(data, size);
      blockPin(buses[i]->getPin());

      buses[i]->runDetect();
      buses[i]->convert();
      LOG_U(F("bus restored at index ") << (int)i);
      break;
    }
  }
}

void ArduinoAndamBusUnit::restorePin(uint8_t data[], uint8_t size)
{
  if (size == 2) {
    if (data[0] < MAX_VIRTUAL_PORTS)
      pins[data[0]].restore(data, size);
  }
}

uint16_t ArduinoAndamBusUnit::getConfigSize() {
  uint32_t magic;
  EEPROM.get(0, magic);
  
  if (magic != EEPROM_MAGICWORD && magic != EEPROM_MAGICWORD_OLD)
	return 0;

// magic[4],cfgSize[2],sAddr[4],versoin[2],interrupt[1]
// cfgSize = 2 + 4 + 2 + 1

  int adr = sizeof(EEPROM_MAGICWORD);

  if (magic == EEPROM_MAGICWORD) { // new cfg
    uint16_t cfgSize = 0;
  
	EEPROM.get(adr, cfgSize);
	
	adr += cfgSize;
  } else {

	  uint32_t sadr = 0xffffffff;
	  EEPROM.get(adr, sadr);
	  if (sadr == 0 || sadr > UNIT_MAX_ID)
		return 0;
	  
	  adr += sizeof(sadr);
  }
  
  while (EEPROM[adr] != 0 && EEPROM[adr] != 0xff && adr < MAX_PERSIST_SIZE) {
      uint8_t len = (int)EEPROM[adr++];
//	  LOG_U("eprom item " << (int)adr << " " << len);
	  adr++; // item type
      adr += (len); // item data
  }

  return adr-sizeof(EEPROM_MAGICWORD);
}

void ArduinoAndamBusUnit::readConfigPart(unsigned char *data, uint8_t size, uint8_t page) {
    uint16_t pagesize = (UNIT_BUFFER_SIZE/2);
	for (uint16_t i=0;i<size; i++)
		data[i] = EEPROM[i+page*pagesize+sizeof(EEPROM_MAGICWORD)];
}

void ArduinoAndamBusUnit::restoreUnit()
{
  LOG_U(F("EEPROM:") << iom::hex);
  
/*  char eb[150];
  for (int i=0;i<150;i++)
	 eb[i]=EEPROM[i];
 
  hexdump(eb, 150); */

// magic[4],cfgSize[2],sAddr[4],versoin[2],interrupt[1]

  uint32_t bb;
  EEPROM.get(0, bb);
  if (bb == EEPROM_MAGICWORD || bb == EEPROM_MAGICWORD_OLD) {
	
	int adr = sizeof(EEPROM_MAGICWORD);
	uint32_t sadr = 0xffffffff;
	if (bb == EEPROM_MAGICWORD_OLD) {  // old cfg found
		LOG_U(F("old magic found"));

		EEPROM.get(adr, sadr);

		LOG_U(F("old restore - setting address:") << sadr);
		if (sadr > 0 && sadr < UNIT_MAX_ID)
		  setAddress(sadr);
	  
		adr += sizeof(sadr);
	} else {
		uint8_t versionMajor = getSwVersionMajor();
		uint8_t versionMinor = getSwVersionMinor();
		
		uint16_t cfgSize;
		
		EEPROM.get(adr, cfgSize);
		LOG_U(F("cfgsize:") << cfgSize);
		adr += sizeof(cfgSize);

//		cfgSize = sizeof(cfgSize)+sizeof(sadr)+sizeof(versionMajor)+sizeof(versionMinor)+uint8_t; // 2-byte size, 4-byte address, 2-byte versions, 1-byte interrupt pin
		
		EEPROM.get(adr, sadr);
		adr += sizeof(sadr);
		
		LOG_U(F("new  restore - address:") << sadr);
		if (sadr > 0 && sadr < UNIT_MAX_ID)
		  setAddress(sadr);
	  
		uint8_t bVerMin, bVerMaj, intr;
		EEPROM.get(adr++, bVerMaj);
		EEPROM.get(adr++, bVerMin);

		LOG_U(F("new  restore - version:") << (int)bVerMaj << "." << (int)bVerMin);

		EEPROM.get(adr++, intr);
		LOG_U(F("new  restore - intr:") << (int)intr);

		setInterruptPin(intr);
/*		if (pinValid(intr)) {
			interrupt = intr;
			pinMode(interrupt, OUTPUT);
			blockPin(interrupt);
		} else {
			LOG_U(F("Interrupt pin invalid") << (int)intr);
		}*/
	}

    int i = 0;

    uint8_t data[MAX_PERSIST_ITEM_SIZE];

    while (EEPROM[adr] != 0 && EEPROM[adr] != 0xff) {
      uint8_t len = (int)EEPROM[adr++];

      if (len > MAX_PERSIST_ITEM_SIZE) {
		adr++; // type
        adr += len; // item data
        LOG_U(F("restoring device - skipping len=") << (int)len);
        continue;
      }

      uint8_t type = (int)EEPROM[adr++];
      for (int j = 0; j < len; j++) {
        data[j] = EEPROM[adr++];
        //        Serial.print((int)data[i],HEX);
        //        Serial.print(" ");
      }
      //      Serial.println("");
	  
	  
      switch (static_cast<Persistent::Type>(type)) {
        case Persistent::Type::ArduinoPin:
          LOG_U(F("restoring pin ") << i << F(" len:") << (int)len << F(" type:") << (int)type);
          restorePin(data, len);
          break;
        case Persistent::Type::ArduinoDevice:
//        case Persistent::Type::DiffThermostat:
          LOG_U(F("restoring device ") << i << F(" len:") << (int)len << F(" type:") << (int)type);
          restoreDevice(data, len);
          break;
        case Persistent::Type::W1Bus:
          LOG_U(F("restoring bus ") << i << F(" len:") << (int)len << F(" type:") << (int)type);
          restoreBus(data, len);
          break;
        default:
          LOG_U(F("Restore - Invalid type ") << (int)type);
          return;
      }
      i++;
    }

    LOG_U(F("restored ") << i << F(" items at ") << adr << F(" val=0x") << iom::hex << EEPROM[adr]);
  }
}

void ArduinoAndamBusUnit::doTestConvert() {
  secondaryBusCommand(0x40, OnewireBusCommand::CONVERT);
}

void ArduinoAndamBusUnit::softwareReset() {
  asm volatile ( "jmp 0");
}

ArduinoW1* ArduinoAndamBusUnit::getBusByIndex(uint8_t idx) {
  if (idx < MAX_SECONDARY_BUSES && buses[idx] != nullptr && buses[idx]->active)
    return buses[idx];
  return nullptr;
}

uint8_t ArduinoAndamBusUnit::getBusIdByPin(uint8_t pin) {
  if (pin > MAX_VIRTUAL_PORTS)
	  return 0;
  
  for (int i = 0; i < MAX_SECONDARY_BUSES; i++)
    if (buses[i] != nullptr && buses[i]->getPin() == pin && buses[i]->active)
      return getBusId(i);

  return 0;
}	

ArduinoW1* ArduinoAndamBusUnit::getBusById(uint8_t id) {
  uint8_t idx = getBusIndex(id);
  if (idx == 0xff)
    return nullptr;
  return getBusByIndex(idx);
}
