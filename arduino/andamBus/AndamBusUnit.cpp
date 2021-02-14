#include "AndamBusUnit.h"
#include "util.h"

using namespace std;

char unitBuffer[UNIT_BUFFER_SIZE];

AndamBusUnit::AndamBusUnit(BroadcastSocket &_bs, uint16_t _address):bs(_bs),address(_address),
    counter(0),lastPortRefresh(0),lastItemRefresh(0)
{
}

AndamBusUnit::~AndamBusUnit()
{
    //dtor
}

void AndamBusUnit::doWork() {
    if (!bs.isSynchronized())
        bs.trySync();

    if (!bs.isSynchronized()) {
        DEBUG("not synchronized");
        return;
    }

    AndamBusFrame fcmd;

#ifndef ARDUINO
    try {
#else
        int ret =
#endif // ARDUINO
            bs.receiveFrame(fcmd, address, UNIT_BUFFER_SIZE);

#ifdef ARDUINO
      if (ret == 2)
        LOG_U(F("Sync lost"));

      if (ret == 5)
        responseError(AndamBusCommandError::CRC_MISMATCH);

      if (ret == 7)
        responseError(AndamBusCommandError::INVALID_API_VERSION);

      if (ret != 0) {
            return;
        }
#endif // ARDUINO

        AndamBusFrameCommand &cmd = fcmd.command;
        counter = fcmd.header.counter;

        ntohCmd(cmd);
        processCommand(cmd);
#ifndef ARDUINO
    } catch (DifferentTargetException &e) {
        DEBUG("Different address " << e.getFrameAddress() << " this is " << e.getTargetAddress());
    } catch (SynchronizationLostException &e) {
        WARNING("SynchronizationLostException:" << e.what());
        bs.trySync();
    } catch (BusTimeoutException &e) {
        DEBUG("DoWork.BusTimeoutException:" << e.what());
    }
#endif // ARDUINO
}

