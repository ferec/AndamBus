#include "TestAndamBusUnit.h"

#include "util.h"
#include <zlib.h>

unsigned char EEPROMemu[2048] = "12345";
uint16_t cfgPos = 5;

using namespace std;
TestAndamBusUnit::TestAndamBusUnit(BroadcastSocket &bs, uint16_t _address):AndamBusUnit(bs,_address),TESTVAL(235),IDGEN(111)
{
    //ctor
}

TestAndamBusUnit::~TestAndamBusUnit()
{
    //dtor
}

bool TestAndamBusUnit::pinInUse(uint8_t pin) {
    for (auto it=buses.begin();it!=buses.end();it++)
        if (it->pin == pin)
            return true;
    for (auto it=devs.begin();it!=devs.end();it++)
        if (it->pin == pin)
            return true;
    for (auto it=ports.begin();it!=ports.end();it++)
        if (it->pin == pin)
            return true;

    return false;
}

TestItem* TestAndamBusUnit::findBusById(uint8_t id) {
    for (auto it=buses.begin();it!=buses.end();it++)
        if (it->id == id)
            return &(*it);
    return nullptr;
}

//--------------------- Secondary bus ----------------------

AndamBusCommandError TestAndamBusUnit::createSecondaryBus(SecondaryBus &bus, uint8_t pin) {
    if (pinInUse(pin))
        return AndamBusCommandError::PIN_ALREADY_USED;

    TestItem ti(IDGEN++, 0, pin, 0, getCounter());

    buses.push_back(ti);

    bus.id = ti.id;
    bus.pin = ti.pin;

    return AndamBusCommandError::OK;
}

uint8_t TestAndamBusUnit::getSecondaryBusList(SecondaryBus busList[]) {
    int i=0;
    for (auto it=buses.begin();it!=buses.end();it++,i++) {
        TestItem &ti = *it;

        busList[i].id = ti.id;
        busList[i].pin = ti.pin;
    }

    return buses.size();
}

AndamBusCommandError TestAndamBusUnit::setData(uint8_t id, const char *data, uint8_t size) {
    TestItem *dev = findDevById(id);

    if (dev != nullptr) {
        dev->setData(data, size);
        return AndamBusCommandError::OK;
    }
    return AndamBusCommandError::ITEM_DOES_NOT_EXIST;
}

AndamBusCommandError TestAndamBusUnit::setPropertyList(ItemProperty propList[], uint8_t count) {
//    AB_ERROR("TODO");

    for (int i=0;i<count;i++)
        AB_ERROR("Setting property " << propertyTypeString(propList[i].type) << " on " << (int)propList[i].entityId << " to " << propList[i].value);
    return AndamBusCommandError::OK;
}

void TestAndamBusUnit::secondaryBusDetect(uint8_t id) {
    TestItem *bus = findBusById(id);
    if (bus == nullptr)
        return;

    createDeviceOnBus(bus->pin, (uint8_t)VirtualDeviceType::THERMOMETER, id);
}

bool TestAndamBusUnit::secondaryBusCommand(uint8_t id, OnewireBusCommand busCmd) {
    TestItem *bus = findBusById(id);
    if (bus == nullptr)
        return false;

    if (busCmd == OnewireBusCommand::CONVERT) {
        refreshThermometerValuesOnBus(id);
        return true;
    }

    return false;
}

void TestAndamBusUnit::createDeviceOnBus(uint8_t pin, uint8_t type, uint8_t busId) {
    TestItem dev(IDGEN++, busId, pin, type, getCounter());
    devs.push_back(dev);

    TestItem port(IDGEN++, dev.id, pin, (uint8_t)VirtualPortType::ANALOG_OUTPUT, getCounter());
    ports.push_back(port);
}

vector<TestItem*> TestAndamBusUnit::getDevsOnBus(uint8_t busId) {
    vector<TestItem*> bds;
    for (auto it=devs.begin();it!=devs.end();it++)
        if (it->parentId == busId)
            bds.push_back(&(*it));
    return bds;
}

vector<TestItem*> TestAndamBusUnit::getPortsOnBus(uint8_t busId) {
    vector<TestItem*> bds = getDevsOnBus(busId);
    vector<TestItem*> bps;
    for (auto it=bds.begin();it!=bds.end();it++)
        if ((*it)->parentId == busId)
            bps.push_back(*it);

    return bps;
}

