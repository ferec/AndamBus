#include "util.h"
#include "shared/AndamBusStrings.h"
#include <chrono>
#include <iostream>
//#include "AndamBusSlave.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>

using namespace std;


const ItemProperty* findPropertyByType(const ItemProperty props[], uint8_t size, AndamBusPropertyType type) {
    for (int i=0; i<size;i++) {
//        ERROR("findPropertyByType " << (int)props[i].type);
        if (props[i].type == type)
            return &props[i];
    }

    return nullptr;
}

void ntohHdr(AndamBusFrameHeader &hdr) {
    hdr.slaveAddress = ntohs(hdr.slaveAddress);
    hdr.counter = ntohs(hdr.counter);
//    hdr.crc32 = ntohl(hdr.crc32);
    hdr.payloadLength = ntohl(hdr.payloadLength);
    hdr.magicWord = ntohl(hdr.magicWord);
}

void htonHdr(AndamBusFrameHeader &hdr) {
    hdr.slaveAddress = htons(hdr.slaveAddress);
    hdr.counter = htons(hdr.counter);
//    hdr.crc32 = htonl(hdr.crc32);
    hdr.payloadLength = htonl(hdr.payloadLength);
    hdr.magicWord = htonl(hdr.magicWord);
}

void htonResp(AndamBusFrameResponse &resp) {
    if (resp.responseType == AndamBusResponseType::OK_DATA) {
        switch (resp.typeOkData.content) {
            case ResponseContent::NONE:
            case ResponseContent::RAWDATA:
            case ResponseContent::SECONDARY_BUS:
            case ResponseContent::VIRTUAL_DEVICE:
                break;
            case ResponseContent::METADATA:
                for (int i=0;i<resp.typeOkData.itemCount;i++) {
                    resp.typeOkData.responseData.metadata[i].type = static_cast<MetadataType>(htons(static_cast<uint16_t>(resp.typeOkData.responseData.metadata[i].type)));
                    resp.typeOkData.responseData.metadata[i].propertyId = htons(resp.typeOkData.responseData.metadata[i].propertyId);
                    resp.typeOkData.responseData.metadata[i].value = htonl(resp.typeOkData.responseData.metadata[i].value);
                }
                break;
            case ResponseContent::PROPERTY:
                for (int i=0;i<resp.typeOkData.itemCount;i++) {
                    htonProp(resp.typeOkData.responseData.props[i]);
                }
                break;
            case ResponseContent::VIRTUAL_PORT:
                for (int i=0;i<resp.typeOkData.itemCount;i++)
                    resp.typeOkData.responseData.portItems[i].value = htonl(resp.typeOkData.responseData.portItems[i].value);
                break;
            case ResponseContent::VALUES:
                for (int i=0;i<resp.typeOkData.itemCount;i++)
                    resp.typeOkData.responseData.portItems[i].value = htonl(resp.typeOkData.responseData.portValues[i].value);
                break;
            case ResponseContent::MIXED:
                VirtualDevice *vdl = resp.typeOkData.responseData.deviceItems;
                VirtualPort *vpl = reinterpret_cast<VirtualPort*>(vdl+resp.typeOkData.itemCount);

                for (int i=0;i<resp.typeOkData.itemCount2;i++)
                    vpl[i].value = htonl(vpl[i].value);

                ItemProperty *ipl = reinterpret_cast<ItemProperty*>(vpl+resp.typeOkData.itemCount2);

                for (int i=0;i<resp.typeOkData.itemCount3;i++) {
                    htonProp(ipl[i]);
                }
                break;

        }
    }
}

void ntohCmd(AndamBusFrameCommand &cmd) {
    if (cmd.command == AndamBusCommand::RESTORE_CONFIG)
        return;

    for (int i=0;i<cmd.propertyCount;i++) {
        ntohProp(cmd.props[i]);
    }
}