void AndamBusUnit::processCommand(AndamBusFrameCommand &cmd) {
    LOG_U("Command: 0x" << iom::hex << (int)cmd.command << " " << (int)AndamBusCommand::VIRTUAL_ITEMS_CREATED);

    int32_t value;
    VirtualPortType portType;
    VirtualDeviceType devType;
    OnewireBusCommand busCmd;
    AndamBusCommandError ret;

    const ItemProperty *prop;
#warning Validate command:parameter bound checking

    if (!isInitialized() && cmd.command != AndamBusCommand::DETECT_SLAVE &&
            cmd.command != AndamBusCommand::SECONDARY_BUS_LIST &&
            cmd.command != AndamBusCommand::METADATA &&
            cmd.command != AndamBusCommand::GET_CONFIG) {
        responseError(AndamBusCommandError::NOT_REFRESHED);
        return;
        }

    switch (cmd.command) {
    case AndamBusCommand::DETECT_SLAVE:
        responseDetect();
        return;
    case AndamBusCommand::PERSIST:
        doPersist();
        responseOk();
        return;
    case AndamBusCommand::RESET:
        responseOk();
        softwareReset();
        return;
    case AndamBusCommand::SECONDARY_BUS_LIST:
        responseBusList();
        return;
    case AndamBusCommand::SECONDARY_BUS_CREATE:
        if (cmd.pin == 0)
            responseError(AndamBusCommandError::MISSING_ARGUMENT);
        else
            responseBusCreate(cmd.pin);
        return;
    case AndamBusCommand::SECONDARY_BUS_DETECT:
        if (checkBusActive(cmd.id))
            responseOk();
        else
            responseError(AndamBusCommandError::ITEM_DOES_NOT_EXIST);
        secondaryBusDetect(cmd.id);
        return;
    case AndamBusCommand::SECONDARY_BUS_COMMAND:
        prop = findPropertyByType(cmd.props, cmd.propertyCount, AndamBusPropertyType::W1CMD);

        if (prop == nullptr) {
            responseError(AndamBusCommandError::MISSING_ARGUMENT);
            return;
        }

        busCmd = prop->cmd;

        responseOk();
        secondaryBusCommand(cmd.id, busCmd);
        return;
    case AndamBusCommand::SECONDARY_BUS_REMOVE:
        if (removeSecondaryBus(cmd.id))
            responseOk();
        else
            responseError(AndamBusCommandError::ITEM_DOES_NOT_EXIST);
        return;
    case AndamBusCommand::VIRTUAL_DEVICE_LIST:
        lastItemRefresh = counter;
        responseDeviceList(cmd.id);
        return;
    case AndamBusCommand::VIRTUAL_DEVICE_CREATE:
        prop = findPropertyByType(cmd.props, cmd.propertyCount, AndamBusPropertyType::DEVICE_TYPE);

        if (prop == nullptr) {
            responseError(AndamBusCommandError::MISSING_ARGUMENT);
//            ERROR("mossing argument");
            return;
        }

        devType = prop->deviceType;

        responseDeviceCreate(cmd.pin, devType);
        return;
    case AndamBusCommand::VIRTUAL_DEVICE_REMOVE:
        if (removeVirtualDevice(cmd.id))
            responseOk();
        else
            responseError(AndamBusCommandError::ITEM_DOES_NOT_EXIST);
        return;
    case AndamBusCommand::VIRTUAL_PORT_LIST:
        responsePortList(cmd.id);
        lastPortRefresh = counter;
        return;
    case AndamBusCommand::VIRTUAL_PORT_CREATE:
//        LOG_U("Create port");
        prop = findPropertyByType(cmd.props, cmd.propertyCount, AndamBusPropertyType::PORT_TYPE);

        if (prop == nullptr) {
            responseError(AndamBusCommandError::MISSING_ARGUMENT);
            return;
        }

        portType = prop->portType;

        responsePortCreate(cmd.pin, portType);
        return;
    case AndamBusCommand::VALUES_CHANGED:
        responseValuesChanged(cmd.pin == 1); // pin = 1 -> full refresh
        lastPortRefresh = counter;
        return;
    case AndamBusCommand::VIRTUAL_PORT_REMOVE:
        if (cmd.id == 0) {
            responseError(AndamBusCommandError::ITEM_DOES_NOT_EXIST);
            return;
        }

        if(removePort(cmd.id))
            responseOk();
        else
            responseError(AndamBusCommandError::NON_REMOVABLE);
        return;
    case AndamBusCommand::VIRTUAL_PORT_SET_VALUE:
        prop = findPropertyByType(cmd.props, cmd.propertyCount, AndamBusPropertyType::PORT_VALUE);

        if (prop == nullptr) {
            responseError(AndamBusCommandError::MISSING_ARGUMENT);
            return;
        }

        value = prop->value;

        ret = setPortValue(cmd.id, value);
        if (ret == AndamBusCommandError::OK)
            responseOk();
        else
            responseError(ret);
        return;
    case AndamBusCommand::GET_CONFIG:
        responseGetConfig();
        return;
    case AndamBusCommand::RESTORE_CONFIG:
        if (cmd.propertyCount == 0)
            responseError(AndamBusCommandError::OTHER_ERROR);
        else
            responseOk();
        restoreConfig((uint8_t*)cmd.data, cmd.propertyCount, cmd.id, cmd.pin); // propertyCount contains the real size of data, cmd.id contains current page, cmd.pin contains finish flag
        return;
    case AndamBusCommand::PROPERTY_LIST:
        if (cmd.id == 0)
            responsePropertyList();
        else
            responsePropertyList(cmd.id);
        return;
    case AndamBusCommand::SET_PROPERTY:
        ret = setPropertyList(cmd.props, cmd.propertyCount);
        if (ret == AndamBusCommandError::OK)
            responseOk();
        else
            responseError(ret);
        return;
    case AndamBusCommand::SET_DATA:
        ret = setData(cmd.id, cmd.data, cmd.propertyCount);
        if (ret == AndamBusCommandError::OK)
            responseOk();
        else
            responseError(ret);
        return;
    case AndamBusCommand::VIRTUAL_ITEMS_CREATED:
        responseNewItems();
        lastItemRefresh = counter;
        return;
    case AndamBusCommand::METADATA:
        responseMetadata();
        return;
    case AndamBusCommand::NONE:
        LOG_U(F("Command NONE received"));
        ERROR("NONE received");
        return;
    case AndamBusCommand::GET_DATA:
    case AndamBusCommand::VIRTUAL_DEVICE_CHANGE:
    case AndamBusCommand::VIRTUAL_ITEMS_REMOVED:
        ERROR("Command " << commandString(cmd.command) << " not implemented");
#ifdef ARDUINO
		const __FlashStringHelper* cs = commandString(cmd.command);
		if (cs != nullptr) {
			ERROR(F("Command ") << cs << F(" not implemented"));
		}
		else {
			ERROR(F("Command ") << (int)cmd.command << F(" not implemented"));
		}
#endif
		responseError(AndamBusCommandError::NOT_IMPLEMENTED);
        return;
    }

    ERROR("Command x " << (int)(cmd.command) << " not implemented");
    responseError(AndamBusCommandError::NOT_IMPLEMENTED);
}