void TestAndamBusUnit::refreshThermometerValuesOnBus(uint8_t busId) {
    vector<TestItem*> bps = getPortsOnBus(busId);

    for (auto it=bps.begin();it!=bps.end();it++) {
        TestItem *p = *it;
        p->setValue(p->getValue()+1, getLastPortRefresh());
    }

}

bool TestAndamBusUnit::removeSecondaryBus(uint8_t busId) {
    for (auto it=buses.begin();it!=buses.end();it++)
        if (it->id == busId) {
            it = buses.erase(it);
            removeVirtualDeviceOnBus(busId);
            return true;
        }

    return false;
}

void TestAndamBusUnit::removeVirtualDeviceOnBus(uint8_t busId) {
    for (auto it=devs.begin();it!=devs.end();it++)
        if (it->parentId == busId) {
            removeVirtualPortOnDevice(it->id);
            it = devs.erase(it);
            if (it == devs.end())
                break;
        }
}

void TestAndamBusUnit::removeVirtualPortOnDevice(uint8_t devId) {
    for (auto it=ports.begin();it!=ports.end();it++)
        if (it->parentId == devId) {
            it = ports.erase(it);
            if (it == ports.end())
                break;
        }
}

//--------------------- Virtual devs ----------------------
uint8_t TestAndamBusUnit::getVirtualDeviceList(VirtualDevice devList[]) {
    return getVirtualDevicesOnBus(devList, 0);
}

uint8_t TestAndamBusUnit::getVirtualDevicesOnBus(VirtualDevice devList[], uint8_t busId) {
    int i=0;
    for (auto it=devs.begin();it!=devs.end();it++,i++) {
        TestItem &ti = *it;

        if (ti.parentId == busId || busId == 0) {
            devList[i].id = ti.id;
            devList[i].pin = ti.pin;
            devList[i].busId = ti.parentId;
            devList[i].type = static_cast<VirtualDeviceType>(ti.type);
        }
    }

    return i;
}

AndamBusCommandError TestAndamBusUnit::createDevice(uint8_t pin, VirtualDeviceType type, VirtualDevice &dev, VirtualPort vports[], uint8_t &portCount, ItemProperty props[], uint8_t &propCount) {
    if (pinInUse(pin))
        return AndamBusCommandError::PIN_ALREADY_USED;

    TestItem dev1(IDGEN++, 0, pin, static_cast<uint8_t>(type), getCounter());
    devs.push_back(dev1);

    TestItem port1(IDGEN++, dev1.id, pin, (uint8_t)VirtualPortType::ANALOG_OUTPUT, getCounter());
    ports.push_back(port1);
    TestItem port2(IDGEN++, dev1.id, pin, (uint8_t)VirtualPortType::DIGITAL_INPUT, getCounter());
    ports.push_back(port2);


    dev.id = dev1.id;
    dev.busId = 0;
    dev.pin = pin;
    dev.type = type;

    vports[0].deviceId = port1.parentId;
    vports[0].id = port1.id;
    vports[0].type = static_cast<VirtualPortType>(port1.type);
    vports[0].pin = port1.pin;
    vports[0].value = port1.getValue();

    vports[1].deviceId = port2.parentId;
    vports[1].id = port2.id;
    vports[1].type = static_cast<VirtualPortType>(port2.type);
    vports[1].pin = port2.pin;
    vports[1].value = port2.getValue();

    portCount = 2;

    props[0].type = AndamBusPropertyType::CUSTOM;
    props[0].entityId = dev.id;
    props[0].propertyId = 0;
    props[0].value = 155;

    propCount = 1;

    return AndamBusCommandError::OK;
}

bool TestAndamBusUnit::removeVirtualDevice(uint8_t id) {
    for (auto it=devs.begin();it!=devs.end();it++)
        if (it->id == id) {
            it = devs.erase(it);
            removeVirtualPortOnDevice(id);
            return true;
        }

    return false;
}

//--------------------- Virtual port ----------------------

uint8_t TestAndamBusUnit::getVirtualPortList(VirtualPort portList[]) {
    return getVirtualPortsOnDevice(portList, 0);
}

uint8_t TestAndamBusUnit::getVirtualPortsOnDevice(VirtualPort portList[], uint8_t devId) {
    int i=0;
    for (auto it=ports.begin();it!=ports.end();it++,i++) {
        TestItem &ti = *it;

        if (ti.parentId == devId || devId == 0) {
            portList[i].id = ti.id;
            portList[i].pin = ti.pin;
            portList[i].deviceId = ti.parentId;
            portList[i].type = static_cast<VirtualPortType>(ti.type);
            portList[i].value = 23;
        }
    }

    return i;
}