void htonCmd(AndamBusFrameCommand &cmd) {
    if (cmd.command == AndamBusCommand::RESTORE_CONFIG)
        return;

    for (int i=0;i<cmd.propertyCount;i++) {
        htonProp(cmd.props[i]);
    }
}

void ntohResp(AndamBusFrameResponse &resp) {
    if (resp.responseType == AndamBusResponseType::OK_DATA) {
        switch (resp.typeOkData.content) {
            case ResponseContent::NONE:
            case ResponseContent::RAWDATA:
            case ResponseContent::SECONDARY_BUS:
            case ResponseContent::VIRTUAL_DEVICE:
                break;
            case ResponseContent::METADATA:
                for (int i=0;i<resp.typeOkData.itemCount;i++) {
                    resp.typeOkData.responseData.metadata[i].type = static_cast<MetadataType>(ntohs(static_cast<uint16_t>(resp.typeOkData.responseData.metadata[i].type)));
                    resp.typeOkData.responseData.metadata[i].propertyId = ntohs(resp.typeOkData.responseData.metadata[i].propertyId);
                    resp.typeOkData.responseData.metadata[i].value = ntohl(resp.typeOkData.responseData.metadata[i].value);
                }
                break;
            case ResponseContent::PROPERTY:
                for (int i=0;i<resp.typeOkData.itemCount;i++) {
                    ntohProp(resp.typeOkData.responseData.props[i]);
                }
                break;
            case ResponseContent::VIRTUAL_PORT:
                for (int i=0;i<resp.typeOkData.itemCount;i++)
                    resp.typeOkData.responseData.portItems[i].value = ntohl(resp.typeOkData.responseData.portItems[i].value);
                break;
            case ResponseContent::VALUES:
                for (int i=0;i<resp.typeOkData.itemCount;i++)
                    resp.typeOkData.responseData.portValues[i].value = ntohl(resp.typeOkData.responseData.portValues[i].value);
                break;
            case ResponseContent::MIXED:
                VirtualDevice *vdl = resp.typeOkData.responseData.deviceItems;
                VirtualPort *vpl = reinterpret_cast<VirtualPort*>(vdl+resp.typeOkData.itemCount);

                for (int i=0;i<resp.typeOkData.itemCount2;i++)
                    vpl[i].value = ntohl(vpl[i].value);

                ItemProperty *ipl = reinterpret_cast<ItemProperty*>(vpl+resp.typeOkData.itemCount2);

                for (int i=0;i<resp.typeOkData.itemCount3;i++)
                    ntohProp(ipl[i]);

                break;
        }
    }
}

void ntohProp(ItemProperty &prop) {
	prop.value = ntohl(prop.value);
	prop.type = static_cast<AndamBusPropertyType>(ntohs(static_cast<uint16_t>(prop.type)));
}

void htonProp(ItemProperty &prop) {
	prop.value = htonl(prop.value);
	prop.type = static_cast<AndamBusPropertyType>(htons(static_cast<uint16_t>(prop.type)));
}

const char* LogLevelString(LogLevel lvl) {
    switch(lvl) {
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::WARNING: return "WARN ";
        case LogLevel::INFO: return " INFO";
        case LogLevel::DEBUG: return "DEBUG";
        default: return "MESSAGE";
    }
 }


void hexdump(const char *buf, uint16_t size, LogLevel ll) {
    ostringstream ss;

    if (size > 64)
        size  = 64;

    for (size_t i=0;i<size;i++) {
//        cout << hex << (int)buf[i] << " ";
        ss << "0x" << hex << ((int)buf[i]&0xff) << " ";
    }

    AB_LOG(ll, "---------------\n" << ss.str() << "\n---------------");
}

void hexdump(const char *buf, uint16_t size) {
    hexdump(buf, size, LogLevel::DEBUG);
}

string responseTypeString(AndamBusResponseType rtp) {
    switch (rtp) {
        case AndamBusResponseType::OK: return "OK";
        case AndamBusResponseType::OK_DATA: return "OK data";
        case AndamBusResponseType::ERROR: return "ERROR";
    }
    return int_to_string(static_cast<int>(rtp));
}