void AndamBusUnit::responseError(AndamBusCommandError code) {
    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(unitBuffer);
    resp.responseType = AndamBusResponseType::ERROR;
    resp.typeError.errorCode = code;

    startResponse(resp);
    bs.frameData(sizeof(AndamBusCommandError), reinterpret_cast<const unsigned char*>(&code));

    finishResponse();
}

void AndamBusUnit::responseOk() {
    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(unitBuffer);

    resp.responseType = AndamBusResponseType::OK;

    startResponse(resp);

    finishResponse();
//    LOG_U(millis());
    }

void AndamBusUnit::responseGetConfig() {
//    ERROR("responseGetConfig");
    uint16_t pagesize = (UNIT_BUFFER_SIZE/2);
    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(unitBuffer);
    resp.responseType = AndamBusResponseType::OK_DATA;
    resp.typeOkData.content = ResponseContent::RAWDATA;

    unsigned char eeData[pagesize];
    uint16_t cfgSize = getConfigSize();
//    ERROR("cfgSize=" << (int)cfgSize);

    if (cfgSize == 0) {
        responseError(AndamBusCommandError::OTHER_ERROR);
        return;
    }


    uint8_t cnt = (cfgSize-1)/pagesize+1;
    for (int i=0;i<cnt;i++) {
        uint8_t realsize = i==cnt-1?((cfgSize-1)%pagesize)+1:pagesize;
        readConfigPart(eeData, realsize, i);

        if (i==0) {
            resp.typeOkData.itemCount = cfgSize/0x100;
            resp.typeOkData.itemCount2 = cfgSize%0x100;
            startResponse(resp);
        }

//        ERROR("sendRawData " << (i==cnt-1) << " " << cfgSize);
        sendRawData(eeData, realsize);
    }

//    ERROR("finish");
    finishResponse();

}

void AndamBusUnit::responsePortCreate(uint8_t pin, VirtualPortType type) {
    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(unitBuffer);

    resp.responseType = AndamBusResponseType::OK_DATA;
    resp.typeOkData.content = ResponseContent::VIRTUAL_PORT;

    ItemProperty pr[ANDAMBUS_MAX_ITEM_COUNT];
    uint8_t propCnt;

    VirtualPort port;
    AndamBusCommandError ret = createPort(pin, type, port, pr, propCnt);
    if (ret == AndamBusCommandError::OK) {
        resp.typeOkData.itemCount = 1;
        startResponse(resp);
        sendVirtualPort(port);
        finishResponse();
    } else {
        responseError(ret);
    }
}

void AndamBusUnit::responseBusCreate(uint8_t pin) {
    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(unitBuffer);

    resp.responseType = AndamBusResponseType::OK_DATA;
    resp.typeOkData.content = ResponseContent::SECONDARY_BUS;

    SecondaryBus sb;
    AndamBusCommandError ret = createSecondaryBus(sb, pin);

    if (ret == AndamBusCommandError::OK) {
        resp.typeOkData.itemCount = 1;
        startResponse(resp);
        sendSecondaryBus(sb);
        finishResponse();
    } else {
        responseError(ret);
    }
}

