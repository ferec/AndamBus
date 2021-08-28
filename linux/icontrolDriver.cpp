#include "icontrolDriver.h"
#include "SerialBroadcastSocket.h"
#include "AndamBusSlave.h"
//#include "../iControl/include/driver.h"
#include <vector>
#include <algorithm>
#include <cassert>

//#include <execinfo.h>
//#include <stdio.h>
//#include <unistd.h>

using namespace std;

//t_valueCallback _valueCallback = NULL;
//void* _driverBase = NULL;

AndamBusMaster *abm=nullptr;
uint32_t refreshCounter=0;

vector<uint64_t> w1addrs;

ChangeListener vclist;

void ChangeListener::idChanged(AndamBusSlave *slave, ItemType type, uint8_t oldId, uint8_t newId) {
    INFO("id changed " << (int)oldId << " -> " << (int)newId);
}

void ChangeListener::metadataChanged(AndamBusSlave *unit, SlaveVirtualDevice *dev, MetadataType type, uint16_t propertyId, int32_t value) {
    if (unit == nullptr)
        return;
//    INFO("metadataChanged " << (int)type << " to " << value);

    DeviceMetadataType dtype;
    int dvalue;
    bool relevant = convertMetadataType(type, dtype, value, dvalue);

    if (_metadataCallback == nullptr) {
        WARNING("Metadata callback not set");
        return;
    }

    if (!relevant) {
        DEBUG("Metadata type not relevant " << (int)type);
        return;
    }

    if (dev!=nullptr) {
        DEBUG("dev _metadataCallback try count " << dev2devId(unit->getAddress(), dev) << " to " << value);
        _metadataCallback(_driverBase, dev2devId(unit->getAddress(), dev), dtype, propertyId, dvalue);
    }
    else
        _metadataCallback(_driverBase, unit2devId(unit->getAddress()), dtype, propertyId, dvalue);
}

void ChangeListener::valueChanged(AndamBusSlave *unit, SlaveVirtualPort *port, int32_t oldValue) {
    if (unit == nullptr || port == nullptr)
        return;
//    cout << "valueChanged" << endl;
    DEBUG("valueChanged");

    uint8_t unitId = unit->getAddress();

    int portId, devId;

    if (port->getDevice() == nullptr) { // unit pin
        portId = port2Id(unitId, port);
        devId = unitId;
        DEBUG("pin value changed on unit " << unit->getAddress() << " pin=" << (int)port->getPin() << " port=" << (int)port->getIdOnDevice() << " from " << oldValue << " to " << port->getValue());
    } else {
        SlaveVirtualDevice *dev = port->getDevice();

        if (dev->getBus() == nullptr) { // virtual device port
            devId = dev2devId(unitId, dev);
            portId = ordinal2portId(port->getIdOnDevice());
            DEBUG("dev port value changed on unit " << unit->getAddress() << " pin=" << (int)port->getPin() << " port=" << (int)port->getIdOnDevice() << " from " << oldValue << " to " << port->getValue());
        } else { // bus device port
            devId = longaddr2devId(unitId, dev->getLongAddress());
            portId = ordinal2portId(port->getIdOnDevice());
            DEBUG("bus dev port value changed on unit " << unit->getAddress() << " pin=" << (int)port->getPin() << " longaddr=0x" << hex << dev->getLongAddress() << dec << " port=" << (int)port->getIdOnDevice() << " from " << oldValue << " to " << port->getValue());
        }
    }

    _valueCallback(_driverBase, devId, portId, port->getValue());
}

vector<string> split(string str, char delimiter)
{
    vector<string> tokens;
    stringstream buf;
    string token;

    for (string::iterator it = str.begin();it != str.end();it++)
    {
        if (*it == delimiter)
        {
            tokens.push_back(buf.str());
            buf.str(string());
        }
        else
            buf << *it;
    }

    string last = buf.str();
    if (last.size() > 0)
        tokens.push_back(last);

    return tokens;
}

