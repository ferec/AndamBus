#include "SlaveVirtualPort.h"
#include "SlaveVirtualDevice.h"

#include <iostream>

using namespace std;

SlaveVirtualPort::SlaveVirtualPort(uint8_t _id, VirtualPortType type, SlaveVirtualDevice *dev, uint8_t _pin, uint8_t _ordinal):id(_id),idOnDevice(_ordinal),pin(_pin),value(0),device(dev),age(0) {
}

SlaveVirtualPort::SlaveVirtualPort(VirtualPort &port, SlaveVirtualDevice *dev):id(port.id),idOnDevice(port.ordinal),pin(port.pin),value(port.value),type(port.type),device(dev),age(0) {
}

SlaveVirtualPort::~SlaveVirtualPort()
{
    //dtor
}

void SlaveVirtualPort::merge(SlaveVirtualPort *port) {
    id = port->id;
    value = port->value;
    age = port->age;

    swap(port->getProperties());
}
