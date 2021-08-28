#ifndef ARDUINOANDAMBUSUNIT_H
#define ARDUINOANDAMBUSUNIT_H



// #if defined(__AVR_ATmega2560__) // Arduino Mega2560 config
#define MAX_VIRTUAL_PORTS 0x40                  // ID of ports are 0x0-0x2F
#define MAX_VIRTUAL_DEVICES 32
#define MAX_SECONDARY_BUSES 4
//#define ArduinoMega
/*#else
#define MAX_VIRTUAL_PORTS 0x2                  // ID of ports are 0x0-0x2F
#define MAX_VIRTUAL_DEVICES 2
#define MAX_SECONDARY_BUSES 0
#warning "Other"
#endif*/

#define SECONDARY_BUS_OFFSET MAX_VIRTUAL_PORTS  // ID of buses are 0x20-0x43

#define ANDAMBUS_UNIT_SW_VERSION_MAJOR 1
#define ANDAMBUS_UNIT_SW_VERSION_MINOR 5
#define ANDAMBUS_UNIT_SW_VERSION_BUILD 2
#define ANDAMBUS_UNIT_SW_BUILD_DATE __DATE__

#define ARDUINO_SERIAL_RX_PIN 17
#define ARDUINO_SERIAL_TX_PIN 16

#define EEPROM_MAGICWORD 0x36f5d98d
#define EEPROM_MAGICWORD_OLD 0x36f5d98c

#define MAX_PERSIST_ITEM_SIZE 0x80
#define MAX_PERSIST_SIZE 2048

#define BUS_WORK_PERIOD_MILLIS 1000
#define DEV_WORK_PERIOD_MILLIS 5000

#include "AndamBusUnit.h"
//#include "ArduinoAndamBusUnit.h"
#include "ArduinoPin.h"

#include "ArduinoW1.h"

class ArduinoDevice;
class DevicePort;

class ArduinoAndamBusUnit:public AndamBusUnit
{        
    friend class ArduinoW1;
    friend class ArduinoDevice;
    friend class ArduinoDevFact;
	friend class DevPortHelper;
	friend class BusDeviceHelper2;

	public:
		ArduinoAndamBusUnit(HardwareSerial &ser, uint8_t pinTransmit, uint16_t unitAddress);
//		ArduinoAndamBusUnit(SoftwareSerial &ser, uint8_t pinTransmit, uint16_t unitAddress);
        void createTestItems();
        void doTestConvert();
        void init();

        virtual void doWork();


        ArduinoW1* getBusById(uint8_t id);
        ArduinoW1* getBusByIndex(uint8_t idx);
        uint8_t getBusIdByPin(uint8_t pin);
        ArduinoDevice* getDeviceById(uint8_t id);
        ArduinoDevice* getDeviceByPin(uint8_t pin);
		
        W1Slave* getBusDeviceById(uint8_t id, uint8_t &busIndex, uint8_t &devIndex);
        W1Slave* getBusDeviceByIndex(uint8_t busIndex, uint8_t devIndex);
        W1Slave* getBusDeviceByAddress(DeviceAddress &deviceAddress, uint8_t &busIndex, uint8_t &devIndex);
        W1Slave* getBusDevicePortById(uint8_t id);
        DevicePort* getDevicePortById(uint8_t id);
		uint8_t getDeviceIndexByPin(uint8_t pin);
        uint8_t getDeviceId(uint8_t index);
        uint8_t getDeviceIndex(uint8_t id);

        void blockPin(uint8_t pin);
        void unblockPin(uint8_t pin);
        bool pinAvailable(uint8_t pin);
        static bool pinValid(uint8_t pin) { return pin > 0 && pin < MAX_VIRTUAL_PORTS; }
		
		void startInterrupt();
		void endInterrupt();