void busdev2idev(IDevice &d, SlaveVirtualDevice *dev, int unitId) {
    stringstream ss;
    d.cls = DeviceClass::SENSOR;
    d.subCls = DeviceSubclass::THERMOMETER;

    uint64_t longaddr = dev->getLongAddress();
    d.id = longaddr2devId(unitId, longaddr);
    d.parent = unitId;
    ss << "bd" << "." << hex << toSimpleW1Addr(longaddr);
    d.code = ss.str();
    d.config = "therm_ab";

    DEBUG("busdev2idev dev " << d.code << " id=" << d.id << " parent=" << d.parent);
}

void vdev2idev(IDevice &d, SlaveVirtualDevice *dev) {
    stringstream ss;
    d.cls = abusDevType2DevCls(dev->getType());
    d.subCls = DeviceSubclass::GENERIC;
    d.id = dev->getPermanentId();
    uint8_t unitId = dev->getUnit()->getAddress();
    d.parent = unitId;
    ss << "d" << (int)unitId << "." << (int)dev->getPin();
    d.code = ss.str();
    d.config = "slave";
    d.internalId = dev->getId();
    d.internalType = static_cast<uint8_t>(dev->getType());

    DEBUG("vdev2idev dev " << d.code << " id=" << d.id << " parent=" << d.parent);
}

void unit2idev(IDevice &d, AndamBusSlave *slave) {
    stringstream ss;
    d.cls = DeviceClass::MULTIDEVICE_CONTROL;
    d.subCls = DeviceSubclass::GENERIC;
    d.id = slave->getAddress();
    d.parent = 0;
    ss << "ab" << d.id;
    d.code = ss.str();
    d.config = "slave";

    DEBUG("slave2idev dev " << d.code << " id=" << d.id << " parent=" << d.parent);
}

void vport2iport(IPort &p, SlaveVirtualPort *abPort, int deviceId, uint8_t unitId) {
    stringstream ss;
    if (abPort == nullptr)
        return;

    if (abPort->getDevice() == nullptr)
        ss << "p" << (int)abPort->getPin();
    else
        ss << "p" << (int)abPort->getIdOnDevice();

//    SlaveVirtualDevice *dev = abPort->getDevice();
    p.code = ss.str().c_str();
    p.dataType = DataType::NONE;
    p.deviceId = deviceId;
    p.id = port2Id(unitId, abPort);
    VirtualPortType pt = abPort->getType();
    if (pt == VirtualPortType::ANALOG_INPUT || pt == VirtualPortType::ANALOG_OUTPUT || pt == VirtualPortType::ANALOG_OUTPUT_PWM) {
        p.type = PortType::ANALOG;
        p.aValue = abPort->getValue();
        DEBUG("value=" << p.aValue);
    }
    else {
        p.type = PortType::DIGITAL;
        p.dValue = abPort->getValue()!=0;
    }


    if (pt == VirtualPortType::DIGITAL_OUTPUT || pt == VirtualPortType::DIGITAL_OUTPUT_REVERSED || pt == VirtualPortType::ANALOG_OUTPUT || pt == VirtualPortType::ANALOG_OUTPUT_PWM)
        p.direction = PortDirection::OUTPUT;
    else
        p.direction = PortDirection::INPUT;

    DEBUG("vport2iport " << p.code << " id=" << p.id << " deviceId=" << p.deviceId);
}

