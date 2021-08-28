#include "AndamBusSlave.h"
#include "domain/SlaveVirtualDevice.h"

#include "shared/AndamBusExceptions.h"

#include <arpa/inet.h>
#include <bitset>

using namespace std;

AndamBusSlave::AndamBusSlave(AndamBusMaster &master, uint16_t addr, SlaveHwType _hwType, uint32_t _swVersion, uint16_t _apiVersion):
        address(addr),master(master),hwType(_hwType),swVersion(_swVersion),apiVersion(_apiVersion),counter(0),ordinal(0),cntErr(0),chgLst(nullptr)
{
    //ctor
}

AndamBusSlave::~AndamBusSlave()
{
    for (auto it=ports.begin(); it!=ports.end();it++)
        delete(*it);
    for (auto it=devs.begin(); it!=devs.end();it++)
        delete(*it);
    for (auto it=buses.begin(); it!=buses.end();it++)
        delete(*it);
}

void AndamBusSlave::updatePortValues(size_t cnt, ItemValue *ivals) {
    for (unsigned int i=0;i<cnt;i++) {

        uint8_t id=ivals[i].id;
        SlaveVirtualPort *p = getPort(id);

//        hexdump((const char*)&ivals[i], sizeof(ItemValue), LogLevel::AB_INFO);

        if (ivals[i].age > 0) {
            AB_DEBUG("updating port value for " << (int)id << " to " << ivals[i].value << " (age " << (int)ivals[i].age << ")");
        }
        else {
            AB_DEBUG("updating port value for " << (int)id << " to " << ivals[i].value);
        }

        if (p!=nullptr) {
            int32_t oldValue = p->value;
            int8_t oldAge = p->age;

            p->value = ivals[i].value;
            p->age = ivals[i].age;

            if (chgLst!=nullptr && oldValue != p->value)
                chgLst->valueChanged(this, p, oldValue);
            if (chgLst!=nullptr && oldAge != p->age) {
                chgLst->metadataChanged(this, p->getDevice(), MetadataType::TRY_COUNT, 0, p->age);
            }
        }


/*        if (mports.count(id) == 1)
            mports[id]->value = ivals[i].value;*/
    }
}

void AndamBusSlave::updatePorts(size_t cnt, VirtualPort *_ports) {
    AB_DEBUG("updatePorts " << (int)cnt);
    for (unsigned int i=0;i<cnt;i++) {
//        SlaveVirtualPort *p;

        uint8_t id=_ports[i].id;

        SlaveVirtualPort *port;

        SlaveVirtualDevice *idev = getIncompleteDevice(_ports[i].deviceId);

        if (idev != nullptr) {
            AB_DEBUG("creating incomplete port " << (int)_ports[i].id);
            port = new SlaveVirtualPort(_ports[i], idev);
            incompletePorts.push_back(port);
            continue;
        }

        SlaveVirtualDevice *dev = nullptr;

        if (_ports[i].deviceId == 0)
            port = getPort(id);
        else {
            dev = getDevice(_ports[i].deviceId);

            if (dev==nullptr) {
                AB_ERROR("missing device");
                continue;
            }

            if (dev->getBus() != nullptr)
                port = getPort(id);
            else
                port = getVirtualPortByPinOrdinal(_ports[i].pin, _ports[i].ordinal);
        }


        if (port == nullptr) {
            AB_DEBUG("AndamBusSlave create port " << (int)cnt << " " << (int)_ports[i].ordinal);
            port = new SlaveVirtualPort(_ports[i], dev);
            ports.push_back(port);
        } else {

            if (port->getId() != _ports[i].id) {
                if (chgLst != nullptr)
                    chgLst->idChanged(this, ChangeListener::ItemType::Port, port->getId(), _ports[i].id);
                port->setId(_ports[i].id);
            }
        }

/*        if (mports.count(id) == 1)
            p = mports[id];
        else {
            p = new SlaveVirtualPort(_ports[i], getDevice(_ports[i].deviceId));
            ports.push_back(p);
            mports[id]=p;

            AB_INFO("new port " << (int)p->getId());
            }*/
        port->value = _ports[i].value;
    }
}

