#ifndef PROPERTYCONTAINER_H
#define PROPERTYCONTAINER_H

#include "../shared/AndamBusTypes.h"

#include <map>
#include <string>


class PropertyContainer
{
    public:
        PropertyContainer();
        virtual ~PropertyContainer();

        void setProperty(ItemProperty &prop);
        int32_t getPropertyValue(AndamBusPropertyType type, uint8_t id);
        std::map<AndamBusPropertyType,std::map<uint8_t,int32_t>> &getProperties() { return properties; }
        std::map<uint8_t,int32_t> getProperties(AndamBusPropertyType type);
        void clearProperties();
    protected:
        void swap(std::map<AndamBusPropertyType,std::map<uint8_t,int32_t>> props) { properties.swap(props); }

    private:
        std::map<AndamBusPropertyType,std::map<uint8_t,int32_t>> properties;
};

#endif // PROPERTYCONTAINER_H
