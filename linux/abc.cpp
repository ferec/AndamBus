#include <iostream>

#include <string>
/*#include <fcntl.h>
#include <unistd.h>*/
#include <fstream>

#include "SerialBroadcastSocket.h"
#include "testing/LocalBroadcastSocket.h"
#include "AndamBusMaster.h"
#include "AndamBusSlave.h"

using namespace std;

#define CFG_BUFSIZE 2048

bool stop = false;

AndamBusMaster *abu = nullptr;

ChangeListener chgList;

void ChangeListener::valueChanged(AndamBusSlave *slave, SlaveVirtualPort *port, int32_t oldValue) {
    cout << dec << "port value changed on unit " << slave->getAddress() << " pin=" << (int)port->getPin() << " from " << oldValue << " to " << port->getValue() << endl;;
}

void ChangeListener::idChanged(AndamBusSlave *slave, ChangeListener::ItemType type, uint8_t oldId, uint8_t newId) {
    cout << dec << itemTypeString(type) << " ID changed on unit " << slave->getAddress() << " from " << (int)oldId << " to " << (int)newId << endl;
}

void printDeviceTypes() {
//        cout << "\t\tTHM = THERMOMETER" << endl;
        cout << "\t\tTHS = THERMOSTAT" << endl;
        cout << "\t\tBLC = BLINDS_CONTROL" << endl;
        cout << "\t\tPSH = Push/click detector" << endl;
        cout << "\t\tDFT = Differential themostat" << endl;
        cout << "\t\tCIP = Hot water circulation" << endl;
        cout << "\t\tSOT = Solar themostat" << endl;
        cout << "\t\tPOT = Digital potentiometer" << endl;
        cout << "\t\tLCD = LCD display" << endl;
        cout << "\t\tHVC = HVAC" << endl;
        cout << "\t\tTMR = Timer" << endl;
        cout << "\t\tMOD = Modifier" << endl;
}

void printPropertyTypes() {
        cout << "1 port type" << endl;
        cout << "3 device type" << endl;
        cout << "7 custom property" << endl;
        cout << "9 unit address" << endl;
        cout << "10 1-wire bus refresh period in seconds" << endl;
        cout << "11 item ID" << endl;
        cout << "12 temperature" << endl;
        cout << "13 period(sec)" << endl;
        cout << "14 pin" << endl;
        cout << "15 step" << endl;
        cout << "16 period(ms)" << endl;
        cout << "17 highlogic" << endl;
}

void printPortTypes() {
        cout << "\t\tDIP = Digital input with pullup" << endl;
        cout << "\t\tDO = Digital output" << endl;
        cout << "\t\tDOR = Digital output reversed" << endl;
        cout << "\t\tAI = Analog input" << endl;
        cout << "\t\tAO = Analog output" << endl;
        cout << "\t\tAOP = Analog output PWM" << endl;
        cout << "\t\tDI = Digital input" << endl;
}

