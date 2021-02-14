#ifndef SLAVESECONDARYBUS_H
#define SLAVESECONDARYBUS_H

#include <stdint.h>
#include <vector>

#include "shared/AndamBusTypes.h"
#include "PropertyContainer.h"

//using namespace std;

class SlaveVirtualDevice;
class AndamBusSlave;

class SlaveSecondaryBus : public PropertyContainer
{
    public:
/*        enum class Command:uint8_t {
            W1_CONVERT = 0x44
        };*/

        SlaveSecondaryBus(SecondaryBus &bus, AndamBusSlave *slave);
        SlaveSecondaryBus(uint8_t id, uint8_t pin, AndamBusSlave *slave);
        virtual ~SlaveSecondaryBus();

        uint8_t getId() { return id; }
        uint8_t getPin() { return pin; }

        bool isDeviceMissing() { return status&1; }

        void addVirtualDevice(SlaveVirtualDevice *dev);

        void setStatus(uint8_t status);

    protected:

    private:
        const uint8_t id, pin;
        std::vector<SlaveVirtualDevice*> devs;
        AndamBusSlave *slave;
        uint8_t status;
};

#endif // SLAVESECONDARYBUS_H
