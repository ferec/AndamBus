#include "AndamBusMaster.h"
#include "AndamBusSlave.h"
#include "AndamBusUnit.h"
#include "shared/AndamBusExceptions.h"

#include "util.h"


#include <iostream>
#include <chrono>
#include <thread>
#include <arpa/inet.h>

#include <zlib.h>
#include <string.h>

using namespace std;

char buffer[ANDAMBUS_BUFFER_SIZE];

AndamBusMaster::AndamBusMaster(BroadcastSocket &sock):bsock(sock),inSync(true),
    cntInvalidFrm(0),cntSyncLost(0),cntBusTimeout(0),cntCmdError(0),cntSent(0),cntReceived(0)
{
}

AndamBusMaster::~AndamBusMaster()
{
}

AndamBusSlave* AndamBusMaster::findSlaveByAddress(uint16_t _addr) {
    for (auto it=slaves.begin();it!=slaves.end();it++) {
        AndamBusSlave *slave = (*it);

        if (slave->getAddress() == _addr)
            return slave;
    }

    return nullptr;
}

void AndamBusMaster::setVirtualPortValue(uint16_t address, uint8_t portId, int value) {
    AndamBusFrameCommand cmd = {AndamBusCommand::VIRTUAL_PORT_SET_VALUE};

    cmd.id = portId;
    cmd.propertyCount = 1;
    cmd.props[0].type = AndamBusPropertyType::PORT_VALUE;
    cmd.props[0].value = value;

    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(buffer);
    AndamBusFrameHeader hdr;

    sendCommandWithResponse(cmd, address, resp, hdr);
    AB_DEBUG("set slave port value " << address << " item count:" << (int)resp.typeOkData.itemCount);
}

void AndamBusMaster::doPropertySet(uint16_t addr, uint8_t id, AndamBusPropertyType prop, int32_t value, uint8_t propId)
{
    AndamBusFrameCommand cmd = {AndamBusCommand::SET_PROPERTY};

    cmd.id = 0;
    cmd.propertyCount = 1;
    cmd.props[0].type = prop;
    cmd.props[0].entityId = id;
    cmd.props[0].propertyId = propId;
    cmd.props[0].value = value;

    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(buffer);
    AndamBusFrameHeader hdr;

    sendCommandWithResponse(cmd, addr, resp, hdr);
    AB_DEBUG("set property value " << addr << " item count:" << (int)resp.typeOkData.itemCount);
}

void AndamBusMaster::sendData(uint16_t addr, uint8_t id, uint8_t size, const char *data)
{
    AndamBusFrameCommand cmd = {AndamBusCommand::SET_DATA};

    cmd.id = id;
    cmd.propertyCount = size;
    memcpy(cmd.data, data, size);

    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(buffer);
    AndamBusFrameHeader hdr;

    sendCommandWithResponse(cmd, addr, resp, hdr);
    AB_DEBUG("set raw data " << addr << " size:" << (int)resp.typeOkData.itemCount);
}

void AndamBusMaster::restoreConfig(uint16_t addr, uint8_t *data, uint16_t size)
{
    uint16_t pagesize = (UNIT_BUFFER_SIZE/2);
    uint8_t cnt = (size-1)/pagesize+1;
    for (int i=0;i<cnt;i++)
        restoreConfigPart(addr, data+i*pagesize, i<(cnt-1)?pagesize:(((size-1)%pagesize)+1), i, i==cnt-1);
//    throw runtime_error("not implemented");
    AB_INFO("restoring CRC=0x" << hex << crc32(0, data, size));
}

void AndamBusMaster::restoreConfigPart(uint16_t addr, uint8_t *data, uint8_t size, uint8_t page, bool finish) {
    AB_INFO("restoreConfigPart " << addr << " " << dec << (int)page << " size=" << (int)size << " finish=" << finish);
    AndamBusFrameCommand cmd = {AndamBusCommand::RESTORE_CONFIG};

    cmd.id = page;
    cmd.propertyCount = size;
    cmd.pin = finish;
    memcpy(cmd.data, data, size);

/*    char bufx[size+1];
    memcpy(bufx, data, size);
    bufx[size] = '\0';

    ERROR("data:" << bufx << " " << hex << (int)data[56]);*/

    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(buffer);
    AndamBusFrameHeader hdr;

    sendCommandWithResponse(cmd, addr, resp, hdr);
    this_thread::sleep_for(chrono::milliseconds(300));
}

