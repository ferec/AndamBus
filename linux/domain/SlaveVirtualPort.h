#ifndef SLAVEVIRTUALPORT_H
#define SLAVEVIRTUALPORT_H

#include <inttypes.h>
#include <string>
#include "../shared/AndamBusTypes.h"
#include "PropertyContainer.h"

class SlaveVirtualDevice;

class SlaveVirtualPort : public PropertyContainer
{
    friend class AndamBusSlave;

    public:
        SlaveVirtualPort(uint8_t id, VirtualPortType type, SlaveVirtualDevice *dev, uint8_t _pin, uint8_t _ordinal);
        SlaveVirtualPort(VirtualPort &port, SlaveVirtualDevice *dev);
        virtual ~SlaveVirtualPort();

        uint8_t getIdOnDevice() { return idOnDevice; }

        uint8_t getId() { return id; }
        void setId(uint8_t _id) { id = _id; }
        uint8_t getPin() { return pin; }
        uint8_t getAge() { return age; }
        SlaveVirtualDevice* getDevice() { return device; }
        VirtualPortType getType() { return type; }
        int getValue() { return value; }

        void merge(SlaveVirtualPort *port);

//        static std::string getTypeString(VirtualPortType type);
//        static AndamBusPropertyValue portType2PropertyValue(VirtualPortType pt);
//        static VirtualPortType propertyValue2PortType(AndamBusPropertyValue pv);

    protected:

    private:
        uint8_t id;
        const uint8_t idOnDevice, pin;
        int value;
        VirtualPortType type;
        SlaveVirtualDevice *device;
        uint8_t age;
};

#endif // SLAVEVIRTUALPORT_H