string responseContentString(ResponseContent cont) {
    switch (cont) {
        case ResponseContent::NONE: return "NONE";
        case ResponseContent::RAWDATA: return "RAWDATA";
        case ResponseContent::SECONDARY_BUS: return "SECONDARY_BUS";
        case ResponseContent::VIRTUAL_DEVICE: return "VIRTUAL_DEVICE";
        case ResponseContent::VIRTUAL_PORT: return "VIRTUAL_PORT";
        case ResponseContent::PROPERTY: return "PROPERTY";
        case ResponseContent::METADATA: return "METADATA";
        case ResponseContent::VALUES: return "VALUES";
        case ResponseContent::MIXED: return "MIXED";
    }

    return int_to_string(static_cast<int>(cont));
}

string commandString(AndamBusCommand cmd) {
    switch (cmd) {
        case AndamBusCommand::NONE: return "NONE";
        case AndamBusCommand::METADATA: return "METADATA";
        case AndamBusCommand::GET_DATA: return "GET_DATA";
        case AndamBusCommand::SET_DATA: return "SET_DATA";
        case AndamBusCommand::PERSIST: return "PERSIST";
        case AndamBusCommand::GET_CONFIG: return "GET_CONFIG";
        case AndamBusCommand::RESTORE_CONFIG: return "RESTORE_CONFIG";
        case AndamBusCommand::RESET: return "RESET";
        case AndamBusCommand::DETECT_SLAVE: return "DETECT_SLAVE";
        case AndamBusCommand::SECONDARY_BUS_CREATE: return "SECONDARY_BUS_CREATE";
        case AndamBusCommand::SECONDARY_BUS_DETECT: return "SECONDARY_BUS_DETECT";
        case AndamBusCommand::SECONDARY_BUS_COMMAND: return "SECONDARY_BUS_COMMAND";
        case AndamBusCommand::SECONDARY_BUS_REMOVE: return "SECONDARY_BUS_REMOVE";
        case AndamBusCommand::SECONDARY_BUS_LIST: return "SECONDARY_BUS_LIST";
        case AndamBusCommand::VALUES_CHANGED: return "VALUES_CHANGED";
        case AndamBusCommand::VIRTUAL_DEVICE_CREATE: return "VIRTUAL_DEVICE_CREATE";
        case AndamBusCommand::VIRTUAL_DEVICE_REMOVE: return "VIRTUAL_DEVICE_REMOVE";
        case AndamBusCommand::VIRTUAL_DEVICE_CHANGE: return "VIRTUAL_DEVICE_CHANGE";
        case AndamBusCommand::VIRTUAL_DEVICE_LIST: return "VIRTUAL_DEVICE_LIST";

        case AndamBusCommand::VIRTUAL_PORT_CREATE: return "VIRTUAL_PORT_CREATE";
        case AndamBusCommand::VIRTUAL_PORT_REMOVE: return "VIRTUAL_PORT_REMOVE";
        case AndamBusCommand::VIRTUAL_PORT_SET_VALUE: return "VIRTUAL_PORT_SET_VALUE";
        case AndamBusCommand::VIRTUAL_PORT_LIST: return "VIRTUAL_PORT_LIST";
        case AndamBusCommand::PROPERTY_LIST: return "PROPERTY_LIST";
        case AndamBusCommand::VIRTUAL_ITEMS_CREATED: return "VIRTUAL_ITEMS_CREATED";
        case AndamBusCommand::VIRTUAL_ITEMS_REMOVED: return "VIRTUAL_ITEMS_REMOVED";
        case AndamBusCommand::SET_PROPERTY: return "SET_PROPERTY";

//            default:
    }
    return int_to_string(static_cast<int>(cmd));
}