uint16_t AndamBusMaster::getConfig(uint16_t addr, uint8_t *data, uint16_t max_size)
{
    AndamBusFrameCommand cmd = {AndamBusCommand::GET_CONFIG};

    cmd.id = 0;
    cmd.propertyCount = 0;

    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(buffer);
    AndamBusFrameHeader hdr;

    sendCommandWithResponse(cmd, addr, resp, hdr);

    // itemCount contains BLOB H byte, itemCount2 contains L byte
    uint16_t sz = resp.typeOkData.itemCount * 0x100 + resp.typeOkData.itemCount2;
    AB_DEBUG("getConfig " << addr << " size:" << (int)resp.typeOkData.itemCount);

    memcpy(data, resp.typeOkData.responseData.data, sz);
    return sz;
}

void AndamBusMaster::refreshSlavePortValues(AndamBusSlave *slave, bool full) {
    AndamBusFrameCommand cmd = {AndamBusCommand::VALUES_CHANGED};

    cmd.propertyCount = 0;
    cmd.pin = full?1:0;

    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(buffer);
    AndamBusFrameHeader hdr;

    try {
        sendCommandWithResponse(cmd, slave->getAddress(), resp, hdr);
        AB_INFO("refreshed slave " << slave->getAddress() << " port values item count:" << (int)resp.typeOkData.itemCount);
        slave->updatePortValues(resp.typeOkData.itemCount, resp.typeOkData.responseData.portValues);

        slave->resetErrorCounter();
    } catch (ExceptionBase &e) {
        slave->incrementErrorCounter();

        throw e;
    }

}

void AndamBusMaster::detectSlaves() {
    int retryCount = 0;
    for (int addr=ANDAMBUS_ADDRESS_MIN;addr<=ANDAMBUS_ADDRESS_MAX;addr++) {
        AB_DEBUG("detecting " << addr);
        try {
            detectSlave(addr);
        } catch (InvalidFrameException &e) {
            AB_WARNING("Communication failure, retrying " << ++retryCount << "..." << e.what());

            if (retryCount < ANDAMBUS_RETRY_COUNT_MAX)
                addr--;
            else
                retryCount = 0;

            inSync = false;
        }
    }
}

void AndamBusMaster::persistSlave(uint16_t address) {
    AndamBusFrameCommand cmd = {};
    cmd.command = AndamBusCommand::PERSIST;
    cmd.propertyCount = 0;
    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(buffer);
    AndamBusFrameHeader hdr;

    try {
        sendCommandWithResponse(cmd, address, resp, hdr);
    } catch (BusTimeoutException &e) {
        AB_INFO("Persist:" << e.what());
    }
}

void AndamBusMaster::doReset(uint16_t address) {
    AndamBusFrameCommand cmd = {};
    cmd.command = AndamBusCommand::RESET;
    cmd.propertyCount = 0;
    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(buffer);
    AndamBusFrameHeader hdr;

    try {
        sendCommandWithResponse(cmd, address, resp, hdr);

        removeSlave(address);
    } catch (BusTimeoutException &e) {
        AB_INFO("Reset:" << e.what());
    }
}

