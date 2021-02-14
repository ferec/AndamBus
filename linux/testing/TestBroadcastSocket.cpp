#include "TestBroadcastSocket.h"

#include <iostream>
#include <arpa/inet.h>
#include <string.h>
#include <zlib.h>

#include "shared/AndamBusExceptions.h"
#include "shared/AndamUtils.h"

#include "domain/SlaveVirtualPort.h"
#include "domain/SlaveVirtualDevice.h"

using namespace std;

int TestBroadcastSocket::ID = 40;

TestBroadcastSocket::TestBroadcastSocket():BroadcastSocket(),bufPos(0), bufSize(0),portValue{5,6,7,8},portIdMax(4)
{
    //ctor
}

TestBroadcastSocket::~TestBroadcastSocket()
{
    //dtor
}

void TestBroadcastSocket::setHeader(AndamBusFrame &frm) {
    size_t unit_size=0;
    uint16_t cnt = 0;

    AndamBusFrameResponse &resp = frm.response;

    if (resp.responseType == AndamBusResponseType::OK_DATA) {
        cnt=resp.typeOkData.itemCount;
        switch(resp.typeOkData.content) {
            case ResponseContent::NONE:
                unit_size = 0;
                break;
            case ResponseContent::SECONDARY_BUS:
                unit_size = sizeof(SecondaryBus);
                break;
            case ResponseContent::VIRTUAL_DEVICE:
                unit_size = sizeof(VirtualDevice);
                break;
            case ResponseContent::VIRTUAL_PORT:
                unit_size = sizeof(VirtualPort);
                break;
            case ResponseContent::PROPERTY:
                unit_size = sizeof(ItemProperty);
                break;
        }
    }
    setHeader(frm, offsetof(AndamBusFrameResponse, typeOkData.props) - offsetof(AndamBusFrameResponse, responseType) + unit_size*cnt);
}

void TestBroadcastSocket::setHeader(AndamBusFrame &frm, size_t payload_size) {
    frm.header.magicWord = ANDAMBUS_MAGIC_WORD;
    frm.header.payloadLength = payload_size;

    frm.header.apiVersion.major = ANDAMBUS_API_VERSION_MAJOR;
    frm.header.apiVersion.minor = ANDAMBUS_API_VERSION_MINOR;

//    return len;
}

void TestBroadcastSocket::setResponse(AndamBusFrame &frm) {
    AndamBusFrameResponse &resp = frm.response;
    if (frm.header.magicWord == 0) {
        bufSize = 0;
        return;
    }

    size_t framelen = frm.header.payloadLength + sizeof(AndamBusFrameHeader);
    frm.header.crc32 = 0;

//    size_t len = resp.header.payloadLength;
    AndamUtils::htonResp(frm);

    frm.header.crc32 = htonl(crc32(0, reinterpret_cast<unsigned char*>(&resp), framelen));

    memcpy(ibuffer, reinterpret_cast<unsigned char*>(&resp), framelen);
    bufSize = framelen;

    DEBUG("bufSize=" << bufSize);
}

void TestBroadcastSocket::testResponseOk(AndamBusFrame &frmcmd, AndamBusFrame &frmresp) {
    AndamBusFrameCommand &cmd = frmcmd.command;
    AndamBusFrameResponse &resp = frmresp.response;

    resp.responseType = AndamBusResponseType::OK;
    resp.typeOkData.content = ResponseContent::NONE;
    setHeader(frmresp, sizeof(AndamBusResponseType));

    INFO("creating Ok response for command " << CommandException::commandString(cmd.command));
}

void TestBroadcastSocket::testResponseSlavePresent(AndamBusFrame &frm) {
    AndamBusFrameResponse &resp = frm.response;
    resp.responseType = AndamBusResponseType::OK_DATA;
    resp.typeOkData.content = ResponseContent::PROPERTY;

    resp.typeOkData.itemCount = 2;
    ItemProperty *props = resp.typeOkData.props;
    props[0].type = AndamBusPropertyType::SLAVE_HW_TYPE;
    props[0].hwType = SlaveHwType::TEST;
    props[1].type = AndamBusPropertyType::SW_VERSION;
    props[1].value = TESTSLAVE_SW_VERSION_MAJOR*0x100+TESTSLAVE_SW_VERSION_MINOR;

    setHeaderResponse(frm);
}

