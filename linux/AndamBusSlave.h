#ifndef ANDAMBUSSLAVE_H
#define ANDAMBUSSLAVE_H

#include <inttypes.h>
#include <iostream>
#include <vector>
#include <set>

#include "AndamBusMaster.h"

#include "domain/SlaveVirtualPort.h"
#include "domain/SlaveVirtualDevice.h"
#include "domain/SlaveSecondaryBus.h"

#include "domain/PropertyContainer.h"
#include "ChangeListener.h"

class AndamBusSlave : public PropertyContainer
{
    friend class AndamBusMaster;

    public:
        AndamBusSlave(AndamBusMaster &master, uint16_t addr, SlaveHwType hwType, uint32_t swVersion, uint16_t apiVersion);
        virtual ~AndamBusSlave();

        //generic
        PropertyContainer* getPropertyContainer(uint8_t id);

        void updateProperties(size_t cnt, ItemProperty *prop);
        void doPropertySet(uint8_t id, AndamBusPropertyType prop, int32_t value, uint8_t propId);
        void clearProperties();
        uint16_t getAddress() { return address; }
        uint16_t getNextCounter();
        void refreshMetadata();
        uint16_t getConfig(uint8_t *data, uint16_t max_size);
        void restoreConfig(uint8_t *data, uint16_t size);

        void sendData(uint8_t id, uint8_t size, const char *data);

        void commitIncompleteItems();
        void dropIncompleteItems();

        void refreshVirtualItems(); // get newly created devices and ports
        //void doReset();

        // secondary bus
        std::vector<SlaveSecondaryBus*>& getSecondaryBuses() { return buses; }
        SecondaryBus createSecondaryBus(uint8_t pin);
        void removeSecondaryBus(uint8_t busId);
        void secondaryBusDetect(uint8_t busId);
        void secondaryBusRunCommand(uint8_t busId, OnewireBusCommand cmd);
        SlaveSecondaryBus* getBus(uint8_t busId);

        // virtual devices
        std::vector<SlaveVirtualDevice*>& getVirtualDevices() { return devs; }
        SlaveVirtualDevice* getVirtualDeviceByPin(uint8_t pin);
        SlaveVirtualDevice* getVirtualDeviceByLongAddress(uint64_t longaddr);

//        void addVirtualDevice(VirtualDevice &sb);
        SlaveVirtualDevice* createVirtualDevice(VirtualDeviceType type, uint8_t pin);
        SlaveVirtualDevice* getDevice(uint8_t deviceId);
        SlaveVirtualDevice* getIncompleteDevice(uint8_t deviceId);

        void createVirtualThermostat(uint8_t pin);
        void removeVirtualDevice(uint8_t devId);

        // virtual port
        std::vector<SlaveVirtualPort*>& getVirtualPorts() { return ports; }
        std::vector<SlaveVirtualPort*> getVirtualDevicePorts(uint8_t devId);
        std::vector<SlaveVirtualPort*> getVirtualDevicePortsByPin(uint8_t pin);
        std::vector<SlaveVirtualPort*> getVirtualDevicePortsByLongAddress(uint64_t longaddr);
        SlaveVirtualPort* getVirtualPortByPinOrdinal(uint8_t pin, uint8_t ordinal);
        SlaveVirtualPort* getVirtualDevicePortByOrdinal(uint8_t devId, uint8_t ordinal);

        void setPortValue(SlaveVirtualPort *port, int value);
        void setPortValue(uint8_t portId, int value);
        void refreshPortValues(bool full);
        SlaveVirtualPort* createVirtualPort(uint8_t pin, VirtualPortType type);
        void removeVirtualPort(uint8_t portId);
        SlaveVirtualPort *getPort(uint8_t portId);
        SlaveVirtualPort *getIncompletePort(uint8_t portId);
        std::set<uint8_t> getUnusedPins();

//        static std::string getHwTypeString(SlaveHwType type);
        std::string getHwTypeString();
        std::string getApiVersionString();
        std::string getSWVersionString();
        SlaveHwType getHwType() { return hwType; }
        uint16_t getApiVersion();
        uint32_t getSWVersion();




        std::vector<SlaveVirtualPort*>& getPorts() {return ports; }
        std::vector<SlaveSecondaryBus*>& getBuses() {return buses; }
        std::vector<SlaveVirtualDevice*>& getDevices() {return devs; }

        void setChangeListener(ChangeListener *lst);

        void resetErrorCounter();
        void incrementErrorCounter();

    protected:

        void removeBusFromList(uint8_t busId);
        void removeDeviceFromList(uint8_t devId);
        void removePortFromList(uint8_t portId);

        void removeDevicePorts(uint8_t devId);

        void updateMetadata(size_t cnt, UnitMetadata *metadata);
        void updateMetadata(MetadataType tp, uint16_t propId, uint32_t value);
        void updatePorts(size_t cnt, VirtualPort *ports);
        void updatePortValues(size_t cnt, ItemValue *ivals);
        void updateVirtualDevices(size_t cnt, VirtualDevice *sb);
        void updateSecondaryBuses(size_t cnt, SecondaryBus *sb);

    private:
        uint16_t address;
        AndamBusMaster &master;

//        std::map<uint8_t,SlaveVirtualPort*> mports;

        std::vector<SlaveVirtualPort*> ports, incompletePorts;
        std::vector<SlaveSecondaryBus*> buses;
        std::vector<SlaveVirtualDevice*> devs, incompleteDevs;

        const SlaveHwType hwType;
        const uint32_t swVersion;
        const uint16_t apiVersion;
        uint16_t counter, ordinal;
        uint16_t cntErr;

        ChangeListener *chgLst;
};

#endif // ANDAMBUSSLAVE_H