void AndamBusSlave::updateSecondaryBuses(size_t cnt, SecondaryBus *sb) {
    for (unsigned int i=0;i<cnt;i++) {
        SlaveSecondaryBus *bus = getBus(sb[i].id);
        if (bus == nullptr) {
            AB_DEBUG("creating bus on " << (int)sb[i].pin);
            buses.push_back(new SlaveSecondaryBus(sb[i], this));
        }
        else
            bus->setStatus(sb[i].status);
    }
}

void AndamBusSlave::updateVirtualDevices(size_t cnt, VirtualDevice *sb) {
    for (unsigned int i=0;i<cnt;i++) {
        SlaveVirtualDevice *dev = nullptr;
        if (sb[i].busId != 0) {
            AB_DEBUG("creating incomplete dev " << (int)sb[i].pin);
            incompleteDevs.push_back(new SlaveVirtualDevice(this, sb[i], getBus(sb[i].busId), ordinal++));
            continue;
        }


        dev = getVirtualDeviceByPin(sb[i].pin);
/*        else
            dev = getDevice(sb[i].id);*/

        if (dev == nullptr) {
            devs.push_back(new SlaveVirtualDevice(this, sb[i], getBus(sb[i].busId), ordinal++));
            AB_DEBUG("created device:" << (int)sb[i].id);
        } else {
            AB_DEBUG("device exists:" << (int)sb[i].id);
            dev->setStatus(sb[i].status);

            if (dev->getId() != sb[i].id) {
                if (chgLst != nullptr)
                    chgLst->idChanged(this, ChangeListener::ItemType::Device, dev->getId(), sb[i].id);
                dev->setId(sb[i].id);
            }
        }
    }
}

void AndamBusSlave::updateMetadata(size_t cnt, UnitMetadata *metadata) {
    for(unsigned int i=0;i<cnt;i++) {
        UnitMetadata &md = metadata[i];

        updateMetadata(md.type, md.propertyId, md.value);
        /*if (chgLst!=nullptr) {
            chgLst->metadataChanged(this, nullptr, md.type, md.propertyId, md.value);
        }*/

    }
}

void AndamBusSlave::updateMetadata(MetadataType tp, uint16_t propId, uint32_t value) {
    if (chgLst!=nullptr) {
        chgLst->metadataChanged(this, nullptr, tp, propId, value);
    }
}

void AndamBusSlave::resetErrorCounter() {
    if (cntErr>0) {
        cntErr=0;
        updateMetadata(MetadataType::TRY_COUNT, 0, 0);
    }
}

void AndamBusSlave::incrementErrorCounter() {
    updateMetadata(MetadataType::TRY_COUNT, 0, ++cntErr);
}

PropertyContainer* AndamBusSlave::getPropertyContainer(uint8_t id) {
    if (id == 0)
        return this;

    SlaveVirtualDevice *idev = getIncompleteDevice(id);
    if (idev != nullptr)
        return idev;
    SlaveVirtualPort *iport = getIncompletePort(id);
    if (iport != nullptr)
        return iport;


    SlaveSecondaryBus *bus = getBus(id);
    if (bus != nullptr)
        return bus;
    SlaveVirtualDevice *dev = getDevice(id);
    if (dev != nullptr)
        return dev;
    SlaveVirtualPort *port = getPort(id);
    if (port != nullptr)
        return port;
    return nullptr;
}

