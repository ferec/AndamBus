#include "PropertyContainer.h"

#include "shared/AndamBusExceptions.h"

using namespace std;

PropertyContainer::PropertyContainer()
{
    //ctor
}

PropertyContainer::~PropertyContainer()
{
    //dtor
}

void PropertyContainer::setProperty(ItemProperty &prop) {
    properties[prop.type][prop.propertyId] = prop.value;
}

int32_t PropertyContainer::getPropertyValue(AndamBusPropertyType type, uint8_t id) {
    if (properties.count(type) == 0)
        throw PropertyException("Property does not exists");

    map<uint8_t,int32_t> &pm = properties[type];

    if (pm.count(id) == 0)
        throw PropertyException("Property id does not exists");
    return pm[id];
}

map<uint8_t,int32_t> PropertyContainer::getProperties(AndamBusPropertyType type) {
    if (properties.count(type) > 0)
        return properties[type];

    map<uint8_t,int32_t> m;
    return m;
}

void PropertyContainer::clearProperties() {
    properties.clear();
}