void printUsage(string sType) {
    char type = sType.size()==0?'*':'x';
    if (sType.size()==1)
        type = sType[0];

    if (type != '*' && type!='l' && type!='c' && type != 'b' && type != 'p') {
        cout << "unknown type " << sType << endl;
        return;
    }

    if (type == '*') {
        cout << "usage:" << endl;
        cout << "abc <dev>" << endl;
        cout << "\twhere dev can be a real device or string test to use unix socket" << endl;
        cout << "h [l|c|p] - this help" << endl;
        cout << "q - quit" << endl;
        cout << "i - version info" << endl;
        cout << "m - refresh metadata/telemetry" << endl;
        cout << "d - detect units" << endl;
        cout << "p - print communication statistics" << endl;
        cout << "r [unit address] - refresh unit/all units" << endl;
        cout << "g <unit address> - get new items (devs/ports)" << endl;
        cout << "w <unit address> - writes configuration to unit's persistent memory" << endl;
        cout << "v [unit address] [F] - refresh values for changed ports, F option for all ports" << endl;
        cout << "x <unit address> - restart unit" << endl;
        cout << "s <unit address> <port id> <value> - set port value" << endl;
        cout << "e <unit address> <id> - deletes bus/device/port by ID" << endl;
        cout << "a <unit address> <id> [<property> <value>] [prop id] - gets/sets property value on unit(id=0)/bus/device/port" << endl;
        cout << "t <unit address> <id> <data> - sends raw data to bus/device/port identified by ID" << endl;
        cout << "f <unit address> <file name> - get config data from unit and store in a binary file" << endl;
        cout << "k <unit address> <file name> - read config data from binary file and restores on unit" << endl;
        cout << ". <log level> - sets log level to e - error, w - warning, i - info, d - debug" << endl;
        cout << "\tfor property list run help command" << endl;
    }
    if (type=='p') {
        cout << "Property types:" << endl;
        printPropertyTypes();
    }
    if (type == '*' || type=='b') {
        cout << "b d <unit address> [bus id] - bus detect on bus/all buses" << endl;
        cout << "b c <unit address> [bus id] - bus convert on bus/all buses" << endl;
    }
    if (type == '*' || type=='l') {
        cout << "l s - list units" << endl;
        cout << "l b <unit address> - list buses" << endl;
        cout << "l d <unit address> - list virtual devices" << endl;
        cout << "l p <unit address> - list ports" << endl;
        cout << "l u <unit address> - list unused pins" << endl;
    }
    if (type == '*' || type=='c') {
        cout << "c b <unit address> <pin> - create bus" << endl;
        cout << "c d <unit address> <pin> <type> - create device" << endl;
        cout << "\twhere type is \n";
        printDeviceTypes();
        cout << "c p <unit address> <pin> <type> - create port" << endl;
        cout << "\twhere type is \n";
        printPortTypes();
    }
}


VirtualDeviceType getDeviceTypeByCode(string code) {
    if (code == "THM") return VirtualDeviceType::THERMOMETER;
    if (code == "THS") return VirtualDeviceType::THERMOSTAT;
    if (code == "BLC") return VirtualDeviceType::BLINDS_CONTROL;
    if (code == "PSH") return VirtualDeviceType::PUSH_DETECTOR;
    if (code == "DFT") return VirtualDeviceType::DIFF_THERMOSTAT;
    if (code == "CIP") return VirtualDeviceType::CIRC_PUMP;
    if (code == "SOT") return VirtualDeviceType::SOLAR_THERMOSTAT;
    if (code == "POT") return VirtualDeviceType::DIGITAL_POTENTIOMETER;
    if (code == "LCD") return VirtualDeviceType::I2C_DISPLAY;
    if (code == "HVC") return VirtualDeviceType::HVAC;
    if (code == "TMR") return VirtualDeviceType::TIMER;
    if (code == "MOD") return VirtualDeviceType::MODIFIER;
    return VirtualDeviceType::NONE;
}

void printVersion() {
    cout << "AndamBus library version:" << (int)abu->getLibVersionMajor() << "." << (int)abu->getLibVersionMinor() << " API version:" << (int)abu->getApiVersionMajor() << "." << (int)abu->getApiVersionMinor() << endl;
}

void printUnits() {
    cout << "unit count:" << abu->getSlaves().size() << endl;
    for (auto it=abu->getSlaves().begin();it!=abu->getSlaves().end();it++) {
        AndamBusSlave *slave = *it;
        cout << unitInfoString(slave->getAddress(), slave->getHwTypeString(), slave->getHwType(), slave->getSWVersionString(), slave->getApiVersionString()) << endl;;
    }
}

int parseInt(string s, int dflt) {
    if (s.size() == 0) {
        return dflt;
    }

    if (s.size() > 2 && s.substr(0,2) == "0x")
        return stoi(s,nullptr, 16);

    return stoi(s);
}

int parseInt(string s) {
    if (s.size() == 0) {
        throw invalid_argument(s);
    }

    return stoi(s);
}

