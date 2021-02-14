#ifndef ANDAMBUSMASTER_H
#define ANDAMBUSMASTER_H

#include <string>
#include <vector>
#include <mutex>

#include "BroadcastSocket.h"
#include "shared/AndamBusTypes.h"
#include "domain/SlaveVirtualDevice.h"
#include "util.h"

#define ANDAMBUS_ADDRESS_MIN 1
#define ANDAMBUS_ADDRESS_MAX 5
#define ANDAMBUS_UNITS_LIMIT 16

#define ANDAMBUS_LIB_VERSION_MAJOR 1
#define ANDAMBUS_LIB_VERSION_MINOR 0

#define ANDAMBUS_RESYNC_TIMEOUT_MS 400
#define ANDAMBUS_RESYNC_LIMIT 10
#define ANDAMBUS_RETRY_COUNT_MAX 2

#define ANDAMBUS_BUFFER_SIZE 2048


class AndamBusSlave;

class AndamBusMaster
{

    friend class AndamBusSlave;
    public:

        AndamBusMaster(BroadcastSocket &sock);
//        AndamBusMaster(std::string tty);
        virtual ~AndamBusMaster();

        static uint8_t getLibVersionMajor() { return ANDAMBUS_LIB_VERSION_MAJOR; }
        static uint8_t getLibVersionMinor() { return ANDAMBUS_LIB_VERSION_MINOR; }
        static uint8_t getApiVersionMajor() { return ANDAMBUS_API_VERSION_MAJOR; }
        static uint8_t getApiVersionMinor() { return ANDAMBUS_API_VERSION_MINOR; }

        void detectSlaves();
        void detectSlave(uint16_t address);
        void refreshSlaves();
        void refreshSlave(AndamBusSlave *slave);

        void refreshPortValues(bool full);
        void refreshSlave(uint16_t addr);
        void refreshVirtualItems(AndamBusSlave *slave);
        void persistSlave(uint16_t address);
        void doReset(uint16_t address);
        uint16_t getConfig(uint16_t addr, uint8_t *data, uint16_t max_size);
        void restoreConfig(uint16_t addr, uint8_t *data, uint16_t size);

        std::vector<AndamBusSlave*>& getSlaves() { return slaves; }

        AndamBusSlave* findSlaveByAddress(uint16_t addr);
        SlaveVirtualDevice* getVirtualDeviceByPermanentID(uint64_t permId);

        unsigned int getInvalidFrameCount() { return cntInvalidFrm; }
        unsigned int getSyncLostCount() { return cntSyncLost;}
        unsigned int getBusTimeoutCount() { return cntBusTimeout; }
        unsigned int getCommandErrorCount() { return cntCmdError; }
        unsigned int getSentCount() { return cntSent; }
        unsigned int getReceivedCount() { return cntReceived; }

    protected:

        // communication methods
        void sendCommandWithResponse(AndamBusFrameCommand &cmd, uint16_t addr, AndamBusFrameResponse &resp, AndamBusFrameHeader &hdr);
        void getResponse(AndamBusCommand cmd, AndamBusFrame &resp);
        void checkResponse(AndamBusCommand cmd, AndamBusFrameResponse &resp);

        size_t calculateFrameSize(AndamBusCommand cmd, uint8_t propCount);

        // slave helper methods
        void refreshSecondaryBuses(AndamBusSlave *slave);
        void refreshVirtualDevices(AndamBusSlave *slave, uint8_t busId);
        void refreshVirtualPorts(AndamBusSlave *slave, uint8_t devId);
        void refreshProperties(AndamBusSlave *slave, uint8_t itemId);
        void refreshVirtualDevices(AndamBusSlave *slave);
        void refreshVirtualPorts(AndamBusSlave *slave);
        void refreshProperties(AndamBusSlave *slave);
        void refreshMetadata(AndamBusSlave *slave);
        void deduplicateLongAddressDevices(AndamBusSlave *slave);

        void refreshSlavePortValues(AndamBusSlave *slave, bool full);
        void removeSlave(uint16_t addr);
        void doPropertySet(uint16_t addr, uint8_t id, AndamBusPropertyType prop, int32_t value, uint8_t propId);
        void sendData(uint16_t addr, uint8_t id, uint8_t size, const char *data);


        void createVirtualDevice(uint16_t addr, uint8_t pin, VirtualDeviceType type, VirtualDevice &vdev, VirtualPort ports[], size_t &portCount, ItemProperty props[], size_t &propCount);
        void removeVirtualDevice(uint16_t addr, uint8_t devId);

        SecondaryBus createSecondaryBus(uint16_t addr, uint8_t pin);
        void removeSecondaryBus(uint16_t addr, uint8_t busId);
        void secondaryBusDetect(uint16_t addr, uint8_t busId);
        void secondaryBusRunCommand(uint16_t addr, uint8_t busId, OnewireBusCommand cmd);

        void setVirtualPortValue(uint16_t addr, uint8_t portId, int value);
        VirtualPort createVirtualPort(uint16_t addr, uint8_t pin, VirtualPortType type);
        void removeVirtualPort(uint16_t addr, uint8_t portId);

        void restoreConfigPart(uint16_t addr, uint8_t *data, uint8_t size, uint8_t page, bool finish);

        SlaveVirtualDevice* getVirtualDeviceByLongAddress(uint64_t longaddr);

/*

        void fillFrameHeader(AndamBusFrameHeader &hdr, uint16_t address);
        void finalizeFrame(AndamBusFrameCommand &cmd, size_t size);
        void checkCrc(AndamBusFrameResponse &resp);


        void trySync(AndamBusFrameResponse &resp);

        void refreshSlavePortValues(AndamBusSlave *slave);




        bool sendSimpleCommand(AndamBusFrameCommand &fc);
        bool sendCommandWithResponse(AndamBusFrameCommand &fc, size_t callSize, AndamBusFrameResponse &resp);
*/

    private:
        BroadcastSocket &bsock;

        std::vector<AndamBusSlave*> slaves;

        bool inSync;

        unsigned int cntInvalidFrm, cntSyncLost, cntBusTimeout,cntCmdError,cntSent,cntReceived;

        std::mutex mtTxn;
//        static fnLog logger;
};

#endif // ANDAMBUSMASTER_H