AndamBusCommandError TestAndamBusUnit::createPort(uint8_t pin, VirtualPortType type, VirtualPort &vport, ItemProperty props[], uint8_t &propCount) {
    if (pinInUse(pin))
        return AndamBusCommandError::PIN_ALREADY_USED;

    ports.push_back(TestItem(IDGEN++, 0, pin, (uint8_t)type, getCounter()));


    vport.id = IDGEN-1;
    vport.deviceId = 0;
    vport.pin = pin;
    vport.type = type;
    vport.value = 0;

    propCount = 0;
    return AndamBusCommandError::OK;
}

bool TestAndamBusUnit::removePort(uint8_t id) {
    for (auto it=ports.begin();it!=ports.end();it++)
        if (it->id == id) {
            it = ports.erase(it);
            return true;
        }

    return false;
}

TestItem* TestAndamBusUnit::findPortById(uint8_t portId) {
    for (auto it=ports.begin();it!=ports.end();it++)
        if (it->id == portId)
            return &(*it);
    return nullptr;
}

TestItem* TestAndamBusUnit::findDevById(uint8_t id) {
    for (auto it=devs.begin();it!=devs.end();it++)
        if (it->id == id)
            return &(*it);
    return nullptr;
}

AndamBusCommandError TestAndamBusUnit::setPortValue(uint8_t id, int32_t value) {
    TestItem *port = findPortById(id);

    if (port == nullptr)
        return AndamBusCommandError::ITEM_DOES_NOT_EXIST;

    port->setValue(value, getLastPortRefresh());
    return AndamBusCommandError::OK;
}

//--------------------- Property ----------------------
uint8_t TestAndamBusUnit::getPropertyList(ItemProperty propList[], uint8_t size) {
    return getPropertyList(propList, size, 0);
}

uint8_t TestAndamBusUnit::getPropertyList(ItemProperty propList[], uint8_t size, uint8_t id) {
    int i=0;
    for (auto it=buses.begin();it!=buses.end();it++,i++) {
        if (it->id == id || id == 0) {
            propList[i].type = AndamBusPropertyType::CUSTOM;
            propList[i].propertyId = 1;
            propList[i].value = 123;
            propList[i].entityId = it->id;
        }

        vector<TestItem*> bdl = getDevsOnBus(it->id);

        for (auto it2=bdl.begin();it2!=bdl.end();it2++) {
            propList[i].type = AndamBusPropertyType::LONG_ADDRESS;
            propList[i].propertyId = 0;
            propList[i].value = 0x12345678;
            propList[i++].entityId = (*it2)->id;

            propList[i].type = AndamBusPropertyType::LONG_ADDRESS;
            propList[i].propertyId = 1;
            propList[i].value = 0x10000000 + (*it2)->id;
            propList[i++].entityId = (*it2)->id;
        }
    }

    for (auto it=devs.begin();it!=devs.end();it++) {
        if (it->id == id || id == 0) {
            propList[i].type = AndamBusPropertyType::CUSTOM;
            propList[i].propertyId = 2;
            propList[i].value = 456;
            propList[i++].entityId = it->id;
        }
    }

    for (auto it=ports.begin();it!=ports.end();it++) {
        if (it->id == id || id == 0) {
            propList[i].type = AndamBusPropertyType::CUSTOM;
            propList[i].propertyId = 3;
            propList[i].value = 789;
            propList[i++].entityId = it->id;
        }
    }

    return i;
}

//------------------------------- Other -----------------------

void TestAndamBusUnit::readConfigPart(unsigned char *data, uint8_t bufsize, uint8_t page) {
//    AB_ERROR("readConfigPart " << (int)bufsize << " " << (int)page);
    uint16_t pagesize = (UNIT_BUFFER_SIZE/2);
    if (page*pagesize > cfgPos)
        return;
    if (bufsize < pagesize)
        return;
    if (page*pagesize+bufsize > cfgPos)
        bufsize = cfgPos - page*pagesize;
    memcpy(data, EEPROMemu + page*pagesize, bufsize);
}

uint16_t TestAndamBusUnit::getConfigSize() {
    return cfgPos;
}