void AndamBusMaster::detectSlave(uint16_t address) {
    AndamBusFrameCommand cmd = {};

    cmd.command = AndamBusCommand::DETECT_SLAVE;

    cmd.propertyCount = 1;
    cmd.props[0].type = AndamBusPropertyType::SW_VERSION;
    cmd.props[0].value = getLibVersionMajor() *0x100 + getLibVersionMinor();

    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(buffer);
    AndamBusFrameHeader hdr;

    try {
        sendCommandWithResponse(cmd, address, resp, hdr);
        SlaveHwType hwType = SlaveHwType::UNKNOWN;
        uint32_t swVer = 0;

//        hexdump(reinterpret_cast<const char*>(&resp), 44, LogLevel::ERROR);

        if (resp.responseType == AndamBusResponseType::OK_DATA && resp.typeOkData.content == ResponseContent::PROPERTY) {
            const ItemProperty *propHwType = findPropertyByType(resp.typeOkData.responseData.props, resp.typeOkData.itemCount, AndamBusPropertyType::SLAVE_HW_TYPE);
            const ItemProperty *propSwVer = findPropertyByType(resp.typeOkData.responseData.props, resp.typeOkData.itemCount, AndamBusPropertyType::SW_VERSION);

            if (propHwType != nullptr)
                hwType = propHwType->hwType;

            if (propSwVer != nullptr)
                swVer = propSwVer->value;
        }

        if (findSlaveByAddress(address) == nullptr) {
            AndamBusSlave *abslave = new AndamBusSlave(*this, address, hwType, swVer, hdr.apiVersion.major*0x100+hdr.apiVersion.minor);
//            AB_INFO("detected slave " << address<<",HW Type:" << abslave->getHwTypeString()<< ",SW version:" << abslave->getSWVersionString() << ", API version:" << abslave->getApiVersionString());
            AB_INFO(unitInfoString(address, abslave->getHwTypeString(), hwType, abslave->getSWVersionString(), abslave->getApiVersionString()));
            slaves.push_back(abslave);
        } else
            AB_DEBUG("slave " << address<<" detected again");
    } catch (BusTimeoutException &e) {
        AB_INFO("Detect:" << e.what());
    }

}

void AndamBusMaster::refreshSlaves() {
    for (auto it=slaves.begin();it!=slaves.end();it++)
        refreshSlave(*it);
}

void AndamBusMaster::refreshSlave(uint16_t addr) {
    AndamBusSlave *slave = findSlaveByAddress(addr);

    if (slave != nullptr)
        refreshSlave(slave);
}

void AndamBusMaster::refreshSlave(AndamBusSlave *slave) {
    AB_INFO("refreshSlave");

    try {
        refreshSecondaryBuses(slave);
        refreshVirtualDevices(slave);
        refreshVirtualPorts(slave);
        refreshProperties(slave);
        slave->commitIncompleteItems();
    } catch (ExceptionBase &eb) {
        slave->dropIncompleteItems();
        throw;
    }
}

void AndamBusMaster::refreshMetadata(AndamBusSlave *slave) {
    AndamBusFrameCommand cmd = {AndamBusCommand::METADATA};
    cmd.propertyCount = 0;
    cmd.id = 0;

    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(buffer);
    AndamBusFrameHeader hdr;

    try {
        sendCommandWithResponse(cmd, slave->getAddress(), resp, hdr);
        AB_DEBUG("refreshed metadata " << slave->getAddress() << " item count:" << (int)resp.typeOkData.itemCount);
        slave->updateMetadata(resp.typeOkData.itemCount, resp.typeOkData.responseData.metadata);
    } catch (CommandException &e) {
        AB_WARNING("refreshMetadata:" << e.what());
    }
}

void AndamBusMaster::refreshProperties(AndamBusSlave *slave) {
    refreshProperties(slave, 0);
}

void AndamBusMaster::refreshProperties(AndamBusSlave *slave, uint8_t itemId) {
    AndamBusFrameCommand cmd = {AndamBusCommand::PROPERTY_LIST};
    cmd.propertyCount = 0;
    cmd.id = itemId;

    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(buffer);
    AndamBusFrameHeader hdr;

    try {
        sendCommandWithResponse(cmd, slave->getAddress(), resp, hdr);
        AB_DEBUG("refreshed slave properties " << slave->getAddress() << " item count:" << (int)resp.typeOkData.itemCount);
        slave->updateProperties(resp.typeOkData.itemCount, resp.typeOkData.responseData.props);
    } catch (CommandException &e) {
        AB_WARNING("refreshProperties:" << e.what());
    }
}

void AndamBusMaster::refreshVirtualItems(AndamBusSlave *slave) {
    AndamBusFrameCommand cmd = {AndamBusCommand::VIRTUAL_ITEMS_CREATED};
    cmd.propertyCount = 0;
    cmd.id = 0;

    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(buffer);
    AndamBusFrameHeader hdr;

    sendCommandWithResponse(cmd, slave->getAddress(), resp, hdr);
    AB_DEBUG("refreshed virtual items " << slave->getAddress() << " item count:" << (int)resp.typeOkData.itemCount);

    slave->updateVirtualDevices(resp.typeOkData.itemCount, resp.typeOkData.responseData.deviceItems);
    VirtualPort *ports = reinterpret_cast<VirtualPort*>(resp.typeOkData.responseData.deviceItems+resp.typeOkData.itemCount);
    slave->updatePorts(resp.typeOkData.itemCount2, ports);
    slave->updateProperties(resp.typeOkData.itemCount3, reinterpret_cast<ItemProperty*>(ports+resp.typeOkData.itemCount2));

}