const string responseErrorString(AndamBusCommandError err, AndamBusCommand cmd) {
    stringstream ss;
    switch (err) {
        case AndamBusCommandError::OK:
            return MSG_NO_ERROR;
        case AndamBusCommandError::ITEM_DOES_NOT_EXIST:
            return MSG_NOT_EXISTS;
        case AndamBusCommandError::MISSING_ARGUMENT:
            return MSG_ARG_MISSING;
        case AndamBusCommandError::NOT_IMPLEMENTED:
            ss<<MSG_NOT_IMPLEMENTED<<":"<<commandString(cmd);
            return ss.str();
        case AndamBusCommandError::CRC_MISMATCH:
            return MSG_CRC_MISMATCH;
        case AndamBusCommandError::INVALID_API_VERSION:
            return MSG_INVALID_API_VERSION;
        case AndamBusCommandError::PIN_ALREADY_USED:
            return MSG_PIN_ALREADY_USED;
        case AndamBusCommandError::PIN_DIRECTION:
            return MSG_PIN_DIRECTION;
        case AndamBusCommandError::INVALID_PIN:
            return MSG_INVALID_PIN;
        case AndamBusCommandError::ITEM_LIMIT_REACHED:
            return MSG_ITEM_LIMIT_REACHED;
        case AndamBusCommandError::UNKNOWN_DEVICE_TYPE:
            return MSG_UNKNOWN_DEVICE_TYPE;
        case AndamBusCommandError::UNKNOWN_PORT_TYPE:
            return MSG_UNKNOWN_PORT_TYPE;
        case AndamBusCommandError::NOT_REFRESHED:
            return MSG_NOT_REFRESHED;
        case AndamBusCommandError::NON_REMOVABLE:
            return MSG_NON_REMOVABLE;
        case AndamBusCommandError::OTHER_ERROR:
            return MSG_OTHER_ERROR;
    }
    return MSG_UNKNOWN;
}

string virtualDeviceTypeString(VirtualDeviceType type) {
    switch(type) {
        case VirtualDeviceType::THERMOMETER: return "Thermometer";
        case VirtualDeviceType::THERMOSTAT: return "Thermostat";
        case VirtualDeviceType::BLINDS_CONTROL: return "Blinds control";
        case VirtualDeviceType::PUSH_DETECTOR: return "Push detector";
        case VirtualDeviceType::NONE: return "None";
        case VirtualDeviceType::DIFF_THERMOSTAT: return "Differential thermostat";
        case VirtualDeviceType::CIRC_PUMP: return "Water pump";
        case VirtualDeviceType::SOLAR_THERMOSTAT: return "Solar thermostat";
        case VirtualDeviceType::DIGITAL_POTENTIOMETER: return "Digital potentiometer";
        case VirtualDeviceType::I2C_DISPLAY: return "I2C display";
        case VirtualDeviceType::HVAC: return "HVAC";
        case VirtualDeviceType::TIMER: return "Timer";
        case VirtualDeviceType::MODIFIER: return "Modifier";
    }
    return to_string((int)type);
}

string virtualPortTypeString(VirtualPortType type) {
    switch (type) {
        case VirtualPortType::DIGITAL_INPUT: return "Digital input";
        case VirtualPortType::DIGITAL_OUTPUT: return "Digital output";
        case VirtualPortType::DIGITAL_OUTPUT_REVERSED: return "Digital output reversed";
        case VirtualPortType::DIGITAL_INPUT_PULLUP: return "Digital input with pullup";
        case VirtualPortType::ANALOG_INPUT: return "Analog input";
        case VirtualPortType::ANALOG_OUTPUT: return "Analog output";
        case VirtualPortType::ANALOG_OUTPUT_PWM: return "Analog output with PWM";
        case VirtualPortType::NONE: return "None";
    }
    return std::to_string((int)type);
}

string virtualPortTypeStringShort(VirtualPortType type) {
    switch (type) {
        case VirtualPortType::DIGITAL_INPUT: return "DI";
        case VirtualPortType::DIGITAL_OUTPUT: return "DO";
        case VirtualPortType::DIGITAL_OUTPUT_REVERSED: return "DOR";
        case VirtualPortType::DIGITAL_INPUT_PULLUP: return "DIP";
        case VirtualPortType::ANALOG_INPUT: return "AI";
        case VirtualPortType::ANALOG_OUTPUT: return "AO";
        case VirtualPortType::ANALOG_OUTPUT_PWM: return "AOP";
        case VirtualPortType::NONE: return "X";
    }
    return std::to_string((int)type);
}