void getSlaveDevicePorts(vector<IPort> &iports, int deviceId) {

    int slaveId = devId2unit(deviceId);

    DEBUG("getSlaveDevicePorts " << deviceId << " " << slaveId);

    AndamBusSlave *slave = abm->findSlaveByAddress(slaveId);
    if (slave != nullptr) {
        vector<SlaveVirtualPort*> ports;

        if (devIdIsBusDevice(deviceId)) {
            DEBUG("bus dev port");
            uint64_t la = devId2longaddr(deviceId);

            INFO("la=0x" << hex << la << dec << " ");
            ports = slave->getVirtualDevicePortsByLongAddress(la);
        } else {
 //           cout << "getSlaveDevPort" << endl;
            int vdevPin = devId2pin(deviceId);
            ports = slave->getVirtualDevicePortsByPin(vdevPin);
        }

        DEBUG("ports on dev " << deviceId << " " << ports.size());

        for (auto it=ports.begin();it!=ports.end();it++) {
            IPort p = {};
            SlaveVirtualPort *abPort = *it;

            if (abPort->getId() <= 0)
                continue;

            vport2iport(p, abPort, deviceId, slave->getAddress());

            DEBUG("creating dev port " << p.code << " deviceId=" << p.deviceId << " id=" << p.id);
            iports.push_back(p);
        }
    } else {
        DEBUG("slave not found" << slaveId);
    }
}

void getSlavePorts(vector<IPort> &iports, int deviceId) {
    DEBUG("getSlavePorts " << deviceId);
    AndamBusSlave *slave = abm->findSlaveByAddress(deviceId);

    if (slave != nullptr) {
        vector<SlaveVirtualPort*> &ports = slave->getPorts();

//        WARNING("slave ports " << ports.size());
//        WARNING("slave devices " << slave->getDevices().size());

        for (auto it=ports.begin();it!=ports.end();it++) {
            IPort p = {};

            SlaveVirtualPort *abPort = *it;
            if (abPort->getDevice() != nullptr || abPort->getId() == 0)
                continue;

            vport2iport(p, abPort, deviceId, slave->getAddress());

            WARNING("port " << p.code);
            iports.push_back(p);
        }
    } else {
        WARNING("slave not found" << deviceId);
    }
}

/*bool getPortValue(int devId, int portId, int &value) {
    uint8_t slaveId;

    slaveId = toSlaveId(devId);

    WARNING("getPortVal " << (int)slaveId << "," << portId);
    AndamBusSlave *slave = abm->findSlaveByAddress(slaveId);

    if (slave == nullptr)
        return false;

    SlaveVirtualPort *port = slave->getPort(portId);
    if (port == nullptr)
        return false;
    value = port->getValue();
    return true;
}*/

bool setPortValue(int devId, int portId, int value) {
    uint8_t unitId;

/*    if (devId < ANDAMBUS_UNIT_PREFIX)
        slaveId = devId;
    else
        slaveId = devId/ANDAMBUS_UNIT_PREFIX;*/
    unitId = devId2unit(devId);

    DEBUG("setPortVal " << (int)unitId << "," << portId << " to " << value);
    AndamBusSlave *slave = abm->findSlaveByAddress(unitId);

    if (slave == nullptr)
        return false;

    try {
        if (devId < ANDAMBUS_UNIT_PREFIX) { //  virtual ports
            slave->setPortValue(portId, value);
            return true;
        }
        if (devId >= ANDAMBUS_UNIT_PREFIX && devId < 10000) { //  virtual device ports
            uint8_t pin = devId % ANDAMBUS_UNIT_PREFIX;
            SlaveVirtualPort *port = slave->getVirtualPortByPinOrdinal(pin, portId-1);

            if (port != nullptr) {
                slave->setPortValue(port->getId(), value);
                return true;
            }
        }
    } catch (ExceptionBase &ex) {
        ERROR("setPortValue:" << ex.what());
        return false;
    }

    return false;
}