// Virtual port methods

void AndamBusMaster::refreshVirtualPorts(AndamBusSlave *slave) {
    refreshVirtualPorts(slave, 0);
}

void AndamBusMaster::refreshVirtualPorts(AndamBusSlave *slave, uint8_t devId) {
    AndamBusFrameCommand cmd = {AndamBusCommand::VIRTUAL_PORT_LIST};
    cmd.propertyCount = 0;
    cmd.id = devId;

    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(buffer);
    AndamBusFrameHeader hdr;

    try {
        sendCommandWithResponse(cmd, slave->getAddress(), resp, hdr);
        AB_DEBUG("refreshed slave ports " << slave->getAddress() << " item count:" << (int)resp.typeOkData.itemCount);
        slave->updatePorts(resp.typeOkData.itemCount, resp.typeOkData.responseData.portItems);
    } catch (CommandException &e) {
        AB_WARNING("refreshVirtualPorts:" << e.what());
    }
}

VirtualPort AndamBusMaster::createVirtualPort(uint16_t addr, uint8_t pin, VirtualPortType type) {
    AndamBusFrameCommand cmd = {AndamBusCommand::VIRTUAL_PORT_CREATE};
    cmd.propertyCount = 1;
    cmd.pin = pin;
    cmd.id = 0;

    cmd.props[0].type = AndamBusPropertyType::PORT_TYPE;
    cmd.props[0].portType = type;

    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(buffer);
    AndamBusFrameHeader hdr;

    AB_DEBUG("Create virtual port addr:" << addr << " pin:" << (int)pin);

    sendCommandWithResponse(cmd, addr, resp, hdr);
    if (resp.typeOkData.itemCount == 1) {
        VirtualPort &vp = resp.typeOkData.responseData.portItems[0];
        uint8_t portId = vp.id;
        VirtualPortType portType = vp.type;
        AB_INFO("port " << (int)portId << " created of type " << virtualPortTypeString(portType));
        return vp;
    }
    throw ExceptionBase("Error creating port");
}

void AndamBusMaster::removeVirtualPort(uint16_t addr, uint8_t portId) {
    AndamBusFrameCommand cmd = {AndamBusCommand::VIRTUAL_PORT_REMOVE};
    cmd.id = portId;
    cmd.propertyCount = 0;

    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(buffer);
    AndamBusFrameHeader hdr;

    sendCommandWithResponse(cmd, addr, resp, hdr);
    AB_INFO("port " << (int)portId << " removed");
}


// Virtual port methods END

// Virtual device methods

void AndamBusMaster::refreshVirtualDevices(AndamBusSlave *slave) {
    refreshVirtualDevices(slave, 0);
}

void AndamBusMaster::createVirtualDevice(uint16_t addr, uint8_t pin, VirtualDeviceType type, VirtualDevice &vdev, VirtualPort ports[], size_t &portCount, ItemProperty props[], size_t &propCount) {
    AndamBusFrameCommand cmd = {AndamBusCommand::VIRTUAL_DEVICE_CREATE};
    cmd.pin = pin;
    cmd.id = 0;


    cmd.props[0].type = AndamBusPropertyType::DEVICE_TYPE;
    cmd.props[0].deviceType = type;
    cmd.propertyCount = 1;

    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(buffer);
    AndamBusFrameHeader hdr;

    sendCommandWithResponse(cmd, addr, resp, hdr);
    if (resp.typeOkData.itemCount == 1) {
        vdev = resp.typeOkData.responseData.deviceItems[0];
        uint8_t devId = vdev.id;
        VirtualDeviceType devType = vdev.type;
        AB_INFO("device " << (int)devId << " created of type " << SlaveVirtualDevice::getTypeString(devType));

        portCount = resp.typeOkData.itemCount2;
        VirtualPort *pl = reinterpret_cast<VirtualPort*>(resp.typeOkData.responseData.deviceItems+1);
        memcpy(ports, pl, portCount * sizeof(VirtualPort));

        propCount = resp.typeOkData.itemCount3;
        memcpy(props, reinterpret_cast<ItemProperty*>(pl+portCount), propCount * sizeof(ItemProperty));
    }
}