void TestBroadcastSocket::testResponseError(AndamBusFrame &frm) {
    AndamBusFrameResponse &resp = frm.response;
//    AndamBusFrameResponse resp={};
    resp.responseType = AndamBusResponseType::ERROR;
    resp.typeError.errorCode = AndamBusCommandError::NOT_IMPLEMENTED;
    setHeader(frm, offsetof(AndamBusFrameResponse, typeError.errorCode) - offsetof(AndamBusFrameResponse, responseType) + sizeof(resp.typeError.errorCode));
}

void TestBroadcastSocket::testProps(AndamBusFrame &frm) {
    AndamBusFrameResponse &resp = frm.response;
//    AndamBusFrameResponse resp={};
    resp.responseType = AndamBusResponseType::OK_DATA;
    resp.typeOkData.itemCount = 4;

    resp.typeOkData.content = ResponseContent::PROPERTY;

    resp.typeOkData.props[0].entityId = 0;
    resp.typeOkData.props[0].type = AndamBusPropertyType::PORT_TYPE;
    resp.typeOkData.props[0].portType = VirtualPortType::DIGITAL_INPUT;

    resp.typeOkData.props[1].entityId = 66;
    resp.typeOkData.props[1].type = AndamBusPropertyType::PORT_TYPE;
    resp.typeOkData.props[1].portType = VirtualPortType::DIGITAL_OUTPUT;

    resp.typeOkData.props[2].entityId = 27;
    resp.typeOkData.props[2].type = AndamBusPropertyType::PORT_TYPE;
    resp.typeOkData.props[2].portType = VirtualPortType::ANALOG_INPUT;

    resp.typeOkData.props[3].entityId = 1;
    resp.typeOkData.props[3].type = AndamBusPropertyType::PORT_TYPE;
    resp.typeOkData.props[3].portType = VirtualPortType::ANALOG_OUTPUT_PWM;

    size_t datalen = offsetof(AndamBusFrameResponse, typeOkData.props) - offsetof(AndamBusFrameResponse, responseType) + resp.typeOkData.itemCount*sizeof(ItemProperty);
    setHeaderResponse(frm, datalen);
}

void TestBroadcastSocket::testBusList(AndamBusFrameResponse &resp) {
//    AndamBusFrameResponse resp={};
    resp.responseType = AndamBusResponseType::OK_DATA;
    resp.typeOkData.content = ResponseContent::SECONDARY_BUS;

    resp.typeOkData.itemCount = 2;
    resp.typeOkData.busItems[0].id = 66;
    resp.typeOkData.busItems[0].pin = 7;
    resp.typeOkData.busItems[1].id = 67;
    resp.typeOkData.busItems[1].pin = 8;

//        resp.typeOkData;
    size_t datalen = offsetof(AndamBusFrameResponse, typeOkData.busItems) - offsetof(AndamBusFrameResponse, responseType) + resp.typeOkData.itemCount*sizeof(SecondaryBus);
    setHeader(resp, datalen);
}

void TestBroadcastSocket::testDevList(AndamBusFrameResponse &resp) {
//    AndamBusFrameResponse resp={};
    resp.responseType = AndamBusResponseType::OK_DATA;
    resp.typeOkData.content = ResponseContent::VIRTUAL_DEVICE;

    resp.typeOkData.itemCount = 4;
    resp.typeOkData.deviceItems[0].id = 27;
    resp.typeOkData.deviceItems[0].busId = 66;
    resp.typeOkData.deviceItems[0].type = VirtualDeviceType::THERMOMETER;
    resp.typeOkData.deviceItems[0].pin = 7;

    resp.typeOkData.deviceItems[1].id = 28;
    resp.typeOkData.deviceItems[1].busId = 0;
    resp.typeOkData.deviceItems[1].type = VirtualDeviceType::PUSH_DETECTOR;
    resp.typeOkData.deviceItems[1].pin = 8;

    resp.typeOkData.deviceItems[2].id = 29;
    resp.typeOkData.deviceItems[2].busId = 0;
    resp.typeOkData.deviceItems[2].type = VirtualDeviceType::BLINDS_CONTROL;
    resp.typeOkData.deviceItems[2].pin = 9;

    resp.typeOkData.deviceItems[3].id = 30;
    resp.typeOkData.deviceItems[3].busId = 67;
    resp.typeOkData.deviceItems[3].type = VirtualDeviceType::THERMOMETER;
    resp.typeOkData.deviceItems[3].pin = 10;
//        resp.typeOkData;
    size_t datalen = offsetof(AndamBusFrameResponse, typeOkData.deviceItems) - offsetof(AndamBusFrameResponse, responseType) + resp.typeOkData.itemCount*sizeof(VirtualDevice);
    setHeader(resp, datalen);
}