void AndamBusSlave::commitIncompleteItems() {
    for (auto it=incompleteDevs.begin();it!=incompleteDevs.end();) {
        SlaveVirtualDevice *idev = *it;
        uint64_t la = idev->getLongAddress();
        AB_DEBUG("incomplete dev 0x" << hex << la << " id " << dec << (int)idev->getId());

        if (la != 0) {
            SlaveVirtualDevice *dev = getVirtualDeviceByLongAddress(la);

            if (dev != nullptr) {
                AB_DEBUG("found dev 0x" << hex << la << " id " << dec << (int)dev->getId());
                if (dev->getId() != idev->getId()) {
                    if (chgLst!=nullptr)
                        chgLst->idChanged(this, ChangeListener::ItemType::Device, dev->getId(), idev->getId());
                    dev->merge(idev);
//                    delete idev;
                }
                it++;
            }
            else {
                AB_DEBUG("creating dev");
                devs.push_back(idev);
                it = incompleteDevs.erase(it);
            }
        } else
            it++;
    }

    for (auto it=incompletePorts.begin();it!=incompletePorts.end();it++) {
        SlaveVirtualPort *iport = *it;
        SlaveVirtualDevice *idev = iport->getDevice();

        SlaveVirtualPort *port = getVirtualDevicePortByOrdinal(idev->getId(), iport->getIdOnDevice());
        if (port != nullptr) {
            AB_DEBUG("found port " << (int)port->getId() << " vs " << (int)iport->getId());
            if (port->getId() != iport->getId()) {
                if (chgLst!=nullptr)
                    chgLst->idChanged(this, ChangeListener::ItemType::Port, port->getId(), iport->getId());
            }
            port->merge(iport);
            delete iport;
        } else {
            AB_DEBUG("creating port");
            ports.push_back(iport);
        }

    }

/*    for (auto it=incompletePorts.begin();it!=incompletePorts.end();it++) {
        delete *it;
    }*/

    for (auto it=incompleteDevs.begin();it!=incompleteDevs.end();it++) {
        delete *it;
    }

    dropIncompleteItems();
}

void AndamBusSlave::dropIncompleteItems() {
    incompletePorts.clear();
    incompleteDevs.clear();
}

void AndamBusSlave::updateProperties(size_t cnt, ItemProperty *prop) {
    for (unsigned int i=0;i<cnt;i++) {
        PropertyContainer *pc = getPropertyContainer(prop[i].entityId);

        if (pc != nullptr)
            pc->setProperty(prop[i]);
        else {
            AB_WARNING("Item with ID " << (int)prop[i].entityId << " not found");
        }

/*        AB_DEBUG("updateProperties:" << i);
        if (prop[i].entityId == 0)
            setProperty(prop[i]);
        SlaveSecondaryBus *bus = getBus(prop[i].entityId);
        if (bus!= nullptr) {
            AB_DEBUG("Setting " << propertyTypeString(prop->type) << " = " << prop->value);
            bus->setProperty(prop[i]);
            continue;
        }
        SlaveVirtualDevice *dev = getDevice(prop[i].entityId);
        if (dev != nullptr) {
            dev->setProperty(prop[i]);
            AB_DEBUG("DEV property set:" << propertyTypeString(prop[i].type));
            continue;
        }

        SlaveVirtualPort *port = getPort(prop[i].entityId);
        if (port != nullptr) {
            port->setProperty(prop[i]);
            continue;
        }*/
    }
}

void AndamBusSlave::setPortValue(uint8_t _portId, int _value) {
    AB_DEBUG("Setting value on " << (int)_portId << " to " << _value);

    SlaveVirtualPort *port = getPort(_portId);


    if (port == nullptr)
        return;

    setPortValue(port, _value);
}

void AndamBusSlave::setPortValue(SlaveVirtualPort *port, int _value) {
    AB_DEBUG("Setting value on " << (int)port->getId() << " to " << _value);

    master.setVirtualPortValue(getAddress(), port->getId(), _value);
    port->value = _value;
}

void AndamBusSlave::refreshMetadata() {
    master.refreshMetadata(this);
}

void AndamBusSlave::refreshPortValues(bool full) {
    AB_DEBUG("refreshPortValues");

        master.refreshSlavePortValues(this, full);
}

void AndamBusSlave::refreshVirtualItems() {
    master.refreshVirtualItems(this);
}