void AndamBusMaster::removeVirtualDevice(uint16_t addr, uint8_t devId) {
    AndamBusFrameCommand cmd = {AndamBusCommand::VIRTUAL_DEVICE_REMOVE};
    cmd.id = devId;
    cmd.propertyCount = 0;

    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(buffer);
    AndamBusFrameHeader hdr;

    sendCommandWithResponse(cmd, addr, resp, hdr);
    AB_INFO("device " << (int)devId << " removed");
}

// Virtual device methods END

// Secondary bus methods

SecondaryBus AndamBusMaster::createSecondaryBus(uint16_t addr, uint8_t pin) {
    AndamBusFrameCommand cmd = {AndamBusCommand::SECONDARY_BUS_CREATE};
    cmd.pin = pin;
    cmd.propertyCount = 0;

    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(buffer);
    AndamBusFrameHeader hdr;

    sendCommandWithResponse(cmd, addr, resp, hdr);
    if (resp.typeOkData.itemCount == 1)
        return resp.typeOkData.responseData.busItems[0];

    throw InvalidContentException("item count should be 1");
}

void AndamBusMaster::removeSecondaryBus(uint16_t addr, uint8_t busId) {
    AndamBusFrameCommand cmd = {AndamBusCommand::SECONDARY_BUS_REMOVE};
    cmd.pin = 0;
    cmd.id = busId;
    cmd.propertyCount = 0;

    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(buffer);
    AndamBusFrameHeader hdr;

    sendCommandWithResponse(cmd, addr, resp, hdr);
}

void AndamBusMaster::secondaryBusDetect(uint16_t addr, uint8_t busId) {
    AndamBusFrameCommand cmd = {AndamBusCommand::SECONDARY_BUS_DETECT};
    cmd.pin = 0;
    cmd.id = busId;
    cmd.propertyCount = 0;

    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(buffer);
    AndamBusFrameHeader hdr;

    sendCommandWithResponse(cmd, addr, resp, hdr);
}

void AndamBusMaster::secondaryBusRunCommand(uint16_t addr, uint8_t busId, OnewireBusCommand comm) {
    AndamBusFrameCommand cmd = {AndamBusCommand::SECONDARY_BUS_COMMAND};
    cmd.pin = 0;
    cmd.id = busId;
    cmd.propertyCount = 1;
    cmd.props[0].type = AndamBusPropertyType::W1CMD;
    cmd.props[0].cmd = comm;

    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(buffer);
    AndamBusFrameHeader hdr;

    sendCommandWithResponse(cmd, addr, resp, hdr);
}

// Secondary bus methods END

void AndamBusMaster::refreshVirtualDevices(AndamBusSlave *slave, uint8_t busId) {
    AndamBusFrameCommand cmd = {AndamBusCommand::VIRTUAL_DEVICE_LIST};
    cmd.propertyCount = 0;
    cmd.id = busId;

    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(buffer);
    AndamBusFrameHeader hdr;

    try {
        sendCommandWithResponse(cmd, slave->getAddress(), resp, hdr);
        AB_DEBUG("refreshed slave devices " << slave->getAddress() << " item count:" << (int)resp.typeOkData.itemCount);
        slave->updateVirtualDevices(resp.typeOkData.itemCount, resp.typeOkData.responseData.deviceItems);
    } catch (CommandException &e) {
        AB_WARNING("refreshVirtualDevices:" << e.what());
    }
}

void AndamBusMaster::refreshSecondaryBuses(AndamBusSlave *slave) {
    AndamBusFrameCommand cmd = {AndamBusCommand::SECONDARY_BUS_LIST};
    cmd.propertyCount = 0;

    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(buffer);
    AndamBusFrameHeader hdr;

    try {
        sendCommandWithResponse(cmd, slave->getAddress(), resp, hdr);
        AB_DEBUG("refreshed secondary buses " << slave->getAddress() << " item count:" << (int)resp.typeOkData.itemCount);
        slave->updateSecondaryBuses(resp.typeOkData.itemCount, resp.typeOkData.responseData.busItems);
    } catch (CommandException &e) {
        AB_WARNING("refreshSecondaryBuses:" << e.what());
    }
}