void refreshUnit(string sAddr) {
    uint16_t addr = parseInt(sAddr, 0);

    AndamBusSlave *slave;

    try {
        if (addr == 0)
            for (auto it=abu->getSlaves().begin();it!=abu->getSlaves().end();it++) {
                abu->refreshSlave((*it)->getAddress());
                (*it)->setChangeListener(&chgList);
            }
        else {
            slave = abu->findSlaveByAddress(addr);
            if (slave != nullptr) {
                abu->refreshSlave(slave);
                slave->setChangeListener(&chgList);
            }
        }
    } catch (ExceptionBase &e) {
        AB_ERROR("Error:" << e.what());
    }
    cout << "refreshed" << endl;
}

void printBuses(AndamBusSlave *slave) {
    cout << "Listing buses (" << dec << slave->getSecondaryBuses().size() << ")" << endl;
    for (auto it=slave->getSecondaryBuses().begin();it!=slave->getSecondaryBuses().end();it++) {
        SlaveSecondaryBus *bus = *it;
        cout << "id " << (int)bus->getId() << " on pin " << (int)bus->getPin() << endl;;

        if (bus->isDeviceMissing()) {
            cout << " missing device" << endl;
        }
    }
}

void printDevices(AndamBusSlave *slave) {
    cout << dec << "Listing devices (" << slave->getVirtualDevices().size() << ")" << endl;
    for (auto it=slave->getVirtualDevices().begin();it!=slave->getVirtualDevices().end();it++) {
        SlaveVirtualDevice *dev = *it;
        if (dev->getBus()!=nullptr)
            cout << "id " << (int)dev->getId() << " 0x" << hex << stripLongAddress(dev->getLongAddress()) << dec << " (pin " << (int)dev->getPin() << "," << (int)(dev->getBus()->getId()) << ")";
        else
            cout << "id " << (int)dev->getId() << " (pin " << (int)dev->getPin() << "," << virtualDeviceTypeString(dev->getType()) << ")";

        if (dev->getStatus() != 0)
            cout << " (status=" << (int)dev->getStatus() << ")" << endl;
        if (!dev->isConfigured())
            cout << " not configured ";
        if (!dev->hasAllInputDevices())
            cout << " missing input dev ";
        if (dev->hasInputError())
            cout << " input error ";
        cout << endl;
    }
}

void printPorts(AndamBusSlave *slave) {
    cout << "Listing ports (" << dec << slave->getVirtualPorts().size() << ")" << endl;
    for (auto it=slave->getVirtualPorts().begin();it!=slave->getVirtualPorts().end();it++) {
        SlaveVirtualPort *port = *it;
        SlaveVirtualDevice *dev = port->getDevice();
        if (dev==nullptr)
            cout << "pin " << (int)port->getPin() << " " << virtualPortTypeStringShort(port->getType());
        else {
            if (dev->getBus() != nullptr)
                cout << "dev 0x" << hex << stripLongAddress(dev->getLongAddress()) << dec << "(" << (int)port->getId() << ")";
            else
                cout << "id " << (int)port->getId() << " p " << (int)port->getIdOnDevice() << " (pin " << (int)port->getPin() << " dev " << (int)dev->getId() << "," << virtualPortTypeStringShort(port->getType()) << ")";
        }

        cout << " value=" << port->getValue();

        if (port->getAge() > 0)
            cout << " age " << (int)port->getAge();
        cout << endl;
    }
}

void printUnused(AndamBusSlave *slave) {
    set<uint8_t> up = slave->getUnusedPins();
    cout << "Unused pins:";
    for (auto it = up.begin(); it != up.end(); it++)
        cout << (int)*it << " ";

    cout << endl;
}

void printList(string type, string sAddr) {
    if (type.size() != 1)
        return;

    uint16_t addr = parseInt(sAddr, 0);

    AndamBusSlave *slave = abu->findSlaveByAddress(addr);

    if (slave == nullptr && type[0] != 's') {
        cout << "unit with address " << addr << " does not exist " << endl;
        return;
    }

    switch(type[0]) {
    case 's':
        printUnits();
        break;
    case 'b':
        printBuses(slave);
        break;
    case 'd':
        printDevices(slave);
        break;
    case 'p':
        printPorts(slave);
        break;
    case 'u':
        printUnused(slave);
        break;
    }
}