void AndamBusUnit::responseDeviceCreate(uint8_t pin, VirtualDeviceType type) {
    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(unitBuffer);

    resp.responseType = AndamBusResponseType::OK_DATA;
    resp.typeOkData.content = ResponseContent::MIXED;

    VirtualDevice vd;
    VirtualPort vp[ANDAMBUS_MAX_DEV_PORTS];
    ItemProperty pr[ANDAMBUS_MAX_ITEM_COUNT];

    uint8_t portCnt, propCnt;

    AndamBusCommandError ret = createDevice(pin, type, vd, vp, portCnt, pr, propCnt);

    if (ret == AndamBusCommandError::OK) {
      resp.typeOkData.responseData.deviceItems[0] = vd;

      resp.typeOkData.itemCount = 1;
      resp.typeOkData.itemCount2 = portCnt;
      resp.typeOkData.itemCount3 = propCnt;

//      VirtualPort *vports = reinterpret_cast<VirtualPort*>(resp.typeOkData.responseData.deviceItems+1);

//      memcpy(vports, vp, portCnt*sizeof(VirtualPort));

//      ItemProperty *props = reinterpret_cast<ItemProperty*>(vports+portCnt);

//      memcpy(props, pr, propCnt*sizeof(ItemProperty));

      resp.typeOkData.itemCount = 1;
      startResponse(resp);
      sendVirtualDevice(vd);
      for (int i=0;i<portCnt;i++)
        sendVirtualPort(vp[i]);
      for (int i=0;i<propCnt;i++)
        sendProperty(pr[i]);

      finishResponse();
    } else {
        responseError(ret);
    }
}

void AndamBusUnit::responsePortList(uint8_t devId) {
    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(unitBuffer);

    resp.responseType = AndamBusResponseType::OK_DATA;
    resp.typeOkData.content = ResponseContent::VIRTUAL_PORT;

    VirtualPort ports[ANDAMBUS_MAX_ITEM_COUNT];

    uint8_t cnt;
    if (devId == 0)
        cnt = getVirtualPortList(ports);
    else
        cnt = getVirtualPortsOnDevice(ports, devId);

    resp.typeOkData.itemCount = cnt;

    startResponse(resp);
    for (int i=0;i<cnt;i++)
        sendVirtualPort(ports[i]);
    finishResponse();
}

void AndamBusUnit::responseValuesChanged(bool full) {
    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(unitBuffer);

    resp.responseType = AndamBusResponseType::OK_DATA;
    resp.typeOkData.content = ResponseContent::VALUES;

    ItemValue values[ANDAMBUS_MAX_ITEM_COUNT];
    uint8_t cnt = getChangedValues(values, full);

    resp.typeOkData.itemCount = cnt;

    startResponse(resp);
    for (int i=0;i<cnt;i++)
        sendItemValue(values[i]);
    finishResponse();
}

void AndamBusUnit::responseDeviceList(uint8_t busId) {
    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(unitBuffer);

    resp.responseType = AndamBusResponseType::OK_DATA;
    resp.typeOkData.content = ResponseContent::VIRTUAL_DEVICE;

    uint8_t cnt;

    VirtualDevice devs[ANDAMBUS_MAX_BUS_DEVS];
    if (busId == 0)
        cnt = getVirtualDeviceList(devs);
    else
        cnt = getVirtualDevicesOnBus(devs, busId);

    resp.typeOkData.itemCount = cnt;

    startResponse(resp);
    for (int i=0;i<cnt;i++)
        sendVirtualDevice(devs[i]);

    finishResponse();
}

void AndamBusUnit::responseBusList() {
    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(unitBuffer);

    resp.responseType = AndamBusResponseType::OK_DATA;
    resp.typeOkData.content = ResponseContent::SECONDARY_BUS;

    SecondaryBus sb[ANDAMBUS_MAX_SECONDARY_BUSES];
    uint8_t cnt = getSecondaryBusList(sb);

    resp.typeOkData.itemCount = cnt;

    startResponse(resp);

    for (int i=0;i<cnt;i++)
        sendSecondaryBus(sb[i]);

    finishResponse();
}

void AndamBusUnit::responseDetect() {
    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(unitBuffer);

    resp.responseType = AndamBusResponseType::OK_DATA;
    resp.typeOkData.content = ResponseContent::PROPERTY;
    resp.typeOkData.itemCount = 2;

/*    ItemProperty *props = resp.typeOkData.responseData.props;
    props[0].type = AndamBusPropertyType::SLAVE_HW_TYPE;
    props[0].hwType = getHwType();
    props[1].type = AndamBusPropertyType::SW_VERSION;
    props[1].value = getSwVersionMajor()*0x100 + getSwVersionMinor();*/

    ItemProperty prop = {};

    startResponse(resp);

    prop.type = AndamBusPropertyType::SLAVE_HW_TYPE;
    prop.hwType = getHwType();
    sendProperty(prop);

    prop.type = AndamBusPropertyType::SW_VERSION;
    prop.value = getSwVersionMajor()*0x100 + getSwVersionMinor();
    sendProperty(prop);

    finishResponse();
}