string propertyTypeString(AndamBusPropertyType type) {
    switch (type) {
        case AndamBusPropertyType::PORT_TYPE:return "PORT_TYPE";
        case AndamBusPropertyType::PORT_VALUE:return "PORT_VALUE";
        case AndamBusPropertyType::DEVICE_TYPE:return "DEVICE_TYPE";
        case AndamBusPropertyType::W1CMD:return "W1CMD";
        case AndamBusPropertyType::SLAVE_HW_TYPE:return "Slave HW type";
        case AndamBusPropertyType::SW_VERSION:return "SW version";
        case AndamBusPropertyType::CUSTOM:return "Custom";
        case AndamBusPropertyType::LONG_ADDRESS:return "Long address";
        case AndamBusPropertyType::SLAVE_ADDRESS:return "Slave address";
        case AndamBusPropertyType::SECONDARY_BUS_REFRESH:return "W1 refresh";
        case AndamBusPropertyType::ITEM_ID:return "Item ID";
        case AndamBusPropertyType::TEMPERATURE:return "Temperature";
        case AndamBusPropertyType::PERIOD_SEC:return "Period(sec)";
        case AndamBusPropertyType::PIN:return "Pin";
        case AndamBusPropertyType::STEP:return "Step";
        case AndamBusPropertyType::PERIOD_MS:return "Period(ms)";
        case AndamBusPropertyType::HIGHLOGIC:return "Highlogic";
    }
    return std::to_string((int)type);
}

string metadataTypeString(MetadataType type) {
    switch (type) {
        case MetadataType::DIGITAL_INPUT_PORTS:return "Digital input ports";
        case MetadataType::ANALOG_INPUT_PORTS:return "Analog input ports";
        case MetadataType::DIGITAL_OUTPUT_PORTS:return "Digital output ports";
        case MetadataType::ANALOG_OUTPUT_PORTS:return "Analog output ports";
        case MetadataType::FREE_MEMORY:return "Free memory";

        case MetadataType::UPTIME:return "Uptime";
        case MetadataType::TICK:return "Tick";
        case MetadataType::MAX_CYCLE_DURATION:return "Max cycle duration";
        case MetadataType::CYCLE_DURATION_RANGE:return "Duration range";

        case MetadataType::ERROR_COUNT_CRC:return "Error count CRC";
        case MetadataType::ERROR_COUNT_SYNC:return "Error count SYNC";
        case MetadataType::ERROR_COUNT_LONG:return "Error count TOOLONG";
    }
    return to_string((int)type);
}

const string itemTypeString(ChangeListener::ItemType type) {
    switch (type) {
        case ChangeListener::ItemType::Bus:
            return "Bus";
        case ChangeListener::ItemType::Device:
            return "Device";
        case ChangeListener::ItemType::Port:
            return "Port";
    }

    return "Unknown";
}

const char* int_to_string(int i) {
    return to_string(i).c_str();
}

std::string getThermostatPropertyString(AndamBusPropertyType propType, uint8_t idx) {
    switch(propType) {
    case AndamBusPropertyType::ITEM_ID:
        return "Thermometer ID";
    case AndamBusPropertyType::TEMPERATURE:
        if (idx == 0)
            return "Initial temp";
        if (idx == 1)
            return "Hysteresis temp";
    case AndamBusPropertyType::HIGHLOGIC:
        switch(idx) {
        case 0:
            return "0/1->1/0";
        }
    default:
        return "";
    }
}