size_t AndamBusMaster::calculateFrameSize(AndamBusCommand cmd, uint8_t propCount) {
    // in case of command type RESTORE_CONFIG propertyCount contains payload size

    if (cmd == AndamBusCommand::RESTORE_CONFIG)
        return offsetof(AndamBusFrameCommand, data) + propCount;

    return offsetof(AndamBusFrameCommand, props) + sizeof(ItemProperty)*propCount;
}

void AndamBusMaster::sendCommandWithResponse(AndamBusFrameCommand &cmd, uint16_t addr, AndamBusFrameResponse &resp, AndamBusFrameHeader &hdr) {
    auto start = chrono::system_clock::now();

//    AB_INFO("before mutex");
    lock_guard<mutex> lg(mtTxn);
//    AB_INFO("after mutex");

//    try {
        AndamBusFrame frm = {};
        frm.command = cmd;

        size_t size = calculateFrameSize(cmd.command, cmd.propertyCount);
        htonCmd(frm.command);

        AndamBusSlave *slave = findSlaveByAddress(addr);

        uint16_t counter = 0;
        if (slave != nullptr)
            counter = slave->getNextCounter();

  //      AB_INFO("before send frame");
        bsock.sendFrame(frm, size, addr, counter);
  //      AB_INFO("after send frame");
        cntSent++;

        char rbuf[ANDAMBUS_BUFFER_SIZE+100];
        AndamBusFrame &rfrm = *(AndamBusFrame*)rbuf;

    //    AB_INFO("before getresponse");
        getResponse(cmd.command, rfrm);
   //     AB_INFO("after getresponse");

        auto dur = chrono::system_clock::now()-start;
        AB_DEBUG("command duration " << chrono::duration_cast<chrono::milliseconds>(dur).count() << "ms");

        AB_DEBUG("memcpy payload len=" << dec << rfrm.header.payloadLength);
        memcpy(&resp, &rfrm.response, rfrm.header.payloadLength);

        if (rfrm.header.counter != counter) {
            AB_WARNING("Response counter does not match " << rfrm.header.counter << " vs " <<counter);
            return;
        }

        ntohResp(resp);

        hdr = rfrm.header;
//    AB_INFO("end cmd response");
}

void AndamBusMaster::getResponse(AndamBusCommand cmd, AndamBusFrame &resp) {
    AB_DEBUG("getResponse");
    try {
        bsock.receiveFrame(resp, 0, ANDAMBUS_BUFFER_SIZE);
  //      AB_INFO("frameReceived " << resp.header.payloadLength);

        cntReceived++;

        checkResponse(cmd, resp.response);
    } catch (ExceptionBase &e) {
        SynchronizationLostException *sle = dynamic_cast<SynchronizationLostException*>(&e);

        if (sle != nullptr)
            cntSyncLost++;

        BusTimeoutException *bte = dynamic_cast<BusTimeoutException*>(&e);
        if (bte != nullptr)
            cntBusTimeout++;

        InvalidFrameException *ife = dynamic_cast<InvalidFrameException*>(&e);
        if (ife != nullptr)
            cntInvalidFrm++;

        AB_INFO("exception " << e.what());
        throw;
    }
}