void createBus(AndamBusSlave *slave, uint8_t pin) {
    slave->createSecondaryBus(pin);
}

VirtualPortType getPortTypeByCode(string code) {
    if (code == "DI") return VirtualPortType::DIGITAL_INPUT;
    if (code == "DIP") return VirtualPortType::DIGITAL_INPUT_PULLUP;
    if (code == "DO") return VirtualPortType::DIGITAL_OUTPUT;
    if (code == "DOR") return VirtualPortType::DIGITAL_OUTPUT_REVERSED;
    if (code == "AI") return VirtualPortType::ANALOG_INPUT;
    if (code == "AO") return VirtualPortType::ANALOG_OUTPUT;
    if (code == "AOP") return VirtualPortType::ANALOG_OUTPUT_PWM;
    return VirtualPortType::NONE;
}

void createDevice(AndamBusSlave *slave, uint8_t pin, string sType) {
    VirtualDeviceType type = getDeviceTypeByCode(sType);
    if (type == VirtualDeviceType::NONE) {
        cout << "Unknown device type " << sType << endl;
        printDeviceTypes();
        return;
    }

    slave->createVirtualDevice(type, pin);
}

void createPort(AndamBusSlave *slave, uint8_t pin, string sType) {
    VirtualPortType type = getPortTypeByCode(sType);
    slave->createVirtualPort(pin, type);
}

void createOnSlave(string type, string sAddr, string sPin, string sType) {
    if (type.size() != 1)
        return;

    uint16_t pin = parseInt(sPin, 0);

    if (pin == 0)
        return;

    uint16_t addr = parseInt(sAddr, 0);

    AndamBusSlave *slave = abu->findSlaveByAddress(addr);

    if (slave == nullptr) {
        cout << "unit with address " << addr << " does not exist " << endl;
        return;
    }

    switch(type[0]) {
    case 'b':
        createBus(slave, pin);
        break;
    case 'd':
        if (sType.size() > 0)
            createDevice(slave, pin, sType);
        else
            cout << "type is mandatory"<<endl;
        break;
    case 'p':
        if (sType.size() > 0)
            createPort(slave, pin, sType);
        else
            cout << "type is mandatory"<<endl;
        break;
    }
}

void busDetect(AndamBusSlave *slave, uint8_t busId) {
    if (busId == 0)
        for (auto it=slave->getSecondaryBuses().begin();it!=slave->getSecondaryBuses().end();it++)
            slave->secondaryBusDetect((*it)->getId());
    else
        slave->secondaryBusDetect(busId);
}

void busConvert(AndamBusSlave *slave, uint8_t busId) {
    if (busId == 0)
        for (auto it=slave->getSecondaryBuses().begin();it!=slave->getSecondaryBuses().end();it++)
            slave->secondaryBusRunCommand((*it)->getId(), OnewireBusCommand::CONVERT);
    else
        slave->secondaryBusRunCommand(busId, OnewireBusCommand::CONVERT);
}

void removeItem(string sAddr, string sId) {
    uint16_t addr = parseInt(sAddr, 0);

    if (addr == 0) {
        cout << "Address is mandatory" << endl;
        return;
    }

    AndamBusSlave *slave = abu->findSlaveByAddress(addr);

    if (slave == nullptr) {
        cout << "unit not found" << endl;
        return;
    }

    uint8_t id = parseInt(sId, 0);

    if (id == 0) {
        cout << "ID is mandatory" << endl;
        return;
    }

    SlaveVirtualPort *port = slave->getPort(id);

    if (port != nullptr) {
        slave->removeVirtualPort(id);
        cout << "Port removed" << endl;
        return;
    }

    SlaveVirtualDevice *dev = slave->getDevice(id);

    if (dev != nullptr) {
        slave->removeVirtualDevice(id);
        cout << "Device removed" << endl;
        return;
    }

    SlaveSecondaryBus *bus = slave->getBus(id);

    if (bus != nullptr) {
        slave->removeSecondaryBus(id);
        cout << "Bus removed" << endl;
        return;
    }
}

