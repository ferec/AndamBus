#include "AndamBusSlave.h"
#include "SlaveVirtualDevice.h"
#include "shared/AndamBusExceptions.h"
#include "util.h"

using namespace std;

SlaveVirtualDevice::SlaveVirtualDevice(AndamBusSlave *_unit, VirtualDevice &dev, SlaveSecondaryBus *_bus, uint16_t _ordinal):unit(_unit),id(dev.id),pin(dev.pin),ordinal(_ordinal),bus(_bus),type(dev.type),status(dev.status) {
    if (bus != nullptr)
        bus->addVirtualDevice(this);
}

SlaveVirtualDevice::SlaveVirtualDevice(uint8_t _id, uint8_t _pin, SlaveSecondaryBus *_bus, VirtualDeviceType _type, uint8_t _status):id(_id),pin(_pin),ordinal(0),bus(_bus),type(_type),status(_status) {
    if (bus != nullptr)
        bus->addVirtualDevice(this);
}

SlaveVirtualDevice::~SlaveVirtualDevice()
{
    //dtor
}

string SlaveVirtualDevice::getTypeString(VirtualDeviceType type) {
    switch(type) {
        case VirtualDeviceType::THERMOMETER: return "Thermometer";
        case VirtualDeviceType::THERMOSTAT: return "Thermostat";
        case VirtualDeviceType::BLINDS_CONTROL: return "Blinds control";
        case VirtualDeviceType::PUSH_DETECTOR: return "Push detector";
        default: return std::to_string((int)type);
    }
}

void SlaveVirtualDevice::setStatus(uint8_t _status) {
    status = _status;
}

uint64_t SlaveVirtualDevice::getLongAddress() {
    try {
        uint64_t la = getPropertyValue(AndamBusPropertyType::LONG_ADDRESS, 0) + getPropertyValue(AndamBusPropertyType::LONG_ADDRESS, 1) * 0x100000000ll;
        return la;
    } catch (PropertyException &ex) {
        AB_DEBUG("Property not found " << ex.what());
    }

    return 0;
}

void SlaveVirtualDevice::merge(SlaveVirtualDevice *dev) {
    id = dev->id;
    type = dev->type;
    status = dev->status;

    swap(dev->getProperties());
}

uint64_t SlaveVirtualDevice::getPermanentId() {
    if (bus == nullptr)
        return unit->getAddress()*0x100+pin;
    uint64_t la = getLongAddress();
    if (la >= 0x10000)
        return la;
    return 0;
}