SlaveVirtualDevice* AndamBusSlave::createVirtualDevice(VirtualDeviceType type, uint8_t pin) {
    VirtualDevice dev;
    VirtualPort devports[ANDAMBUS_MAX_DEV_PORTS];
    ItemProperty devprops[ANDAMBUS_MAX_DEV_PORTS];
    size_t portCount, propCount;

    master.createVirtualDevice(getAddress(), pin, type, dev, devports, portCount, devprops, propCount);
    SlaveVirtualDevice *svd = new SlaveVirtualDevice(this, dev, getBus(dev.busId), ordinal++);
    devs.push_back(svd);

    for(size_t i=0;i<portCount;i++) {
        AB_INFO("creating dev port " << (int)devports[i].id);
        SlaveVirtualPort *svp = new SlaveVirtualPort(devports[i], svd);
        ports.push_back(svp);
    }

    for(size_t i=0;i<propCount;i++) {
        AB_INFO("creating dev property " << propertyTypeString(devprops[i].type) << " on " << (int)devprops[i].entityId);
        svd->setProperty(devprops[i]);
    }

    return svd;
}

SlaveVirtualPort* AndamBusSlave::createVirtualPort(uint8_t _pin, VirtualPortType _type) {
    VirtualPort vp = master.createVirtualPort(getAddress(), _pin, _type);

    SlaveVirtualPort *svp = new SlaveVirtualPort(vp, getDevice(vp.deviceId));
    ports.push_back(svp);

    return svp;
}

void AndamBusSlave::removeVirtualDevice(uint8_t devId) {
    master.removeVirtualDevice(getAddress(), devId);
    removeDevicePorts(devId);
    removeDeviceFromList(devId);
}

void AndamBusSlave::removeVirtualPort(uint8_t portId) {
    master.removeVirtualPort(getAddress(), portId);
    removePortFromList(portId);
}

void AndamBusSlave::removePortFromList(uint8_t portId) {
    for (auto it=ports.begin();it!=ports.end();it++)
        if ((*it)->getId() == portId) {
            it = ports.erase(it);
            return;
            }
}

void AndamBusSlave::removeDevicePorts(uint8_t devId) {
    for (auto it=ports.begin();it!=ports.end();it++) {
        SlaveVirtualPort *vp = *it;
        if (vp->getDevice() != nullptr && vp->getDevice()->getId() == devId) {
            it = ports.erase(it);

            if (it == ports.end())
                break;
        }
    }
}

void AndamBusSlave::removeBusFromList(uint8_t busId) {
    for (auto it=buses.begin();it!=buses.end();it++)
        if ((*it)->getId() == busId) {
            it = buses.erase(it);
            return;
        }
}

SecondaryBus AndamBusSlave::createSecondaryBus(uint8_t pin) {
    SecondaryBus sb = master.createSecondaryBus(getAddress(), pin);
    buses.push_back(new SlaveSecondaryBus(sb, this));

    return sb;
}

uint16_t AndamBusSlave::getConfig(uint8_t *data, uint16_t max_size) {
    return master.getConfig(getAddress(), data, max_size);
}

void AndamBusSlave::restoreConfig(uint8_t *data, uint16_t size) {
    master.restoreConfig(getAddress(), data, size);
}

void AndamBusSlave::sendData(uint8_t id, uint8_t size, const char *data) {
    master.sendData(getAddress(), id, size, data);
}

void AndamBusSlave::doPropertySet(uint8_t id, AndamBusPropertyType pt, int32_t value, uint8_t propId) {
    master.doPropertySet(getAddress(), id, pt, value, propId);

    ItemProperty prop;
    prop.type = pt;
    prop.entityId = id;
    prop.value = value;
    prop.propertyId = propId;

    updateProperties(1, &prop);
}


void AndamBusSlave::removeSecondaryBus(uint8_t busId) {
    master.removeSecondaryBus(getAddress(), busId);
    removeBusFromList(busId);
}

void AndamBusSlave::secondaryBusDetect(uint8_t busId) {
    master.secondaryBusDetect(getAddress(), busId);
}

void AndamBusSlave::secondaryBusRunCommand(uint8_t busId, OnewireBusCommand cmd) {
    master.secondaryBusRunCommand(getAddress(), busId, cmd);
}

