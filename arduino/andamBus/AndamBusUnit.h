#ifndef ANDAMBUSUNIT_H
#define ANDAMBUSUNIT_H

#define UNIT_BUFFER_SIZE 128

#include "BroadcastSocket.h"
#include <inttypes.h>

class AndamBusUnit
{
    public:
        AndamBusUnit(BroadcastSocket &bs, uint16_t _address);
        virtual ~AndamBusUnit();

        void setAddress(uint16_t _address) { address = _address; }
        uint16_t getAddress() { return address; }
        virtual void doWork();
    protected:

// response methods
        void responseOk();
        void responseDetect();
        void responsePortList(uint8_t devId);
        void responsePortCreate(uint8_t pin, VirtualPortType type);
        void responseDeviceList(uint8_t busId);
        void responseBusList();
        void responsePropertyList();
        void responsePropertyList(uint8_t id);
        void responseValuesChanged(bool full);
        void responseError(AndamBusCommandError code);
        void responseBusCreate(uint8_t pin);
        void responseDeviceCreate(uint8_t pin, VirtualDeviceType type);
        void responseNewItems();
        void responseMetadata();
        void responseGetConfig(); // send config BLOB to master

  // general methods
        void startResponse(AndamBusFrameResponse &resp);
        void finishResponse();
        void processCommand(AndamBusFrameCommand &cmd);
        void sendProperty(ItemProperty &prop);
        void sendMetadata(UnitMetadata &md);
        void sendSecondaryBus(SecondaryBus &bus);
        void sendVirtualDevice(VirtualDevice &dev);
        void sendVirtualPort(VirtualPort &port);
        void sendItemValue(ItemValue &val);
        void sendRawData(unsigned const char *data, uint8_t size);

        uint16_t getLastPortRefresh() { return lastPortRefresh; }
        uint16_t getLastItemRefresh() { return lastItemRefresh; }
        uint16_t getCounter() { return counter; }

  // Command execution methods
          // slave
        virtual uint8_t getPropertyList(ItemProperty propList[], uint8_t size) = 0; // gets list of properties for slave/virtual devices/virtual ports/secondary buses
        virtual uint8_t getPropertyList(ItemProperty propList[], uint8_t size, uint8_t id) = 0; // gets list of properties for virtual device/virtual port/secondary bus identified by ID
        virtual AndamBusCommandError setPropertyList(ItemProperty propList[], uint8_t count) = 0; // sets one or more property value
        virtual AndamBusCommandError setData(uint8_t id, const char *data, uint8_t size) = 0; // sets raw data on specified item
        virtual uint8_t getMetadataList(UnitMetadata mdl[], uint8_t count) = 0; // sets one or more metadata item
        virtual uint8_t getNewDevices(VirtualDevice devList[]) = 0; // gets newly created devices after counter
        virtual uint8_t getNewPorts(VirtualPort portList[]) = 0; // gets newly created ports after counter
        virtual uint8_t getNewProperties(ItemProperty propertyList[], VirtualDevice devices[], uint8_t devCount, VirtualPort ports[], uint8_t portCount) = 0; // gets properties on newly created ports and devices
        virtual uint8_t getSwVersionMajor() = 0; // get SW version major
        virtual uint8_t getSwVersionMinor() = 0; // get SW version minor
        virtual SlaveHwType getHwType() = 0; // get hardware type
        virtual void doPersist() = 0; // saves configuration to NV memory
        virtual void softwareReset() = 0; // resets slave device
        virtual void restoreConfig(const uint8_t *data, uint8_t size, uint8_t page, bool finish) = 0; // restores BLOB sent from master to config
        virtual uint16_t getConfigSize() = 0; // reads config BLOB size
        virtual void readConfigPart(unsigned char *data, uint8_t bufsize, uint8_t page) = 0; // reads config BLOB page
        virtual bool isInitialized() = 0; // indicator that a unit data was already transfered to control unit, e.g. if in was restarted in the meantime and IDs can be assigned differently

        // secondary bus
        virtual AndamBusCommandError createSecondaryBus(SecondaryBus &bus, uint8_t pin) = 0; // creates secondary bus on pin
        virtual uint8_t getSecondaryBusList(SecondaryBus busList[]) = 0; // gets list of secondary buses and returns count
        virtual void secondaryBusDetect(uint8_t busId) = 0; // detects devices on secondary bus
        virtual bool secondaryBusCommand(uint8_t busId, OnewireBusCommand busCmd) = 0; // runs command on the selected secondary bus
        virtual bool removeSecondaryBus(uint8_t busId)=0; // removes secondary bus
        virtual bool checkBusActive(uint8_t id) = 0; // returns if bus is in use

        // virtual device
        virtual uint8_t getVirtualDeviceList(VirtualDevice devList[]) = 0; // gets list of virtual devices and returns count
        virtual uint8_t getVirtualDevicesOnBus(VirtualDevice devList[], uint8_t busId) = 0; // gets list of virtual devices on specified bus and returns count
        virtual AndamBusCommandError createDevice(uint8_t pin, VirtualDeviceType type, VirtualDevice &vdev, VirtualPort ports[], uint8_t &portCount, ItemProperty props[], uint8_t &propCount) = 0; // creates virtual device, returns device info, created ports and properties
        virtual bool removeVirtualDevice(uint8_t id)=0; // removes virtual device with ports

        // virtual port
        virtual AndamBusCommandError setPortValue(uint8_t id, int32_t value) = 0; // set port value, returns true if successful; otherwise (port does not exists) returns false
        virtual AndamBusCommandError createPort(uint8_t pin, VirtualPortType type, VirtualPort &port, ItemProperty props[], uint8_t &propCount) = 0; // creates virtual port
        virtual uint8_t getVirtualPortList(VirtualPort portList[]) = 0; // gets list of virtual ports and returns count
        virtual uint8_t getVirtualPortsOnDevice(VirtualPort portList[], uint8_t devId) = 0; // gets list of virtual ports on specified device and returns count
        virtual uint8_t getChangedValues(ItemValue values[], bool full) = 0; // gets list of items changed from last refresh, all values for full = true
        virtual bool removePort(uint8_t id) = 0; // removes virtual port


/*        old methods
 *         virtual uint8_t getVirtualPortList(VirtualPort portList[]);
        virtual int createVirtualPort(uint8_t pin, VirtualPortType type, VirtualPort *port);
        virtual AndamBusCommandError setVirtualPortValue(uint8_t id, int32_t value);
*/

        static uint8_t getItemSize(ResponseContent rc);


    private:
        BroadcastSocket &bs;
        uint16_t address;
        uint16_t counter, lastPortRefresh, lastItemRefresh;

};

#endif // ANDAMBUSUNIT_H