extern "C"
{
    struct IDriver getDriverInfo(struct server_info *si)
    {
        IDriver drvInfo = {0};
        drvInfo.apiVersionMajor = PLUGIN_API_VERSION_MAJOR;
        drvInfo.apiVersionMinor = PLUGIN_API_VERSION_MINOR;
        drvInfo.pluginVersion = DRIVER_VERSION;
        drvInfo.isReal = true;
        drvInfo.refreshType = RefreshType::Interrupt;
        drvInfo.refreshPeriodMs = 3000;
        drvInfo.timeLimitGetValue = 500;
        drvInfo.timeLimitSetValue = 200;
        drvInfo.timeLimitRefresh = 1000;
        drvInfo.timeLimitDetect = 1000;
        drvInfo.irq_gpio = 11;
        return drvInfo;
    }

    bool init()
    {
        try {
            BroadcastSocket *bs = new SerialBroadcastSocket(SERIAL_DEV_FILENAME);
            abm = new AndamBusMaster(*bs);

            abm->detectSlaves();
            abm->refreshSlaves();

//            setLogLevel(LogLevel::DEBUG);
            return true;
        } catch (ExceptionBase &ex) {
            ERROR("Cannot initialize driver:" << ex.what());
            return false;
        }
    }

    vector<IDevice> getDevices() {
        vector<IDevice> dev;

        if (abm != nullptr)
            try {
                vector<AndamBusSlave*> &slaves = abm->getSlaves();

                for(auto it=slaves.begin(); it!= slaves.end();it++) {
                    AndamBusSlave *slave = *it;

                    #ifdef TEST_UNIT_ADDRESS
                    if (slave->getAddress() != TEST_UNIT_ADDRESS)
                        continue;
                    #endif

                    IDevice d;
                    unit2idev(d, slave);

                    DEBUG("slave " << d.code);
                    dev.push_back(d);

                    vector<SlaveVirtualDevice*> &devs = slave->getDevices();

                    for(auto itd=devs.begin(); itd!= devs.end();itd++) {
                        SlaveVirtualDevice *vdev = *itd;

                        IDevice d = {0};
                        if (vdev->getBus() == nullptr)
                            vdev2idev(d, vdev);
                        else
                            busdev2idev(d, vdev, slave->getAddress());

                        DEBUG("creating dev " << d.code << " id=" << d.id << " parent=" << d.parent);
                        dev.push_back(d);

                    }

                    slave->setChangeListener(&vclist);
                }

            } catch (ExceptionBase &ex) {
                ERROR("getDevices:" << ex.what());
            }

        return dev;
    }

    bool configDevice(int devId, const string name, const string value) {
        DEBUG("configDevice " << devId << " " << name << ":" << value);


        SlaveVirtualDevice *dev = devId2dev(devId);

        if (dev == nullptr) {
            WARNING("device not found " << devId);
            return false;
        }

        uint8_t unitId = devId2unit(devId);

        if (unitId < 1 || unitId > ANDAMBUS_ADDRESS_MAX) {
            WARNING("Andam unit ID invalid " << (int)unitId);
            return false;
        }

        AndamBusSlave *unit = abm->findSlaveByAddress(unitId);

        if (unit==nullptr) {
            WARNING("Andam unit not found " << (int)unitId);
            return false;
        }

        AndamBusPropertyType tp;
        uint8_t propId;

        ConfigName2AndambusPropertyType(tp, propId, name);

        if (tp == AndamBusPropertyType::NONE) {
            WARNING("not setting property for " << name);
            return false;
        }

        try {
            int ivalue = stoi(value);

            unit->doPropertySet(dev->getId(), tp, ivalue, propId);

            WARNING("setting on dev=" << (int)dev->getId() << " tp:" << (int)tp << " propid=" << (int)propId << " value=" << ivalue);
            return true;
        } catch (invalid_argument &e) {
            WARNING("invalid property integer value " << value);
            return false;
        }
    }


    vector<IConfigItem> getConfigDevice(int devId) {
        vector<IConfigItem> cfg;

        SlaveVirtualDevice *dev = devId2dev(devId);

        if (dev != nullptr)  {
            map<AndamBusPropertyType,map<uint8_t,int32_t>> &props = dev->getProperties();

            for(auto it=props.begin();it!=props.end();it++) {
                AndamBusPropertyType type = it->first;
                map<uint8_t,int32_t> &vals = it->second;

                for (auto itp = vals.begin();itp!=vals.end();itp++) {
                    string name = AndambusPropertyType2ConfigName(type, itp->first);

                    if (name.length()>0) {
                        IConfigItem ci = {.name= name, .value= to_string(itp->second) };

//                        WARNING("adding dev cfg " << name << ":" << itp->second );
                        cfg.push_back(ci);
                    }
                }
            }
        }

//        WARNING("getConfigDevice " << devId << " " << cfg.size());
        return cfg;
    }

    vector<IPort> getPorts(int deviceId)
    {
        vector<IPort> iports;

 //       DEBUG("getPorts " << deviceId);

        if (abm != nullptr)
            try {
                if (deviceId < ANDAMBUS_UNIT_PREFIX)
                    getSlavePorts(iports, deviceId);
                else
                    getSlaveDevicePorts(iports, deviceId);

            } catch (ExceptionBase &ex) {
                ERROR("getPorts:" << ex.what());
            }

//        INFO("getPorts iports=" << iports.size());
        return iports;
    }

    bool firstRefresh = true;

    bool refreshDevice() {
//        cout << "refreshDevice" << endl;
        DEBUG("refreshDevice");

        if (abm != nullptr)
            try {
                abm->refreshPortValues(false);

                if (refreshCounter++ % 100 == 0) {
                    vector<AndamBusSlave*> &units = abm->getSlaves();
                    for (auto it=units.begin();it<units.end();it++) {
                        AndamBusSlave *unit = (*it);
                        unit->refreshMetadata();

                        if (firstRefresh) {
                            _metadataCallback(_driverBase, unit2devId(unit->getAddress()), DeviceMetadataType::NetApiVersion, 0, unit->getApiVersion() * 0x10000 );
                            _metadataCallback(_driverBase, unit2devId(unit->getAddress()), DeviceMetadataType::SwVersion, 0, unit->getSWVersion() * 0x10000 );
                            _metadataCallback(_driverBase, unit2devId(unit->getAddress()), DeviceMetadataType::HwType, 0, (int)unit->getHwType() );
                        }

                    }
                    if (firstRefresh)
                        firstRefresh=false;
                }


/*                if (_valueCallback != nullptr) {
                    abm->getSlaves()
                }*/

            } catch (ExceptionBase &ex) {
                ERROR("refreshDevice:" << ex.what());
            } catch (exception &ex) {
                ERROR("refreshDevice ex:" << ex.what());
            } catch (...) {
                FATAL("Fatal ex");
            }

        return true;
    }

/*    bool getPortD(int devId, int portId, bool &value) {
        int val;
        bool ret = getPortValue(devId, portId, val);
        value = val!=0;
        return ret;
    }

    bool getPortA(int devId, int portId, int &value) {
        return getPortValue(devId, portId, value);
    }*/

    bool setPortD(int devId, int portId, bool value) {
        return setPortValue(devId, portId, value?1:0);
    }

    bool setPortA(int devId, int portId, int value) {
/*        void *array[30];
        size_t size;
        size = backtrace(array, 30);
        backtrace_symbols_fd(array, size, STDERR_FILENO);*/
//        abort();
        return setPortValue(devId, portId, value);
    }

    const char* getIconFile(int devId)
    {
        if (devId < ANDAMBUS_UNIT_PREFIX)
            return ANDAMBUS_UNIT_ICON_CODE;
/*        if (devIdIsBusDevice(devId))
            return ANDAMBUS_SENSOR_ICON_CODE;*/

        SlaveVirtualDevice *dev = devId2dev(devId);
        if (dev != nullptr) {
            return devType2IconCode(dev->getType()).c_str();
        }
        return ANDAMBUS_GENERIC_ICON_CODE;
    }

    void cleanup() {
        if (abm != nullptr)
            delete abm;
    }

/*    void setValueCallback(t_valueCallback callback)
    {
        _valueCallback = callback;
    }

    void setDriverBaseAddress(void* drvInstance)
    {
        _driverBase = drvInstance;
    }*/

}


