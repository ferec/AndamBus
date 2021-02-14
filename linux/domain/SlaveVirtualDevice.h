#ifndef SLAVEVIRTUALDEVICE_H
#define SLAVEVIRTUALDEVICE_H

#include <inttypes.h>
#include <string>

#include "shared/AndamBusTypes.h"
#include "SlaveSecondaryBus.h"
#include "PropertyContainer.h"

class SlaveVirtualDevice : public PropertyContainer
{
    public:
        SlaveVirtualDevice(AndamBusSlave *unit, VirtualDevice &dev, SlaveSecondaryBus *_bus, uint16_t ordinal);
        /**
        for testing purposes
        */
        SlaveVirtualDevice(uint8_t id, uint8_t pin, SlaveSecondaryBus *bus, VirtualDeviceType type, uint8_t status);

        virtual ~SlaveVirtualDevice();

        uint64_t getPermanentId();
        AndamBusSlave *getUnit() { return unit; }

        uint8_t getId() { return id; }
        void setId(uint8_t _id) { id = _id; }

        uint8_t getPin() { return pin; }
        uint64_t getLongAddress();
        SlaveSecondaryBus* getBus() { return bus; }
        VirtualDeviceType getType() { return type; }
        uint16_t getOrdinal() { return ordinal; }

        void merge(SlaveVirtualDevice *dev);

        static std::string getTypeString(VirtualDeviceType type);
        void setStatus(uint8_t status);
        uint8_t getStatus() { return status; }
        bool isConfigured() { return (status & 1) == 0; }
        bool hasAllInputDevices() { return (status & 2) == 0; }
        bool hasInputError() { return (status & 4) != 0; }
    protected:

    private:
        AndamBusSlave *unit;
        uint8_t id;
        const uint8_t pin;
        const uint16_t ordinal;
        SlaveSecondaryBus *bus;
        VirtualDeviceType type;
        uint8_t status;
};

#endif // SLAVEVIRTUALDEVICE_H