void AndamBusMaster::checkResponse(AndamBusCommand cmd, AndamBusFrameResponse &resp) {

    if (resp.responseType == AndamBusResponseType::ERROR) {
        cntCmdError++;
        throw CommandException(resp.typeError.errorCode, cmd);
    }

    ResponseContent cont = ResponseContent::NONE;
    AndamBusResponseType rtp = AndamBusResponseType::OK;

    switch (cmd) {
        case AndamBusCommand::NONE:
        case AndamBusCommand::GET_DATA:
        case AndamBusCommand::SET_DATA:
        case AndamBusCommand::PERSIST:
        case AndamBusCommand::RESTORE_CONFIG:
        case AndamBusCommand::RESET:
        case AndamBusCommand::SECONDARY_BUS_DETECT:
        case AndamBusCommand::SECONDARY_BUS_REMOVE:
        case AndamBusCommand::SECONDARY_BUS_COMMAND:
        case AndamBusCommand::VIRTUAL_DEVICE_REMOVE:
        case AndamBusCommand::VIRTUAL_DEVICE_CHANGE:
        case AndamBusCommand::VIRTUAL_PORT_REMOVE:
        case AndamBusCommand::VIRTUAL_PORT_SET_VALUE:
        case AndamBusCommand::SET_PROPERTY:
            rtp = AndamBusResponseType::OK;
            break;
        case AndamBusCommand::GET_CONFIG:
            rtp = AndamBusResponseType::OK_DATA;
            cont = ResponseContent::RAWDATA;
            break;
        case AndamBusCommand::METADATA:
            rtp = AndamBusResponseType::OK_DATA;
            cont = ResponseContent::METADATA;
            break;
        case AndamBusCommand::SECONDARY_BUS_CREATE:
        case AndamBusCommand::SECONDARY_BUS_LIST:
            rtp = AndamBusResponseType::OK_DATA;
            cont = ResponseContent::SECONDARY_BUS;
            break;
        case AndamBusCommand::DETECT_SLAVE:
        case AndamBusCommand::PROPERTY_LIST:
            rtp = AndamBusResponseType::OK_DATA;
            cont = ResponseContent::PROPERTY;
            break;
        case AndamBusCommand::VIRTUAL_DEVICE_CREATE:
        case AndamBusCommand::VIRTUAL_ITEMS_CREATED:
            rtp = AndamBusResponseType::OK_DATA;
            cont = ResponseContent::MIXED;
            break;
        case AndamBusCommand::VIRTUAL_DEVICE_LIST:
            rtp = AndamBusResponseType::OK_DATA;
            cont = ResponseContent::VIRTUAL_DEVICE;
            break;
        case AndamBusCommand::VIRTUAL_PORT_CREATE:
        case AndamBusCommand::VIRTUAL_PORT_LIST:
            rtp = AndamBusResponseType::OK_DATA;
            cont = ResponseContent::VIRTUAL_PORT;
            break;
        case AndamBusCommand::VIRTUAL_ITEMS_REMOVED:
        case AndamBusCommand::VALUES_CHANGED:
            rtp = AndamBusResponseType::OK_DATA;
            cont = ResponseContent::VALUES;
            break;
    }

    if (resp.responseType != rtp) {
        stringstream ss;
        ss << "Response type not as expected " << responseTypeString(rtp) << " vs " << responseTypeString(resp.responseType) << " for " << commandString(cmd);
        throw InvalidFrameException(ss.str());
    }

    if (resp.responseType == AndamBusResponseType::OK_DATA && resp.typeOkData.content != cont) {
        stringstream ss;
        ss << "Response content type not as expected " << responseContentString(cont) << " vs " << responseContentString(resp.typeOkData.content) << " for " << commandString(cmd);
        throw InvalidFrameException(ss.str());
    }
}


void AndamBusMaster::refreshPortValues(bool full) {
    for (auto it=slaves.begin();it!=slaves.end();it++)
        refreshSlavePortValues(*it, full);
}


void AndamBusMaster::removeSlave(uint16_t addr) {
    for(auto it=slaves.begin();it!=slaves.end();it++) {
        AndamBusSlave *slave = *it;
        if (slave->getAddress() == addr) {
            slaves.erase(it);
            return;
        }
    }
}

SlaveVirtualDevice* AndamBusMaster::getVirtualDeviceByLongAddress(uint64_t longaddr) {
    for(auto it=slaves.begin();it!=slaves.end();it++) {
        AndamBusSlave *slave = *it;
        SlaveVirtualDevice *vd = slave->getVirtualDeviceByLongAddress(longaddr);

        if (vd != nullptr)
            return vd;
    }
    return nullptr;
}

SlaveVirtualDevice* AndamBusMaster::getVirtualDeviceByPermanentID(uint64_t permId) {
    if (permId < 0x100)
        return nullptr;
    if (permId >= 0x100 && permId < ANDAMBUS_UNITS_LIMIT) {
        uint8_t unitId = permId /0x100;
        uint8_t pin = permId %0x100;

        AndamBusSlave *unit = findSlaveByAddress(unitId);
        if (unit == nullptr)
            return nullptr;

        return unit->getVirtualDeviceByPin(pin);
    }

    return getVirtualDeviceByLongAddress(permId);
}