std::string getPushDetectorPropertyString(AndamBusPropertyType propType, uint8_t idx) {
    switch(propType) {
    case AndamBusPropertyType::ITEM_ID:
        switch(idx) {
        case 0:
            return "1-click port ID";
        case 1:
            return "2-click port ID";
        case 2:
            return "3-click port ID";
        }
    default:
        return "";
    }
}

std::string getBlindsControlPropertyString(AndamBusPropertyType propType, uint8_t idx) {
    switch(propType) {
    case AndamBusPropertyType::PIN:
        switch(idx) {
        case 0:
            return "Down button pin (IN)";
        case 1:
            return "Motor move pin (OUT)";
        case 2:
            return "Motor direction down pin (OUT)";
        default:
            return "Related device main pin";
        }
    case AndamBusPropertyType::PERIOD_MS:
        switch(idx) {
        case 0:
            return "Full up/down time in ms";
        case 1:
            return "Full open/shade time in ms";
        }
    default:
        return "";
    }
}

std::string getDiffThermostatPropertyString(AndamBusPropertyType propType, uint8_t idx) {
    switch(propType) {
    case AndamBusPropertyType::TEMPERATURE:
        switch(idx) {
        case 0:
            return "Difference to turn on";
        case 1:
            return "Difference to turn off";
        case 2:
            return "Low thermometer temp limit";
        }
    case AndamBusPropertyType::ITEM_ID:
        switch(idx) {
        case 0:
            return "Thermometer high";
        case 1:
            return "Thermometer low";
        }
    case AndamBusPropertyType::PERIOD_SEC:
        if (idx == 0)
            return "Noswitch period in seconds";
    default:
        return "";
    }
}

std::string getSolarThermostatPropertyString(AndamBusPropertyType propType, uint8_t idx) {
    switch(propType) {
    case AndamBusPropertyType::TEMPERATURE:
        if (idx == 3)
            return "Tank switch limit temp";
        break;
    case AndamBusPropertyType::PERIOD_SEC:
        if (idx == 1)
            return "Tank noswitch period";
        break;
    case AndamBusPropertyType::PIN:
        if (idx == 0)
            return "Tank switch pin (OUT)";
        break;
    case AndamBusPropertyType::ITEM_ID:
        if (idx == 2)
            return "Thermometer secondary tank";
        break;
    default:
        break;
    }
    return getDiffThermostatPropertyString(propType, idx);
}

std::string getCircPumpPropertyString(AndamBusPropertyType propType, uint8_t idx) {
    switch(propType) {
    case AndamBusPropertyType::TEMPERATURE:
        switch(idx) {
        case 0:
            return "Limit temperature";
        case 1:
            return "High temp limit";
        }
    case AndamBusPropertyType::PERIOD_SEC:
        switch(idx) {
        case 0:
            return "Seconds to run";
        case 1:
            return "Seconds idle";
        case 2:
            return "Seconds idle high temp";
        case 3:
            return "Noswitch period";
        }
    case AndamBusPropertyType::ITEM_ID:
        switch(idx) {
        case 0:
            return "Thermometer";
        }
    default:
        return "";
    }
}

std::string getModifierPropertyString(AndamBusPropertyType propType, uint8_t idx) {
    switch(propType) {
    case AndamBusPropertyType::ITEM_ID:
        switch(idx) {
        case 0:
            return "Target port ID";
        }
    case AndamBusPropertyType::STEP:
        switch(idx) {
        case 0:
            return "Modify step";
        }
    default:
        return "";
    }
}

std::string getDisplayPropertyString(AndamBusPropertyType propType, uint8_t idx) {
    switch(propType) {
    case AndamBusPropertyType::ITEM_ID:
        if (idx>=0 || idx <= 5)
            return "Thermometer";
    case AndamBusPropertyType::PERIOD_SEC:
        switch(idx) {
        case 0:
            return "Backlight period seconds";
        }
    default:
        return "";
    }
}

std::string getTimerPropertyString(AndamBusPropertyType propType, uint8_t idx) {
    switch(propType) {
    case AndamBusPropertyType::PERIOD_SEC:
        switch(idx) {
        case 0:
            return "Timer seconds";
        }
    default:
        return "";
    }
}

