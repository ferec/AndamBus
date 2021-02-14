#ifndef TESTANDAMBUSUNIT_H
#define TESTANDAMBUSUNIT_H

#include "AndamBusUnit.h"
#include "TestItem.h"

#define ANDAMBUS_UNIT_SW_VERSION_MAJOR 0
#define ANDAMBUS_UNIT_SW_VERSION_MINOR 8

#include <vector>

class TestAndamBusUnit : public AndamBusUnit
{
    public:
        TestAndamBusUnit(BroadcastSocket &bs, uint16_t _address);
        virtual ~TestAndamBusUnit();

    protected:
        virtual AndamBusCommandError createSecondaryBus(SecondaryBus &bus, uint8_t pin);
        virtual uint8_t getSecondaryBusList(SecondaryBus busList[]);
        virtual void secondaryBusDetect(uint8_t id);
        virtual bool secondaryBusCommand(uint8_t id, OnewireBusCommand busCmd);
        virtual bool removeSecondaryBus(uint8_t id);
        virtual bool checkBusActive(uint8_t id);

        virtual uint8_t getVirtualDeviceList(VirtualDevice devList[]);
        virtual uint8_t getVirtualDevicesOnBus(VirtualDevice devList[], uint8_t busId);
        virtual AndamBusCommandError createDevice(uint8_t pin, VirtualDeviceType type, VirtualDevice &dev, VirtualPort ports[], uint8_t &portCount, ItemProperty props[], uint8_t &propCount);
        virtual bool removeVirtualDevice(uint8_t id);

        virtual uint8_t getVirtualPortList(VirtualPort portList[]);
        virtual uint8_t getVirtualPortsOnDevice(VirtualPort portList[], uint8_t devId);
        virtual uint8_t getChangedValues(ItemValue values[], bool full);
        virtual AndamBusCommandError setPortValue(uint8_t id, int32_t value);
        virtual AndamBusCommandError createPort(uint8_t pin, VirtualPortType type, VirtualPort &port, ItemProperty props[], uint8_t &propCount);
        virtual bool removePort(uint8_t id);

        virtual uint8_t getPropertyList(ItemProperty propList[], uint8_t size);
        virtual uint8_t getPropertyList(ItemProperty propList[], uint8_t size, uint8_t id);
        virtual AndamBusCommandError setPropertyList(ItemProperty propList[], uint8_t count);
        virtual AndamBusCommandError setData(uint8_t id, const char *data, uint8_t size);

        virtual uint8_t getMetadataList(UnitMetadata mdl[], uint8_t count);

        virtual uint8_t getNewDevices(VirtualDevice devList[]);
        virtual uint8_t getNewPorts(VirtualPort portList[]);
        virtual uint8_t getNewProperties(ItemProperty propertyList[], VirtualDevice devices[], uint8_t devCount, VirtualPort ports[], uint8_t portCount);

        virtual uint8_t getSwVersionMajor();
        virtual uint8_t getSwVersionMinor();
        virtual SlaveHwType getHwType();
        virtual void doPersist();
        virtual void softwareReset();

        virtual uint16_t getConfigSize();
        virtual void readConfigPart(unsigned char *data, uint8_t bufsize, uint8_t page);
        virtual void restoreConfig(const uint8_t *data, uint8_t size, uint8_t page, bool finish);
        virtual bool isInitialized() { return true; }

//      Testing methods
        bool pinInUse(uint8_t pin);
        TestItem* findBusById(uint8_t busId);
        TestItem* findDevById(uint8_t busId);
        TestItem* findPortById(uint8_t portId);
        void createDeviceOnBus(uint8_t pin, uint8_t type, uint8_t busId);
        void refreshThermometerValuesOnBus(uint8_t busId);
        std::vector<TestItem*> getDevsOnBus(uint8_t busId);
        std::vector<TestItem*> getPortsOnBus(uint8_t busId);
        void removeVirtualDeviceOnBus(uint8_t busId);
        void removeVirtualPortOnDevice(uint8_t devId);

    private:
        int32_t TESTVAL;
        uint16_t IDGEN;

        std::vector<TestItem> buses, devs, ports;
};

#endif // TESTANDAMBUSUNIT_H