	    // slave
        virtual void doPersist();
    protected:
        virtual uint8_t getPropertyList(ItemProperty propList[], uint8_t size); // gets list of properties for slave/virtual devices/virtual ports/secondary buses
        virtual uint8_t getPropertyList(ItemProperty propList[], uint8_t size, uint8_t id); // gets list of properties for virtual device/virtual port/secondary bus identified by ID
        virtual AndamBusCommandError setPropertyList(ItemProperty propList[], uint8_t count); // sets slave property
		virtual AndamBusCommandError setData(uint8_t id, const char *data, uint8_t size);
        virtual uint8_t getMetadataList(UnitMetadata mdl[], uint8_t count); // sets one or more metadata item
        virtual uint8_t getNewDevices(VirtualDevice devList[]); // gets newly created devices after counter
        virtual uint8_t getNewPorts(VirtualPort portList[]); // gets newly created ports after counter
        virtual uint8_t getNewProperties(ItemProperty propertyList[], VirtualDevice devices[], uint8_t devCount, VirtualPort ports[], uint8_t portCount);
        virtual uint8_t getSwVersionMajor(); // get SW version major
        virtual uint8_t getSwVersionMinor(); // get SW version minor
        virtual SlaveHwType getHwType();
        virtual void softwareReset();
        virtual void restoreConfig(const uint8_t *data, uint8_t size, uint8_t page, bool finish);
        virtual uint16_t getConfigSize();
        virtual void readConfigPart(unsigned char *data, uint8_t size, uint8_t page);
        virtual bool isInitialized() { return refreshed; } // indicator that a unit data was already transfered to control unit, e.g. if in was restarted in the meantime and IDs can be assigned differently

        bool setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId);

        // secondary bus
		virtual uint8_t getSecondaryBusList(SecondaryBus busList[]); // gets list of secondary buses and returns count
        virtual AndamBusCommandError createSecondaryBus(SecondaryBus &bus, uint8_t pin); // creates secondary bus on pin
        virtual void secondaryBusDetect(uint8_t busId); // detects devices on secondary bus
        virtual bool secondaryBusCommand(uint8_t busId, OnewireBusCommand busCmd); // runs command on the selected secondary bus
        virtual bool removeSecondaryBus(uint8_t busId); // removes secondary bus

        // virtual device
        virtual uint8_t getVirtualDeviceList(VirtualDevice devList[]); // gets list of virtual devices and returns count
        virtual uint8_t getVirtualDevicesOnBus(VirtualDevice devList[], uint8_t busId); // gets list of virtual devices on specified bus and returns count
        virtual AndamBusCommandError createDevice(uint8_t pin, VirtualDeviceType type, VirtualDevice &vdev, VirtualPort ports[], uint8_t &portCount, ItemProperty props[], uint8_t &propCount); // creates virtual device, returns device info, created ports and properties
        virtual bool removeVirtualDevice(uint8_t id); // removes virtual device with ports

        // virtual port
        virtual AndamBusCommandError setPortValue(uint8_t id, int32_t value); // set port value, returns true if successful; otherwise (port does not exists) returns false
        virtual AndamBusCommandError createPort(uint8_t pin, VirtualPortType type, VirtualPort &port, ItemProperty props[], uint8_t &propCount); // creates virtual port
        virtual uint8_t getVirtualPortList(VirtualPort portList[]); // gets list of virtual ports and returns count
        virtual uint8_t getVirtualPortsOnDevice(VirtualPort portList[], uint8_t devId); // gets list of virtual ports on specified device and returns count
        virtual uint8_t getChangedValues(ItemValue values[], bool full); // gets list of items changed from last refresh
        virtual bool removePort(uint8_t id); // removes virtual port


		// general methods
        uint8_t getID();

        void restoreUnit();

        void restoreBus(uint8_t data[], uint8_t size);
        void restoreDevice(uint8_t data[], uint8_t size);
        void restorePin(uint8_t data[], uint8_t size);


        virtual bool checkBusActive(uint8_t id);

		// ID management
        uint8_t getPortId(uint8_t index);
        uint8_t getPortIndex(uint8_t id);
		uint8_t getDeviceIndexOwningPort(uint8_t id);
        uint8_t getBusId(uint8_t index);
        uint8_t getBusIndex(uint8_t id);
        


	private:
	  BroadcastSocket bs;
	  long int maxDuration,maxDurationBus, maxDurationDev,maxDurationOvr,maxDurationParent,durationRange0, durationRange1, durationRange2, durationRange3, durationRange4, durationRange5;

      unsigned long lastBusWork, lastDevWork, lastWork, uptimeSec, iterations;
	  uint8_t interrupt;
	  bool refreshed;
	  
	  void setInterruptPin(uint8_t pin);


    protected:
        ArduinoW1 *buses[MAX_SECONDARY_BUSES];
        ArduinoDevice *devs[MAX_VIRTUAL_DEVICES];
        ArduinoPin pins[MAX_VIRTUAL_PORTS];
};

#endif // ARDUINOANDAMBUSUNIT_H
