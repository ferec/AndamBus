#ifndef CHANGELISTENER_H_INCLUDED
#define CHANGELISTENER_H_INCLUDED

#include "domain/SlaveVirtualPort.h"

class AndamBusSlave;

class ChangeListener {
public:

    enum class ItemType:uint8_t {
        Bus,Device,Port
    } ;

    virtual void valueChanged(AndamBusSlave *unit, SlaveVirtualPort *port, int32_t oldValue);
    virtual void idChanged(AndamBusSlave *unit, ItemType type, uint8_t oldId, uint8_t newId);
};

#endif // CHANGELISTENER_H_INCLUDED