AndamBusSlave *getSlaveByAddr(string sAddr) {
    uint16_t addr = parseInt(sAddr, 0);

    if (addr == 0) {
        cout << "Address is mandatory" << endl;
    }

    AndamBusSlave *slave = abu->findSlaveByAddress(addr);

    if (slave == nullptr) {
        cout << "unit not found" << endl;
    }

    return slave;
}

void printProperties(AndamBusSlave *slave, uint8_t id) {
    map<AndamBusPropertyType,map<uint8_t,int32_t>> *mp = nullptr;

    if (id == 0)
        mp = &slave->getProperties();

    if (mp == nullptr) {
        SlaveSecondaryBus *bus = slave->getBus(id);
        if (bus != nullptr)
            mp = &bus->getProperties();
    }

    SlaveVirtualDevice *dev = slave->getDevice(id);
    if (mp == nullptr) {
        if (dev != nullptr)
            mp = &dev->getProperties();
    }

    if (mp == nullptr) {
        SlaveVirtualPort *port = slave->getPort(id);
        if (port != nullptr)
            mp = &port->getProperties();
        else
            cout << "Item not found" << endl;
    }

    if (mp == nullptr) {
        cout << "no property found" << endl;
        return;
    }

    if (mp->size() == 0) {
        cout << "property list empty" << endl;
        return;
    }

    for(auto it=mp->begin();it!=mp->end();it++) {
        AndamBusPropertyType type = it->first;
        map<uint8_t,int32_t> &p = it->second;

        for (auto itp=p.begin();itp!=p.end();itp++) {
            cout << "Property " << dec << propertyTypeString(type) << " (id " << (int)itp->first;

            if (dev != nullptr) {
                string ps = getDevicePropertyString(dev->getType(), type, itp->first);
                if (ps.length() > 0)
                    cout << "," << ps;
            }

            cout << ") value=";
            if (type == AndamBusPropertyType::LONG_ADDRESS)
                cout << "0x" << hex;
            cout << itp->second << endl;

        }
    }
}

void setProperty(string &sAddr, string &sId, string &sPropTypeId, string &sPropValue, string &sPropId) {
    AndamBusSlave *slave = getSlaveByAddr(sAddr);

    if (slave == nullptr)
        return;

    uint8_t id = parseInt(sId, 0xff);

    if (id == 0xff) {
        cout << "ID is mandatory" << endl;
        return;
    }

    uint8_t propTypeId = parseInt(sPropTypeId, 0xff);

    if (propTypeId == 0xff) {
        printProperties(slave, id);
        return;
    }

    uint8_t propId = parseInt(sPropId, 0);

    int32_t propValue = parseInt(sPropValue, 0);

    slave->doPropertySet(id, static_cast<AndamBusPropertyType>(propTypeId), propValue, propId);
}

void setData(string &sAddr, string &sId, string &sData) {
    AndamBusSlave *slave = getSlaveByAddr(sAddr);

    if (slave == nullptr)
        return;

    uint8_t id = parseInt(sId, 0);

    if (id == 0) {
        cout << "ID is mandatory" << endl;
        return;
    }

    slave->sendData(id, sData.size(), sData.c_str());
}

void setPortValue(string &sAddr, string &sPortId, string &sValue) {
    AndamBusSlave *slave = getSlaveByAddr(sAddr);

    if (slave == nullptr)
        return;

    uint8_t portId = parseInt(sPortId, 0);

    if (portId == 0) {
        cout << "Port ID is mandatory" << endl;
        return;
    }

    SlaveVirtualPort *port = slave->getPort(portId);

    if (port == nullptr) {
        cout << "Port not found" << endl;
        return;
    }

    int32_t value;
    try {
        value = parseInt(sValue);
    } catch (invalid_argument &e) {
        cout << "Value is mandatory and must be a number" << endl;
        return;
    }

    slave->setPortValue(port, value);
}