void AndamBusSlave::removeDeviceFromList(uint8_t devId) {
    for (auto it=devs.begin();it!=devs.end();it++)
        if ((*it)->getId() == devId) {
            it = devs.erase(it);
            return;
            }
}

/*

void AndamBusSlave::removeVirtualDevice(uint8_t devId) {
    if (master.removeVirtualDevice(getAddress(), devId))
        removeDeviceFromList(devId);
}







void AndamBusSlave::createVirtualThermostat(uint8_t pin) {
    ItemProperty props[1] = {{AndamBusPropertyType::DEVICE_TYPE, 0,0, { .deviceType = VirtualDeviceType::THERMOSTAT}}};

//    props[0].type=AndamBusPropertyType::DEVICE_TYPE;
//    props[0].propValue=AndamBusPropertyValue::DEVICE_TYPE_THERMOSTAT;
    createVirtualDevice(VirtualDeviceType::THERMOSTAT, pin, props, 1);
}

VirtualDevice AndamBusSlave::createVirtualDevice(VirtualDeviceType type, uint8_t pin, ItemProperty props[], uint8_t propCnt) {
    VirtualDevice vd = master.createVirtualDevice(getAddress(), pin, type, props, propCnt);

    devs.push_back(new SlaveVirtualDevice(vd, getBus(vd.busId)));
    return vd;
}*/

SlaveSecondaryBus* AndamBusSlave::getBus(uint8_t busId) {
    for(auto it=buses.begin(); it!= buses.end();it++)
        if ((*it)->getId() == busId)
            return *it;

    return nullptr;
}

SlaveVirtualDevice* AndamBusSlave::getIncompleteDevice(uint8_t deviceId) {
    for(auto it=incompleteDevs.begin(); it!= incompleteDevs.end();it++)
        if ((*it)->getId() == deviceId)
            return *it;

    return nullptr;
}

SlaveVirtualDevice* AndamBusSlave::getDevice(uint8_t devId) {
    for(auto it=devs.begin(); it!= devs.end();it++)
        if ((*it)->getId() == devId)
            return *it;

    return nullptr;
}

SlaveVirtualPort* AndamBusSlave::getIncompletePort(uint8_t portId) {
    for(auto it=incompletePorts.begin(); it!= incompletePorts.end();it++)
        if ((*it)->getId() == portId)
            return *it;

    return nullptr;
}

SlaveVirtualPort* AndamBusSlave::getPort(uint8_t portId) {
    for(auto it=ports.begin(); it!= ports.end();it++)
        if ((*it)->getId() == portId)
            return *it;

    return nullptr;
}

SlaveVirtualPort* AndamBusSlave::getVirtualDevicePortByOrdinal(uint8_t devId, uint8_t ordinal) {
    for(auto it=ports.begin(); it!= ports.end();it++) {
        SlaveVirtualPort *port = *it;
        if (port->getDevice() != nullptr && port->getDevice()->getId() == devId && port->getIdOnDevice() == ordinal)
            return port;
    }

    return nullptr;
}

SlaveVirtualPort* AndamBusSlave::getVirtualPortByPinOrdinal(uint8_t pin, uint8_t ordinal) {
    for(auto it=ports.begin(); it!= ports.end();it++) {
        SlaveVirtualPort *port = *it;
        if (port->getPin() == pin && port->getIdOnDevice() == ordinal)
            return port;
    }

    return nullptr;
}

SlaveVirtualDevice* AndamBusSlave::getVirtualDeviceByPin(uint8_t pin) {
    for(auto it=devs.begin(); it!= devs.end();it++)
        if ((*it)->getPin() == pin)
            return *it;

    return nullptr;
}

vector<SlaveVirtualPort*> AndamBusSlave::getVirtualDevicePortsByPin(uint8_t pin) {
    vector<SlaveVirtualPort*> pl;
    for(auto it=ports.begin(); it!= ports.end();it++)
        if ((*it)->getDevice() != nullptr && (*it)->getDevice()->getPin() == pin)
            pl.push_back(*it);

    return pl;
}