uint64_t toSimpleW1Addr(uint64_t addr) {
    return (addr>>8)&0xffffffffffff;
}

/*uint64_t dev2longaddr(SlaveVirtualDevice *vdev) {
    uint32_t ad_lo = vdev->getPropertyValue(AndamBusPropertyType::LONG_ADDRESS, 0),
        ad_hi = vdev->getPropertyValue(AndamBusPropertyType::LONG_ADDRESS, 1);

    return //htonl(ad_hi) * 0x100000000 + htonl(ad_lo);
        ad_hi * 0x100000000ll + ad_lo;
}*/

int unit2devId(uint8_t unitID) {
    assert(unitID<16);
    return unitID;
}

SlaveVirtualDevice* devId2dev(int devId) {
    if (devId < ANDAMBUS_UNIT_PREFIX)
        return nullptr;

//    cout << "devId2dev" << endl;
    uint8_t pin = devId2pin(devId);
    uint8_t unitId = devId2unit(devId);

    AndamBusSlave *unit = abm->findSlaveByAddress(unitId);

    if (unit == nullptr)
        return nullptr;
    return unit->getVirtualDeviceByPin(pin);
}

uint8_t devId2unit(int devId) {
    uint8_t unitId = devId < ANDAMBUS_UNIT_PREFIX?devId:devId / ANDAMBUS_UNIT_PREFIX;
    assert(unitId > 0 && unitId < 16);

    return unitId;
}