void AndamBusUnit::responseNewItems() {
    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(unitBuffer);

    resp.responseType = AndamBusResponseType::OK_DATA;
    resp.typeOkData.content = ResponseContent::MIXED;

    VirtualDevice devs[ANDAMBUS_MAX_ITEM_COUNT];
    resp.typeOkData.itemCount = getNewDevices(devs);

    VirtualPort ports[ANDAMBUS_MAX_DEV_PORTS]; // = reinterpret_cast<VirtualPort*>(resp.typeOkData.responseData.deviceItems+resp.typeOkData.itemCount);
    resp.typeOkData.itemCount2 = getNewPorts(ports);

    ItemProperty props[ANDAMBUS_MAX_ITEM_COUNT]; // = reinterpret_cast<ItemProperty*>(ports+resp.typeOkData.itemCount);
    resp.typeOkData.itemCount3 = getNewProperties(props, devs, resp.typeOkData.itemCount, ports, resp.typeOkData.itemCount2);

    startResponse(resp);
    for (int i=0;i<resp.typeOkData.itemCount;i++)
        sendVirtualDevice(devs[i]);
    for (int i=0;i<resp.typeOkData.itemCount2;i++)
        sendVirtualPort(ports[i]);
    for (int i=0;i<resp.typeOkData.itemCount3;i++)
        sendProperty(props[i]);

    finishResponse();
}

void AndamBusUnit::responseMetadata() {
    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(unitBuffer);

    resp.responseType = AndamBusResponseType::OK_DATA;
    resp.typeOkData.content = ResponseContent::METADATA;

    UnitMetadata mdl[ANDAMBUS_MAX_ITEM_COUNT];
    uint8_t cnt = getMetadataList(mdl, ANDAMBUS_MAX_ITEM_COUNT);

    resp.typeOkData.itemCount = cnt;

    startResponse(resp);
    for (int i=0;i<cnt;i++)
        sendMetadata(mdl[i]);
    finishResponse();
}

void AndamBusUnit::responsePropertyList() {
    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(unitBuffer);

    resp.responseType = AndamBusResponseType::OK_DATA;
    resp.typeOkData.content = ResponseContent::PROPERTY;

    ItemProperty props[ANDAMBUS_MAX_ITEM_COUNT];
    uint8_t cnt = getPropertyList(props, ANDAMBUS_MAX_ITEM_COUNT);

    resp.typeOkData.itemCount = cnt;

    startResponse(resp);
    for (int i=0;i<cnt;i++)
        sendProperty(props[i]);
    finishResponse();
}

void AndamBusUnit::responsePropertyList(uint8_t id) {
    AndamBusFrameResponse &resp = reinterpret_cast<AndamBusFrameResponse&>(unitBuffer);

    resp.responseType = AndamBusResponseType::OK_DATA;
    resp.typeOkData.content = ResponseContent::PROPERTY;

    ItemProperty props[ANDAMBUS_MAX_ITEM_COUNT];
    uint8_t cnt = getPropertyList(props, ANDAMBUS_MAX_ITEM_COUNT, id);

    resp.typeOkData.itemCount = cnt;

    startResponse(resp);
    for (int i=0;i<cnt;i++)
        sendProperty(props[i]);
    finishResponse();
}

void AndamBusUnit::finishResponse()
{
    bs.finishFrame();
}

void AndamBusUnit::sendRawData(unsigned const char *data, uint8_t size)
{
//    ERROR("sendRawData " << (int)size);
    //hexdump((const char*)data, size);
    bs.frameData(size, data);
}

void AndamBusUnit::sendItemValue(ItemValue &val)
{
    val.value = htonl(val.value);
    bs.frameData(sizeof(ItemValue), reinterpret_cast<const unsigned char*>(&val));
}

void AndamBusUnit::sendVirtualPort(VirtualPort &port)
{
    port.value = htonl(port.value);
    bs.frameData(sizeof(VirtualPort), reinterpret_cast<const unsigned char*>(&port));
}

void AndamBusUnit::sendVirtualDevice(VirtualDevice &dev)
{
    bs.frameData(sizeof(VirtualDevice), reinterpret_cast<const unsigned char*>(&dev));
}