void TestBroadcastSocket::testDeviceCreate(const AndamBusFrameCommand &cmd, AndamBusFrameResponse &resp) {

    const ItemProperty *prop = AndamUtils::findPropertyByType(cmd.props, cmd.propertyCount, AndamBusPropertyType::DEVICE_TYPE);

    const VirtualDeviceType type = prop!=nullptr?prop->deviceType:VirtualDeviceType::NONE;
    INFO("creating virtual device of on pin " << dec << (int)cmd.pin << " of type " << SlaveVirtualDevice::getTypeString(type));

//    AndamBusFrameResponse resp={};
    resp.responseType = AndamBusResponseType::OK_DATA;
    resp.typeOkData.content = ResponseContent::VIRTUAL_DEVICE;

    resp.typeOkData.itemCount = 1;

    resp.typeOkData.deviceItems[0].id = 123;
    resp.typeOkData.deviceItems[0].type = type;
    resp.typeOkData.deviceItems[0].busId = 0;

    size_t datalen = offsetof(AndamBusFrameResponse, typeOkData.deviceItems) - offsetof(AndamBusFrameResponse, responseType) + resp.typeOkData.itemCount*sizeof(VirtualDevice);
    setHeader(resp, datalen);

}

void TestBroadcastSocket::testBusCreate(const AndamBusFrameCommand &cmd, AndamBusFrameResponse &resp) {
    resp.responseType = AndamBusResponseType::OK_DATA;
    resp.typeOkData.content = ResponseContent::SECONDARY_BUS;
    resp.typeOkData.itemCount = 1;

    resp.typeOkData.busItems[0].id = 56;
    resp.typeOkData.busItems[0].pin = cmd.pin;

    size_t datalen = offsetof(AndamBusFrameResponse, typeOkData.portItems) - offsetof(AndamBusFrameResponse, responseType) + resp.typeOkData.itemCount*sizeof(SecondaryBus);
    setHeader(resp, datalen);
}

void TestBroadcastSocket::testPortCreate(const AndamBusFrameCommand &cmd, AndamBusFrameResponse &resp) {
    VirtualPortType tp = VirtualPortType::DIGITAL_INPUT;
    const ItemProperty *prop = AndamUtils::findPropertyByType(cmd.props, cmd.propertyCount, AndamBusPropertyType::PORT_TYPE);

    if (prop != nullptr)
        tp = prop->portType;
//            prop

    INFO("creating virtual port on pin " << dec << (int)cmd.pin << " of type " << SlaveVirtualPort::getTypeString(tp));

//    AndamBusFrameResponse resp={};
    resp.responseType = AndamBusResponseType::OK_DATA;
    resp.typeOkData.content = ResponseContent::VIRTUAL_PORT;
    resp.typeOkData.itemCount = 1;

    resp.typeOkData.portItems[0].id = ID++;
    resp.typeOkData.portItems[0].type = tp;
    resp.typeOkData.portItems[0].value = 0;
    resp.typeOkData.portItems[0].deviceId = 0;

    size_t datalen = offsetof(AndamBusFrameResponse, typeOkData.portItems) - offsetof(AndamBusFrameResponse, responseType) + resp.typeOkData.itemCount*sizeof(VirtualPort);
    setHeader(resp, datalen);
}

void TestBroadcastSocket::testPortList(AndamBusFrameResponse &resp) {
//    AndamBusFrameResponse resp={};
    resp.responseType = AndamBusResponseType::OK_DATA;
    resp.typeOkData.content = ResponseContent::VIRTUAL_PORT;
    resp.typeOkData.itemCount = 4;
    resp.typeOkData.portItems[0].id = 1;
    resp.typeOkData.portItems[0].type = VirtualPortType::DIGITAL_INPUT;
    resp.typeOkData.portItems[0].value = portValue[0];
    resp.typeOkData.portItems[0].deviceId = 28;
    resp.typeOkData.portItems[0].pin = 3;
    resp.typeOkData.portItems[1].id = 2;
    resp.typeOkData.portItems[1].type = VirtualPortType::ANALOG_OUTPUT;
    resp.typeOkData.portItems[1].value = portValue[1];
    resp.typeOkData.portItems[1].deviceId = 27;
    resp.typeOkData.portItems[1].pin = 4;
    resp.typeOkData.portItems[2].id = 3;
    resp.typeOkData.portItems[2].type = VirtualPortType::DIGITAL_INPUT;
    resp.typeOkData.portItems[2].value = portValue[2];
    resp.typeOkData.portItems[2].deviceId = 29;
    resp.typeOkData.portItems[2].pin = 5;
    resp.typeOkData.portItems[3].id = 4;
    resp.typeOkData.portItems[3].type = VirtualPortType::DIGITAL_INPUT;
    resp.typeOkData.portItems[3].value = portValue[3];
    resp.typeOkData.portItems[3].deviceId = 29;
    resp.typeOkData.portItems[3].pin = 6;


//        resp.typeOkData;
    size_t datalen = offsetof(AndamBusFrameResponse, typeOkData.portItems) - offsetof(AndamBusFrameResponse, responseType) + resp.typeOkData.itemCount*sizeof(VirtualPort);
    setHeader(resp, datalen);
}