int pin2id(uint8_t unitID, uint8_t pin) {
    assert(unitID<16);
    assert(pin<ANDAMBUS_BUSDEV_START);
    return pin + unitID * ANDAMBUS_UNIT_PREFIX;
}

uint8_t devId2pin(int devId) {
//    cout << "devId:" << devId << endl;
//    assert(devId%ANDAMBUS_UNIT_PREFIX<ANDAMBUS_BUSDEV_START);
    return devId%ANDAMBUS_UNIT_PREFIX;
}

uint8_t portId2pin(int portId) {
//    cout << "portId2pin" << endl;
    return devId2pin(portId);
}

int ordinal2portId(uint8_t ordinal) {
    assert(ordinal<ANDAMBUS_MAX_DEVPINS);
    return ordinal+1;
}

uint8_t portId2ordinal(int portID) {
    assert(portID - 1<ANDAMBUS_MAX_DEVPINS);
    return portID - 1;
}

int longaddr2devId(uint8_t unitID, uint64_t longaddr) {
    unsigned int idx;

    auto it = find(w1addrs.begin(), w1addrs.end(), longaddr);
    if (it == w1addrs.end()) { // address not found
        idx = w1addrs.size();
        w1addrs.push_back(longaddr);
    } else {
        idx = distance(w1addrs.begin(), it);
    }

    assert(idx < w1addrs.size());
    return unitID * ANDAMBUS_UNIT_PREFIX + idx + ANDAMBUS_BUSDEV_START;
}


uint64_t devId2longaddr(int devId) {
    int tmp_id = devId%ANDAMBUS_UNIT_PREFIX;
    assert(tmp_id >= ANDAMBUS_BUSDEV_START);

    unsigned int idx = tmp_id-ANDAMBUS_BUSDEV_START;
    assert(idx < w1addrs.size());

    if (idx >= w1addrs.size())
        return 0;

    return w1addrs[idx];
}

bool devIdIsBusDevice(int devId) {
    return devId%ANDAMBUS_UNIT_PREFIX >= ANDAMBUS_BUSDEV_START;
}

int dev2devId(uint8_t unitId, SlaveVirtualDevice *dev) {
    if (dev == nullptr)
        return 0;

    if (dev->getBus() == nullptr) // virtual device -> device = virtual device
        return pin2id(unitId, dev->getPin());

    uint64_t la = dev->getLongAddress();

    return longaddr2devId(unitId, la);
}

