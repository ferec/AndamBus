#ifndef DEVICEPORT_H
#define DEVICEPORT_H

#include <inttypes.h>
#include "AndamBusTypes.h"

class DevicePort {
  public:
//    DevicePort();
  
    uint8_t id;
    VirtualPortType type;
    bool active;
};

#endif // DEVICEPORT_H