void TestBroadcastSocket::send(size_t bytes, unsigned const char *buffer) {
    AndamBusFrameCommand *cmd = reinterpret_cast<AndamBusFrameCommand*>(const_cast<unsigned char *>(buffer));
    AndamUtils::ntohCmd(*cmd);

    AndamBusFrameResponse resp = {};

    if (cmd->header.slaveAddress == 5) {
        switch(cmd->command) {
            case AndamBusCommand::BUS_DETECT_SLAVE:
                testResponseSlavePresent(resp);
                break;
            case AndamBusCommand::DEVICE_SECONDARY_BUS_DETECTION:
            case AndamBusCommand::DEVICE_REMOVE_SECONDARY_BUS:
            case AndamBusCommand::DEVICE_VIRTUAL_DEVICE_REMOVE:
            case AndamBusCommand::DEVICE_VIRTUAL_PORT_REMOVE:
            case AndamBusCommand::DEVICE_SECONDARY_BUS_COMMAND:
                testResponseOk(*cmd, resp);
                break;
            case AndamBusCommand::DEVICE_CREATE_SECONDARY_BUS:
                testBusCreate(*cmd, resp);
                break;
            case AndamBusCommand::DEVICE_VIRTUAL_PORT_LIST:
                testPortList(resp);
                break;
            case AndamBusCommand::DEVICE_GET_SECONDARY_BUS_LIST:
                testBusList(resp);
                break;
            case AndamBusCommand::DEVICE_VIRTUAL_DEVICE_LIST:
                testDevList(resp);
                break;
            case AndamBusCommand::DEVICE_PROPERTY_LIST:
                testProps(resp);
                break;
            case AndamBusCommand::DEVICE_VIRTUAL_PORT_GET_CHANGES:
                testPortList(resp);
                break;
            case AndamBusCommand::DEVICE_VIRTUAL_PORT_CREATE:
                testPortCreate(*cmd, resp);
                break;
            case AndamBusCommand::DEVICE_VIRTUAL_DEVICE_CREATE:
                testDeviceCreate(*cmd, resp);
                break;
            case AndamBusCommand::DEVICE_VIRTUAL_PORT_SET_VALUE:
                DEBUG("propcount:" << (int)cmd->propertyCount);
                if (cmd->propertyCount == 1) {
                    INFO(" setting port value to " << (int)cmd->props[0].value << " on " << (int)cmd->id);
                    DEBUG("value:"<<(int)cmd->props[0].value);

                    if (cmd->id >= 1 && cmd->id <= 10) portValue[cmd->id-1] = cmd->props[0].value;
                }

                testResponseOk(*cmd, resp);

                break;
            default:
                ERROR("command not implemented");
                testResponseError(resp);
        }
        setResponse(resp);
    }
}


void TestBroadcastSocket::receive(uint32_t bytes, unsigned char *buffer, unsigned int timeoutMs) {
    if (bufSize == 0)
        throw BusTimeoutException("no slave");

    DEBUG("sending test data at pos=" << bufPos);
    memcpy(buffer, ibuffer+bufPos, bytes);
    bufPos += bytes;

    if (bufPos >= bufSize)
        clean();
}

void TestBroadcastSocket::clean() {
    bufPos = 0;
    bufSize = 0;
}

char TestBroadcastSocket::readByte() {
    return 'a';
}

bool TestBroadcastSocket::available() {
    return false;
}

void TestBroadcastSocket::cleanReadBuffer() {}

void TestBroadcastSocket::startTimer() {
}

bool TestBroadcastSocket::hasTimeoutExpired(uint32_t timeoutMs) {
    return false;
}