SlaveVirtualDevice* AndamBusSlave::getVirtualDeviceByLongAddress(uint64_t longaddr) {
    for (auto it=devs.begin();it!=devs.end();it++) {
        if ((*it)->getLongAddress() == longaddr)
            return *it;
    }

    return nullptr;
}

vector<SlaveVirtualPort*> AndamBusSlave::getVirtualDevicePortsByLongAddress(uint64_t longaddr) {
    SlaveVirtualDevice *dev = getVirtualDeviceByLongAddress(longaddr);

    vector<SlaveVirtualPort*> pl;

    if (dev != nullptr)
        for(auto it=ports.begin(); it!= ports.end();it++)
            if ((*it)->getDevice() != nullptr && (*it)->getDevice()->getId() == dev->getId())
                pl.push_back(*it);

    return pl;
}

vector<SlaveVirtualPort*> AndamBusSlave::getVirtualDevicePorts(uint8_t devId) {
    vector<SlaveVirtualPort*> pl;
    for(auto it=ports.begin(); it!= ports.end();it++)
        if ((*it)->getDevice() != nullptr && (*it)->getDevice()->getId() == devId)
            pl.push_back(*it);

    return pl;
}

string AndamBusSlave::getHwTypeString() {
    switch (hwType) {
        case SlaveHwType::ARDUINO_UNO: return "Arduino Uno";
        case SlaveHwType::ARDUINO_DUE: return "Arduino Due";
        case SlaveHwType::ARDUINO_MEGA: return "Arduino Mega";
        case SlaveHwType::TEST: return "Testing";
        case SlaveHwType::UNKNOWN: return "Unknown";

        default: return std::to_string((int)hwType);
    }

}

string AndamBusSlave::getApiVersionString() {
    return std::to_string(apiVersion/0x100) + '.' + std::to_string(apiVersion%0x100);
}

string AndamBusSlave::getSWVersionString() {
    return std::to_string(swVersion/0x100) + '.' + std::to_string(swVersion%0x100);
}

uint16_t AndamBusSlave::getNextCounter() {
    return ++counter;
}

/*void AndamBusSlave::doReset() {
    master.doReset(getAddress());
}*/

void AndamBusSlave::setChangeListener(ChangeListener *lst) {
//    AB_WARNING("setChangeListener " << lst);
    chgLst = lst;
}

set<uint8_t> AndamBusSlave::getUnusedPins() {
    set<uint8_t> up;
    set<uint8_t> used;

    for (auto it = buses.begin(); it != buses.end();it++) {
        SlaveSecondaryBus *bus = *it;
        map<uint8_t,int32_t> busProps = bus->getProperties(AndamBusPropertyType::PIN);
        for (auto it = busProps.begin(); it != busProps.end();it++) {
            int32_t pin = it->second;
            used.insert(pin);
        }

        used.insert(bus->getPin());
    }

    for (auto it = devs.begin(); it != devs.end();it++) {
        SlaveVirtualDevice *dev = *it;
        map<uint8_t,int32_t> devProps = dev->getProperties(AndamBusPropertyType::PIN);
        for (auto it = devProps.begin(); it != devProps.end();it++) {
            int32_t pin = it->second;
            used.insert(pin);
        }

        used.insert(dev->getPin());
    }

    for (auto it = ports.begin(); it != ports.end();it++) {
        SlaveVirtualPort *port = *it;
        used.insert(port->getPin());
    }

    map<uint8_t,int32_t> pinProps = getProperties(AndamBusPropertyType::PIN);
    for (auto it = pinProps.begin(); it != pinProps.end();it++) {
        int32_t pin = it->second;
        used.insert(pin);
    }

    // starting with 3 - 0,1 is built-in serial, 2 is DO/DI flag
    for (int i=3;i<64;i++) {
        if (i==16 || i == 17) // serial2 pins
            continue;
        if (used.find(i) == used.end())
            up.insert(i);
    }

    return up;
}

uint16_t AndamBusSlave::getApiVersion() {
    return apiVersion;
}

uint32_t AndamBusSlave::getSWVersion() {
    return swVersion;
}