void busOperation(string type, string sAddr, string sBusId) {
    if (type.size() != 1)
        return;

    uint16_t addr = parseInt(sAddr, 0);

    if (addr == 0) {
        cout << "address is mandatory"<< endl;
        return;
    }

    AndamBusSlave *slave = abu->findSlaveByAddress(addr);

    if (slave == nullptr && type[0] != 's') {
        cout << "unit with address " << addr << " does not exist " << endl;
        return;
    }

    uint16_t busId = parseInt(sBusId, 0);

/*    if (busId == 0) {
        cout << "bus ID is mandatory"<< endl;
        return;
    }*/

    switch(type[0]) {
        case 'd':
            busDetect(slave, busId);
            break;
        case 'c':
            busConvert(slave, busId);
            break;
        default:
            cout << "Invalid type " << type << endl;
    }
}

void getNewItems(string sAddr) {
    uint16_t addr = parseInt(sAddr, 0);

    if (addr == 0) {
        cout << "address is mandatory"<< endl;
        return;
    }

    AndamBusSlave *slave = abu->findSlaveByAddress(addr);

    if (slave == nullptr) {
        cout << "unit with address " << addr << " does not exist " << endl;
        return;
    }

    slave->refreshVirtualItems();
}

void refreshValues(string sAddr, string sOpt) {
    uint16_t addr = parseInt(sAddr, 0);

/*    if (addr == 0) {
        cout << "address is mandatory"<< endl;
        return;
    }*/

    AndamBusSlave *slave = abu->findSlaveByAddress(addr);

    if (slave == nullptr && addr != 0) {
        cout << "unit with address " << addr << " does not exist " << endl;
        return;
    }

    bool optFull = false;
    if (sOpt == "F")
        optFull = true;

    if (addr == 0)
        for (auto it=abu->getSlaves().begin();it!=abu->getSlaves().end();it++)
            (*it)->refreshPortValues(optFull);
    else
        slave->refreshPortValues(optFull);
}

void refreshMetadata(string sAddr) {
    uint16_t addr = parseInt(sAddr, 0);

    if (addr == 0) {
        cout << "address is mandatory"<< endl;
        return;
    }

    AndamBusSlave *slave = abu->findSlaveByAddress(addr);

    if (slave == nullptr && addr != 0) {
        cout << "unit with address " << addr << " does not exist " << endl;
        return;
    }

    slave->refreshMetadata();
}

void restartSlave(string sAddr) {
    uint16_t addr = parseInt(sAddr, 0);

    if (addr == 0) {
        cout << "address is mandatory"<< endl;
        return;
    }

    abu->doReset(addr);
}

void getConfig(string sAddr, string filename) {
    AndamBusSlave *slave = getSlaveByAddr(sAddr);

    if (slave == nullptr)
        return;

    uint8_t buf[CFG_BUFSIZE];
    uint16_t len = slave->getConfig(buf, CFG_BUFSIZE);

    hexdump((const char*)buf, len);
    std::ofstream cfgFile (filename,std::ofstream::binary);

    cfgFile.write ((const char*)buf,len);
    cfgFile.close();
}

void restoreConfig(string sAddr, string filename) {
    AndamBusSlave *slave = getSlaveByAddr(sAddr);

    if (slave == nullptr)
        return;

    ifstream cfgFile( filename, std::ios::binary );

    if(!cfgFile) {
      cout << "Cannot open file " << filename << endl;
      return;
    }

    uint8_t buf[CFG_BUFSIZE];
    cfgFile.read((char*)buf, CFG_BUFSIZE);
    uint16_t len = cfgFile.gcount();
    cfgFile.close();

    slave->restoreConfig(buf, len);
}

void persistSlave(string &sAddr) {
    AndamBusSlave *slave = getSlaveByAddr(sAddr);

    if (slave == nullptr)
        return;

    abu->persistSlave(slave->getAddress());
}