std::string getHvacPropertyString(AndamBusPropertyType propType, uint8_t idx) {
    switch(propType) {
    case AndamBusPropertyType::ITEM_ID:
        switch(idx) {
        case 0:
            return "In thermometer";
        case 1:
            return "Out thermometer";
        case 2:
            return "Digita potmeter port";
        case 3:
            return "Target temperature port";
        }
    case AndamBusPropertyType::TEMPERATURE:
        switch(idx) {
        case 0:
            return "Target temperature";
        case 1:
            return "HP target temperature";
        }
    case AndamBusPropertyType::PIN:
        switch(idx) {
        case 0:
            return "On/Off (IN)";
        case 1:
            return "Heat mode (IN)";
        case 2:
            return "Ventilation request (IN)";
        case 3:
            return "Ventilator (Out)";
        }
    default:
        return "";
    }
}

std::string getDevicePropertyString(VirtualDeviceType devType, AndamBusPropertyType propType, uint8_t idx) {
    switch(devType) {
    case VirtualDeviceType::THERMOMETER: // no property needed
    case VirtualDeviceType::NONE:
    case VirtualDeviceType::DIGITAL_POTENTIOMETER:
        return "";
    case VirtualDeviceType::THERMOSTAT:
        return getThermostatPropertyString(propType, idx);
    case VirtualDeviceType::PUSH_DETECTOR:
        return getPushDetectorPropertyString(propType, idx);
    case VirtualDeviceType::BLINDS_CONTROL:
        return getBlindsControlPropertyString(propType, idx);
    case VirtualDeviceType::DIFF_THERMOSTAT:
        return getDiffThermostatPropertyString(propType, idx);
    case VirtualDeviceType::SOLAR_THERMOSTAT:
        return getSolarThermostatPropertyString(propType, idx);
    case VirtualDeviceType::CIRC_PUMP:
        return getCircPumpPropertyString(propType, idx);
    case VirtualDeviceType::MODIFIER:
        return getModifierPropertyString(propType, idx);
    case VirtualDeviceType::I2C_DISPLAY:
        return getDisplayPropertyString(propType, idx);
    case VirtualDeviceType::TIMER:
        return getTimerPropertyString(propType, idx);
    case VirtualDeviceType::HVAC:
        return getHvacPropertyString(propType, idx);
    }
    return "";
}


uint64_t stripLongAddress(uint64_t addr) {
    return (addr & 0xffffffffffff00ull) >> 8;
}


auto defaultLog_t_start = chrono::system_clock::now();

void defaultLogger(LogLevel level, const string &msg) {
    auto now = chrono::system_clock::now();
    unsigned long millis = chrono::duration_cast<chrono::milliseconds>(now - defaultLog_t_start).count();

    if (millis > 1000000)
        cout << millis/1000000 << ".";

    cout << (millis/1000)%1000 << "." << (millis/10)%100 << " " << LogLevelString(level) << ":" << msg << endl;
}

void printDevice(VirtualDevice dev) {
    cout << "\tdevice " << (int)dev.id << " on " << (int)dev.busId << " pin " << (int)dev.pin << " of type " << virtualDeviceTypeString(dev.type) << endl;
}

void printPort(VirtualPort port) {
    cout << "\tport " << (int)port.id << " on " << (int)port.deviceId << " pin " << (int)port.pin << " of type " << virtualPortTypeString(port.type) << endl;
}

void printProperty(ItemProperty prop) {
    cout << "\tproperty " << (int)prop.propertyId << " " << propertyTypeString(prop.type) << "=" << prop.value << "(0x" << hex << prop.value << dec << ") on " << (int)prop.entityId << endl;
}