int port2Id(uint8_t unitId, SlaveVirtualPort *port) {
    if (port == nullptr)
        return 0;
    if (port->getDevice() == nullptr) // pin -> device = Slave
        return pin2id(unitId, port->getPin());
//    SlaveVirtualDevice *dev = port->getDevice();

    DEBUG("getIdOnDevice=" << (int)port->getIdOnDevice());
    return ordinal2portId(port->getIdOnDevice());
}

int port2portId(uint8_t unitId, SlaveVirtualPort *port) {
    if (port == nullptr)
        return 0;
    if (port->getDevice() == nullptr) // pin -> device = Slave
        return pin2id(unitId, port->getPin());

    return ordinal2portId(port->getIdOnDevice()); // virtual dev port or bus dev port
}

const std::string devType2IconCode(VirtualDeviceType type) {
    switch (type) {
    case VirtualDeviceType::PUSH_DETECTOR:
        return ANDAMBUS_CLICK_ICON_CODE;
    case VirtualDeviceType::TIMER:
        return ANDAMBUS_TIMER_ICON_CODE;
    case VirtualDeviceType::THERMOMETER:
        return ANDAMBUS_SENSOR_ICON_CODE;
    case VirtualDeviceType::THERMOSTAT:
        return ANDAMBUS_THERMOSTAT_ICON_CODE;
    case VirtualDeviceType::BLINDS_CONTROL:
        return ANDAMBUS_BLINDS_CONTROL_ICON_CODE;
    case VirtualDeviceType::DIFF_THERMOSTAT:
        return ANDAMBUS_DIFF_THERMOSTAT_ICON_CODE;
    case VirtualDeviceType::CIRC_PUMP:
        return ANDAMBUS_CIRC_PUMP_ICON_CODE;
    case VirtualDeviceType::SOLAR_THERMOSTAT:
        return ANDAMBUS_SOLAR_THERMOSTAT_ICON_CODE;
    case VirtualDeviceType::DIGITAL_POTENTIOMETER:
        return ANDAMBUS_DIGITAL_POTENTIOMETER_ICON_CODE;
    case VirtualDeviceType::I2C_DISPLAY:
        return ANDAMBUS_I2C_DISPLAY_ICON_CODE;
    case VirtualDeviceType::HVAC:
        return ANDAMBUS_HVAC_ICON_CODE;
    case VirtualDeviceType::MODIFIER:
        return ANDAMBUS_MODIFIER_ICON_CODE;
    case VirtualDeviceType::NONE:
        return ANDAMBUS_GENERIC_ICON_CODE;
    }

    return ANDAMBUS_GENERIC_ICON_CODE;
}

/*
    FREE_MEMORY = 0x60, // returns number of free bytes on unit
    UPTIME = 0x61,     // returns uptime in seconds
    TICK = 0x62,     // returns ticks/iterations from start
	MAX_CYCLE_DURATION = 0x63, // returns main cycle work duration maximum is ms
	CYCLE_DURATION_RANGE = 0x64, // returns main cycle duration range counts 0: 10ms, 1:30ms, 2: 50ms, 3: 100ms, 4: 150ms, 5: 200ms

    ERROR_COUNT_CRC = 0x80,  // returns CRC errors count
    ERROR_COUNT_SYNC = 0x81,  // returns sync lost errors count
    ERROR_COUNT_LONG = 0x82,  // returns packet too long errors count


enum class DeviceMetadataType:unsigned char {
    Started='S', // time of start in seconds from 2020-01-01
    SwVersion='V',
    NetApiVersion='N',
    ThreadCount='T',
    IrqCount='I',
    RefreshTryCount='C',
    FreeMemory='M'
};


*/