void AndamBusUnit::sendSecondaryBus(SecondaryBus &bus)
{
    bs.frameData(sizeof(SecondaryBus), reinterpret_cast<const unsigned char*>(&bus));
}

void AndamBusUnit::sendProperty(ItemProperty &prop)
{
    htonProp(prop);

//    prop.value = htonl(prop.value);
//    prop.type = static_cast<AndamBusPropertyType>(htons(static_cast<uint16_t>(prop.type)));
    bs.frameData(sizeof(ItemProperty), reinterpret_cast<const unsigned char*>(&prop));
//    ERROR("sending property " << sizeof(ItemProperty));
}

void AndamBusUnit::sendMetadata(UnitMetadata &md)
{
    md.value = htonl(md.value);
    md.type = static_cast<MetadataType>(htons(static_cast<uint16_t>(md.type)));
    md.propertyId = htons(md.propertyId);
    bs.frameData(sizeof(UnitMetadata), reinterpret_cast<const unsigned char*>(&md));
//    ERROR("sending property " << sizeof(ItemProperty));
}

void AndamBusUnit::startResponse(AndamBusFrameResponse &resp) {
    uint32_t size = 0,
        respHdrSize = 0;

    if (resp.responseType == AndamBusResponseType::OK) {
        size = sizeof(AndamBusResponseType);
        respHdrSize = sizeof(AndamBusResponseType);
    }

    if (resp.responseType == AndamBusResponseType::OK_DATA)
        respHdrSize = offsetof(AndamBusFrameResponse, typeOkData.responseData.props);

    if (resp.responseType == AndamBusResponseType::OK_DATA && resp.typeOkData.content != ResponseContent::MIXED) {
        uint8_t itemSize = getItemSize(resp.typeOkData.content);
        size = offsetof(AndamBusFrameResponse, typeOkData.responseData.props) + resp.typeOkData.itemCount * itemSize;
    }

    if (resp.responseType == AndamBusResponseType::OK_DATA && resp.typeOkData.content == ResponseContent::RAWDATA)
        size = offsetof(AndamBusFrameResponse, typeOkData.responseData.props) + resp.typeOkData.itemCount * 0x100 + resp.typeOkData.itemCount2;

    if (resp.responseType == AndamBusResponseType::OK_DATA && resp.typeOkData.content == ResponseContent::MIXED) {
        size_t s = resp.typeOkData.itemCount*sizeof(VirtualDevice)
            + resp.typeOkData.itemCount2*sizeof(VirtualPort)
            + resp.typeOkData.itemCount3*sizeof(ItemProperty);
        size = offsetof(AndamBusFrameResponse, typeOkData.responseData.props) + s;
    }

    if (resp.responseType == AndamBusResponseType::ERROR) {
        size = offsetof(AndamBusFrameResponse, typeError.errorCode) + sizeof(AndamBusCommandError);
        respHdrSize = offsetof(AndamBusFrameResponse, typeError.errorCode);
    }

//    AndamBusFrame frm = {};
//    htonResp(resp);
//    frm.response = resp;

//    memcpy(&fresp.response, &resp, size);

//    ERROR("startresponse sending " << sizeof(AndamBusFrameHeader) << " + " << size);
    bs.startFrame(size, 0, counter);

    if (respHdrSize > 0) {
//        ERROR("continue sending " << respHdrSize);
        bs.frameData(respHdrSize, reinterpret_cast<const unsigned char*>(&resp));
    }
}

uint8_t AndamBusUnit::getItemSize(ResponseContent rc) {
    switch (rc) {
    case ResponseContent::PROPERTY:
        return sizeof(ItemProperty);
    case ResponseContent::METADATA:
        return sizeof(UnitMetadata);
    case ResponseContent::NONE:
    case ResponseContent::MIXED:
        return 0;
    case ResponseContent::RAWDATA:
        return 1;
    case ResponseContent::SECONDARY_BUS:
        return sizeof(SecondaryBus);
    case ResponseContent::VIRTUAL_DEVICE:
        return sizeof(VirtualDevice);
    case ResponseContent::VIRTUAL_PORT:
        return sizeof(VirtualPort);
    case ResponseContent::VALUES:
        return sizeof(ItemValue);
    }
    return 0;
}