void printFrameResponseData(AndamBusResponseData &rdata) {
    cout << " content " << responseContentString(rdata.content) << " cnt: " << (int)rdata.itemCount << endl;
    if (rdata.content == ResponseContent::PROPERTY) {
        for (int i=0; i<rdata.itemCount;i++)
            printProperty(rdata.responseData.props[i]);
    }
    if (rdata.content == ResponseContent::MIXED) {
        VirtualDevice *vdevs = rdata.responseData.deviceItems;
        VirtualPort *vports = reinterpret_cast<VirtualPort*>(vdevs + rdata.itemCount);
        ItemProperty *props = reinterpret_cast<ItemProperty*>(vports + rdata.itemCount2);

        for (int i=0; i<rdata.itemCount;i++)
            printDevice(vdevs[i]);
        for (int i=0; i<rdata.itemCount2;i++)
            printPort(vports[i]);
        for (int i=0; i<rdata.itemCount3;i++)
            printProperty(props[i]);

        hexdump((const char*)&rdata, 24, LogLevel::WARNING);
    }
}

void printFrameResponse(AndamBusFrameResponse &resp) {
    cout << " resp " << responseTypeString(resp.responseType);

    if (resp.responseType == AndamBusResponseType::ERROR)
        cout << " " << responseErrorString(resp.typeError.errorCode, AndamBusCommand::NONE);
    if (resp.responseType == AndamBusResponseType::OK_DATA)
        printFrameResponseData(resp.typeOkData);
}

void printFrameCommand(AndamBusFrameCommand &cmd) {
    cout << " cmd " << commandString(cmd.command) << " id:" << (uint32_t)cmd.id << " pin:" << (uint32_t)cmd.pin << " propCnt:" << (int)cmd.propertyCount;
}

void printFrame(AndamBusFrame &frm) {
    auto now = chrono::system_clock::now();
    cout << chrono::duration_cast<chrono::milliseconds>(now - defaultLog_t_start).count() << " frame addr=" << frm.header.slaveAddress << " len=" << frm.header.payloadLength << " cnt=" << frm.header.counter;

    if (frm.header.slaveAddress == 0)
        printFrameResponse(frm.response);
    else
        printFrameCommand(frm.command);

    cout << endl;
}


static fnLog logger = defaultLogger;

LogLevel logLevel = LogLevel::INFO;

void setLogLevel(LogLevel lvl) {
    logLevel = lvl;
}

void Log(LogLevel lvl, const string &msg) {
    if (logger != nullptr)
        logger(lvl, msg);
}

void setDebugCallback(fnLog _logger) {
    logger = _logger;
}

string unitInfoString(uint16_t addr, string sHwType, SlaveHwType hwType, string sSwVersionString, string sApiVersionString) {
    stringstream ss;
    ss << "detected unit " << addr <<",HW Type:" << sHwType << "(" << (int)hwType << ")"<< ",SW version:" << sSwVersionString << ", API version:" << sApiVersionString;
    return ss.str();
}

vector<string> split(const string& str, const string& delim)
{
    vector<string> tokens;
    size_t prev = 0, pos = 0;
    do
    {
        pos = str.find(delim, prev);
        if (pos == string::npos) pos = str.length();
        string token = str.substr(prev, pos-prev);
        if (!token.empty()) tokens.push_back(token);
        prev = pos + delim.length();
    }
    while (pos < str.length() && prev < str.length());
    return tokens;
}

void nonblock_stdin() {
    stringstream ss;
    int flags = fcntl(STDIN_FILENO, F_GETFL);

    if (flags == -1) {
        ss << "fcntl get:" << strerror(errno);
        throw runtime_error(ss.str());
    }

    int ret = fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    if (ret == -1) {
        ss << "fcntl set:" << strerror(errno);
        throw runtime_error(ss.str());
    }
}

void blocking_stdin() {
    stringstream ss;
    int flags = fcntl(STDIN_FILENO, F_GETFL);

    if (flags == -1) {
        ss << "fcntl get:" << strerror(errno);
        throw runtime_error(ss.str());
    }

    int ret = fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);

    if (ret == -1) {
        ss << "fcntl set:" << strerror(errno);
        throw runtime_error(ss.str());
    }
}