DeviceClass abusDevType2DevCls(VirtualDeviceType tp)
{
    switch (tp) {
    case VirtualDeviceType::NONE:
        return DeviceClass::GENERIC;
    case VirtualDeviceType::THERMOMETER:
        return DeviceClass::SENSOR;
    case VirtualDeviceType::THERMOSTAT:
        return DeviceClass::HEATING_CONTROL;
    case VirtualDeviceType::BLINDS_CONTROL:
        return DeviceClass::BLINDSCONTROL;
    case VirtualDeviceType::PUSH_DETECTOR:
        return DeviceClass::DETECTOR;
    case VirtualDeviceType::DIFF_THERMOSTAT:
        return DeviceClass::HEATING_CONTROL;
    case VirtualDeviceType::CIRC_PUMP:
        return DeviceClass::HEATING_CONTROL;
    case VirtualDeviceType::SOLAR_THERMOSTAT:
        return DeviceClass::HEATING_CONTROL;
    case VirtualDeviceType::DIGITAL_POTENTIOMETER:
        return DeviceClass::GENERIC;
    case VirtualDeviceType::I2C_DISPLAY:
        return DeviceClass::DISPLAY;
	case VirtualDeviceType::HVAC:
        return DeviceClass::HEATING_CONTROL;
    case VirtualDeviceType::TIMER:
        return DeviceClass::TIMER;
	case VirtualDeviceType::MODIFIER:
        return DeviceClass::REMOTE;
    }

    return DeviceClass::GENERIC;
}

bool convertMetadataType(MetadataType type, DeviceMetadataType &dtype, int value, int &dvalue) {
    dvalue = value;
    switch(type) {
    case MetadataType::FREE_MEMORY:
        dtype = DeviceMetadataType::FreeMemory;
        return true;
    case MetadataType::TICK:
        dtype = DeviceMetadataType::TickCount;
        return true;
    case MetadataType::ERROR_COUNT_CRC:
        dtype = DeviceMetadataType::ErrorCount;
        return true;
    case MetadataType::UPTIME:
        dtype = DeviceMetadataType::Started;
        dvalue = time(0)-value;
        return true;
    case MetadataType::TRY_COUNT:
        dtype = DeviceMetadataType::ErrorCount;
        return true;
    default:
        return false;
    }
}

void ConfigName2AndambusPropertyType(AndamBusPropertyType &tp, uint8_t &propId, string configName) {
    tp = AndamBusPropertyType::NONE;

    if (configName.substr(0, sizeof(ANDAMBUS_PROPERTY_PREFIX)-1) != ANDAMBUS_PROPERTY_PREFIX) {
        WARNING("not ab prefix");
        return;
    }

    string abname = configName.substr(sizeof(ANDAMBUS_PROPERTY_PREFIX)-1);
    vector<string> namecomp = split(abname, ":");

    string propname, spropid;
    if (namecomp.size() > 1) {
        spropid = namecomp.at(0);
        propname = namecomp.at(1);
    } else {
        spropid = "0";
        propname = namecomp.at(0);
    }

    try {
        propId = stoi(spropid);
    } catch (invalid_argument &e) {
        WARNING("invalid property integer value " << (int)propId);
        return;
    }

    if (propname == "temp")
        tp = AndamBusPropertyType::TEMPERATURE;
    if (propname == "p_sec")
        tp = AndamBusPropertyType::PERIOD_SEC;
    if (propname == "p_ms")
        tp = AndamBusPropertyType::PERIOD_MS;

}

string AndambusPropertyType2ConfigName(AndamBusPropertyType tp, uint8_t propId) {
    switch (tp) {
    case AndamBusPropertyType::TEMPERATURE:
        return string(ANDAMBUS_PROPERTY_PREFIX) + to_string(propId) + ":temp";
    case AndamBusPropertyType::PERIOD_SEC:
        return string(ANDAMBUS_PROPERTY_PREFIX) + to_string(propId) + ":p_sec";
    case AndamBusPropertyType::PERIOD_MS:
        return string(ANDAMBUS_PROPERTY_PREFIX) + to_string(propId) + ":p_ms";

    default:
        return "";
    }
}
