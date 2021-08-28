
#include "icontrolDriver.h"

using namespace std;

t_logCallback _logCallback = nullptr;
void* _driverBase = nullptr;
t_valueCallback _valueCallback=nullptr;
t_metadataCallback _metadataCallback=nullptr;

int main(int argc, char *argv[]) {
    SlaveVirtualPort port(2, VirtualPortType::DIGITAL_INPUT, nullptr, 36, 1);

    uint8_t unitId = 3;

    cout << "simple pin - calculate portID, devId = unitID" << endl;

    cout << "unit devId=" << (int)unit2devId(unitId) << endl;
    cout << "portID(3) -> unit=" << (int)devId2unit(3) << endl;

    cout << "portID(unit=3,pin=36) =" << port2Id(unitId, &port) << endl;
    cout << "portID 804 -> pin=" << (int)portId2pin(804) << endl;
    cout << "portID 804 -> unit=" << (int)devId2unit(804) << endl;

    cout << "Virtual device - calculate devID, parentID" << endl;

    SlaveVirtualDevice dev(10, 52, nullptr, VirtualDeviceType::NONE, 0);


    cout << "deviceID (unit=3,pin=52)=" << dev2devId(unitId, &dev) << endl;
    cout << "parentID (unit=3)=" << (int)unitId << endl;

    cout << "deviceID 820 -> pin=" << (int)devId2pin(820) << endl;
    cout << "deviceID 820 -> unit=" << (int)devId2unit(820) << endl;

    cout << "Virtual device port - calculate portID,devID" << endl;
    SlaveVirtualPort dport(11, VirtualPortType::DIGITAL_INPUT, &dev, 52, 1);

    cout << "deviceID (unit 3, pin 52)=" << dev2devId(unitId, &dev) << endl;
    cout << "portID (unit 3, pin 52, ordinal 1)=" << port2Id(unitId, &dport) << endl;

    cout << "deviceID 820 -> pin=" << (int)devId2pin(820) << endl;
    cout << "portId 2 -> ordinal=" << (int)portId2ordinal(2) << endl;

    cout << "bus device - calculate devID, parentID" << endl;

    SlaveSecondaryBus bus(20, 19, nullptr);
    SlaveVirtualDevice bdev(21, 19, &bus, VirtualDeviceType::NONE, 0);
    SlaveVirtualPort bdport(22, VirtualPortType::DIGITAL_INPUT, &dev, 19, 1);

    ItemProperty prop0{AndamBusPropertyType::LONG_ADDRESS,0,0,{.value=0x11223344}}, // low addr
        prop1{AndamBusPropertyType::LONG_ADDRESS,0,1,{.value=0x56}}; // hi addr
    bdev.setProperty(prop0);
    bdev.setProperty(prop1);

    int devId = dev2devId(unitId, &bdev);
    cout << "deviceID (unit 3, pin 19)=" << devId << endl;
    cout << "isBus device (" << devId<<")=" << devIdIsBusDevice(devId) << endl;
    cout << "portID (unit 3, pin 52, ordinal 1)=" << port2Id(unitId, &bdport) << endl;

    cout << "deviceID "<<devId<<" -> longaddr=0x" << hex<< devId2longaddr(devId) << endl;
    cout << "portId 2 -> ordinal=" << (int)portId2ordinal(2) << endl;

}