void printStats() {
    cout << "packets stats sent:" << abu->getSentCount() << " received:" << abu->getReceivedCount() << " cmd err:" << abu->getCommandErrorCount()
        << " invalid frm:" << abu->getInvalidFrameCount() << " sync lost:" << abu->getSyncLostCount() << " bus timeout:" << abu->getBusTimeoutCount() << endl;
}

void detectUnits() {
    cout << "detect start" << endl;
    abu->detectSlaves();

    cout << "detect finish" << endl;
}

void setLogLevel(string lvl) {
    if (lvl.size() < 1) {
        cout << "invalid log level " << lvl << endl;
        return;
    }

    LogLevel ll;

    switch (lvl[0]) {
    case 'e':
        ll = LogLevel::ERROR;
        break;
    case 'w':
        ll = LogLevel::WARNING;
        break;
    case 'i':
        ll = LogLevel::INFO;
        break;
    case 'd':
        ll = LogLevel::DEBUG;
        break;
    default:
        cout << "invalid log level " << lvl << endl;
        return;
    }

    setLogLevel(ll);
}

void doCommand(string command, string arg1, string arg2, string arg3, string arg4, string arg5) {
    if (command.size() != 1)
        return;

    switch (command[0]) {
        case 'q':
            cout << "quitting" << endl;
            stop = true;
            break;
        case 'h':
            printUsage(arg1);
            break;
        case 'i':
            printVersion();
            break;
        case 'd':
            detectUnits();
            break;
        case 'l':
            printList(arg1, arg2);
            break;
        case 'c':
            createOnSlave(arg1, arg2, arg3, arg4);
            break;
        case 'a':
            setProperty(arg1, arg2, arg3, arg4, arg5);
            break;
        case 'b':
            busOperation(arg1, arg2, arg3);
            break;
        case 's':
            setPortValue(arg1, arg2, arg3);
            break;
        case 't':
            setData(arg1, arg2, arg3);
            break;
        case 'p':
            printStats();
            break;
        case 'e':
            removeItem(arg1, arg2);
            break;
        case 'g':
            getNewItems(arg1);
            break;
        case 'r':
            refreshUnit(arg1);
            break;
        case 'm':
            refreshMetadata(arg1);
            break;
        case 'v':
            refreshValues(arg1, arg2);
            break;
        case 'x':
            restartSlave(arg1);
            break;
        case 'w':
            persistSlave(arg1);
            break;
        case 'f':
            getConfig(arg1, arg2);
            break;
        case 'k':
            restoreConfig(arg1, arg2);
            break;
        case '.':
            setLogLevel(arg1);
            break;
        default:
            cout << "Invalid command " << command << endl;
            printUsage(arg1);
    }
}

void processLine() {
    string line;
    getline(cin, line);

/*    char buf[64];
    int r = read(STDIN_FILENO, buf, 64);*/

    if (line.size() > 0) {
        vector<string> cmds = split(line, " ");

        doCommand(cmds[0], cmds.size()>1?cmds[1]:"", cmds.size()>2?cmds[2]:"", cmds.size()>3?cmds[3]:"", cmds.size()>4?cmds[4]:"", cmds.size()>5?cmds[5]:"");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printUsage("*");
        return 1;
    }

    blocking_stdin();

    setLogLevel(LogLevel::INFO);

    string tstSerial("test");

    BroadcastSocket *bs = nullptr;
    try {
        if (argv[1] == tstSerial)
            bs = new LocalBroadcastSocket();
        else
            bs = new SerialBroadcastSocket(argv[1]);

        abu = new AndamBusMaster(*bs);
    } catch (ExceptionBase &e) {
        AB_ERROR("Error:" << e.what());
        return 1;
    }

    while(!stop) {
        try {
            processLine();
            } catch (ExceptionBase &e) {
                AB_ERROR("Error:" << e.what());
            } catch (exception &e) {
                AB_ERROR("Error:" << e.what());
            }
        }

    if (abu != nullptr)
        delete abu;
/*    if (bs != nullptr)
        delete bs;*/

    return 0;
}
