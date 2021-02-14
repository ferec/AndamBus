#include "SlaveSecondaryBus.h"

using namespace std;

SlaveSecondaryBus::SlaveSecondaryBus(uint8_t _id, uint8_t _pin, AndamBusSlave *_slave):id(_id),pin(_pin),slave(_slave),status(0) {
}

SlaveSecondaryBus::SlaveSecondaryBus(SecondaryBus &bus, AndamBusSlave *_slave):id(bus.id),pin(bus.pin),slave(_slave),status(0) {
}

SlaveSecondaryBus::~SlaveSecondaryBus()
{
    //dtor
}

void SlaveSecondaryBus::addVirtualDevice(SlaveVirtualDevice *dev) {
    devs.push_back(dev);
}

void SlaveSecondaryBus::setStatus(uint8_t _status) {
    status = _status;
}
