#ifndef SLAVECAPABILITY_H
#define SLAVECAPABILITY_H

#include "shared/AndamBusTypes.h"

class SlaveCapability
{
    public:
        SlaveCapability();
        virtual ~SlaveCapability();

        static uint8_t minUsablePin(SlaveHwType hwType);
        static uint8_t maxUsablePin(SlaveHwType hwType);

        static uint8_t minUsablePinDigital(SlaveHwType hwType);
        static uint8_t maxUsablePinDigital(SlaveHwType hwType);

        static uint8_t minUsablePinAnalogInput(SlaveHwType hwType);
        static uint8_t maxUsablePinAnalogInput(SlaveHwType hwType);

        static uint8_t minUsablePinPwmOutput(SlaveHwType hwType);
        static uint8_t maxUsablePinPwmOutput(SlaveHwType hwType);

        static bool pinSupportsPortType(VirtualPortType type, uint8_t pin, SlaveHwType hwType);
        static bool pinSupportsDeviceType(VirtualPortType type, uint8_t pin, SlaveHwType hwType);
    protected:

    private:
};

#endif // SLAVECAPABILITY_H