void TestAndamBusUnit::restoreConfig(const uint8_t *data, uint8_t size, uint8_t page, bool finish) {
//    AB_ERROR("restoreConfig " << (int)size << " page " << (int)page << " finish=" << finish);

//    hexdump(reinterpret_cast<const char*>(data), size, LogLevel::AB_INFO);

/*    char buf[size+1];
    memcpy(buf, data, size);
    buf[size] = '\0';
    AB_INFO("data:" << buf << " " << hex << (int)data[56]);*/

    uint16_t pagesize = (UNIT_BUFFER_SIZE/2);
    memcpy(EEPROMemu + page*pagesize, data, size);

    cfgPos = page*pagesize + size;
    if (finish) {
        AB_INFO("EEPROM CRC=0x" << hex << crc32(0, EEPROMemu, page*pagesize+size));
    }
}

uint8_t TestAndamBusUnit::getMetadataList(UnitMetadata mdl[], uint8_t count) {
    int i=0;
    mdl[i].type = MetadataType::DIGITAL_INPUT_PORTS;
    mdl[i].propertyId = 1;
    mdl[i++].value = 19;

    mdl[i].type = MetadataType::ANALOG_INPUT_PORTS;
    mdl[i].propertyId = 0;
    mdl[i++].value = 6;

    return i;
}

uint8_t TestAndamBusUnit::getChangedValues(ItemValue values[], bool full) {
    int i=0;
    for (auto it=ports.begin();it!=ports.end();it++) {
        if (it->getLastChange() > getLastPortRefresh()) {
            values[i].id = it->id;
            values[i].value = it->getValue();
            i++;
        }
    }

    return i;
}

uint8_t TestAndamBusUnit::getNewDevices(VirtualDevice devList[]) {
    int i=0;
    for (auto it=devs.begin();it!=devs.end();it++) {
//        AB_INFO("id:" << (int)it->id << " created:" << it->getCreated() << " lastrefresh:" << getLastItemRefresh());
        if (it->getCreated() > getLastItemRefresh()) {
            devList[i].id = it->id;
            devList[i].busId = it->parentId;
            devList[i].pin = it->pin;
            devList[i].type = static_cast<VirtualDeviceType>(it->type);
            i++;
        }
    }
    return i;
}

uint8_t TestAndamBusUnit::getNewPorts(VirtualPort portList[]) {
    int i=0;
    for (auto it=ports.begin();it!=ports.end();it++) {
//        AB_INFO("id:" << it->id << " created:" << it->getCreated() << " lastrefresh:" << getLastItemRefresh());
        if (it->getCreated() > getLastItemRefresh()) {
            portList[i].id = it->id;
            portList[i].deviceId = it->parentId;
            portList[i].pin = it->pin;
            portList[i].type = static_cast<VirtualPortType>(it->type);
            i++;
        }
    }
    return i;
}

uint8_t TestAndamBusUnit::getNewProperties(ItemProperty propertyList[], VirtualDevice devices[], uint8_t devCount, VirtualPort ports[], uint8_t portCount) {
    int i=0;
    for (auto it=devs.begin();it!=devs.end();it++) {
//        AB_INFO("id:" << (int)it->id << " created:" << it->getCreated() << " lastrefresh:" << getLastItemRefresh());
        TestItem &dev = *it;
        if (it->getCreated() > getLastItemRefresh()) {
            propertyList[i].entityId = dev.id;
            propertyList[i].type = AndamBusPropertyType::LONG_ADDRESS;
            propertyList[i].propertyId = 0;
            propertyList[i].value = 0x12345678;
            i++;
            propertyList[i].entityId = dev.id;
            propertyList[i].type = AndamBusPropertyType::LONG_ADDRESS;
            propertyList[i].propertyId = 1;
            propertyList[i].value = 0x10000000 + dev.id;
            i++;
        }
    }
    return i;
}

bool TestAndamBusUnit::checkBusActive(uint8_t id) {
    TestItem *bus = findBusById(id);
    return bus != nullptr;
}

uint8_t TestAndamBusUnit::getSwVersionMajor() {
    return ANDAMBUS_UNIT_SW_VERSION_MAJOR;
}

uint8_t TestAndamBusUnit::getSwVersionMinor() {
    return ANDAMBUS_UNIT_SW_VERSION_MINOR;
}

SlaveHwType TestAndamBusUnit::getHwType() {
    return SlaveHwType::TEST;
}

void TestAndamBusUnit::doPersist() {
    AB_INFO("Persisting");
}

void TestAndamBusUnit::softwareReset() {
    AB_INFO("Resetting");
}
