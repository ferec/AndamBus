#include <iostream>
#include <chrono>

#include "shared/AndamBusExceptions.h"
#include "AndamBusMaster.h"
#include "AndamBusSlave.h"

#include "SerialBroadcastSocket.h"
#include "TestBroadcastSocket.h"
#include "LocalBroadcastSocket.h"

#include "thread"

#include <fcntl.h>
#include <unistd.h>

using namespace std;

auto t_start = chrono::system_clock::now();

void testLogger(LogLevel level, const string &msg) {
    auto now = chrono::system_clock::now();
    cout << chrono::duration_cast<chrono::milliseconds>(now - t_start).count() << " " << LogLevelString(level) << ":" << msg << endl;
}

void test1();

int main() {
    thread tTest(test1);

    tTest.join();

    return 0;
}

void test1() {

    setLogLevel(LogLevel::INFO);
    setDebugCallback(testLogger);

    try {
//        SerialBroadcastSocket bs("/dev/ttyUSB0");
//        TestBroadcastSocket bs;
        LocalBroadcastSocket bs;

        AndamBusMaster master(bs);

//        master.createVirtualPort(3, 13, VirtualPortType::DIGITAL_OUTPUT);
//        master.setVirtualPortValue(3, 13, 5);

        int i=1;
        while (master.getSlaves().size()==0 && i-->0) {
            AB_DEBUG("Detection start");
            master.detectSlaves();
            AB_DEBUG("Detection end");
            this_thread::sleep_for(chrono::milliseconds(500));
            AB_DEBUG("Sleep end");
        }

        master.refreshSlaves();

        vector<AndamBusSlave*> slaves = master.getSlaves();

        for (auto slit= slaves.begin(); slit!=slaves.end();slit++) {
            AndamBusSlave *slave = *slit;

            AB_INFO("Slave " << slave->getAddress() << " type:" << slave->getHwTypeString() << ", sw version:" << slave->getSWVersionString() << ", api version:" << slave->getApiVersionString());

            map<AndamBusPropertyType,map<uint8_t,int>> props = slave->getProperties();

            if (props.size() > 0)
                AB_INFO("Slave properties:");

            for (auto pit=props.begin();pit!=props.end();pit++) {
                AB_INFO(propertyTypeString(pit->first) << " = " << pit->second.size());
            }

            vector<SlaveSecondaryBus*> &buses = slave->getSecondaryBuses();

            for (auto it=buses.begin();it != buses.end(); it++) {
                SlaveSecondaryBus *bus = *it;
                AB_INFO("secondary bus with ID " << dec << (int)bus->getId() << " on pin " << (int)bus->getPin());

                props = bus->getProperties();
                if (props.size() > 0)
                    AB_INFO("Bus properties:");
                for (auto pit=props.begin();pit!=props.end();pit++) {
                    AB_INFO(propertyTypeString(pit->first) << " = " << pit->second.size());
                }
            }

            vector<SlaveVirtualDevice*> &devs = slave->getVirtualDevices();

            for (auto it=devs.begin();it != devs.end(); it++) {
                SlaveVirtualDevice *dev = *it;
                SlaveSecondaryBus *bus = dev->getBus();
                AB_INFO("virtual dev ID " << dec << (int)dev->getId() << " of type " << SlaveVirtualDevice::getTypeString(dev->getType()) << " on bus " << (bus!=nullptr?(bus->getId()):0));

                props = dev->getProperties();
                if (props.size() > 0)
                    AB_INFO("Virtual dev properties:");
                for (auto pit=props.begin();pit!=props.end();pit++) {
                    AB_INFO(propertyTypeString(pit->first) << " = " << pit->second.size());
                }
            }

            vector<SlaveVirtualPort*> &ports = slave->getVirtualPorts();

            for (auto it = ports.begin(); it != ports.end(); it++) {
                SlaveVirtualPort *port = *it;
                SlaveVirtualDevice *dev = port->getDevice();
                AB_INFO("virtual ports with ID " << dec << (int)port->getId() << " type " << virtualPortTypeString(port->getType()) << " on device " << (int)(dev!=nullptr?dev->getId():0) << " value=" << port->getValue());

                props = port->getProperties();
                if (props.size() > 0)
                    AB_INFO("Virtual dev properties:");
                for (auto pit=props.begin();pit!=props.end();pit++) {
                    AB_INFO(propertyTypeString(pit->first) << " = " << pit->second.size());
                }
            }

            master.refreshPortValues(true);

            for (auto it = ports.begin(); it != ports.end(); it++) {
                SlaveVirtualPort *port = *it;
                AB_INFO("virtual ports " << dec << (int)port->getId() << " value=" << port->getValue());

                slave->setPortValue(port, port->getValue()+5);
            }

            slave->refreshPortValues(true);

            AB_INFO("virtual ports after refresh");
            for (auto it = ports.begin(); it != ports.end(); it++) {
                SlaveVirtualPort *port = *it;
                AB_INFO("virtual ports " << dec << (int)port->getId() << " value=" << port->getValue());
            }

            SlaveVirtualPort *vp = nullptr;
            try {
                vp = slave->createVirtualPort(13, VirtualPortType::DIGITAL_OUTPUT);

//                master.refreshVirtualPorts(slave);

                slave->setPortValue(vp, 0);

                AB_INFO("virtual ports after create");
                for (auto it = ports.begin(); it != ports.end(); it++) {
                    SlaveVirtualPort *port = *it;
                    AB_INFO("virtual ports " << dec << (int)port->getId() << " value=" << port->getValue());
                }

//                slave->removeVirtualPort(vp.id);
            } catch (ExceptionBase &e) {
                AB_ERROR("Error " << e.what());
            }

            this_thread::sleep_for(chrono::milliseconds(100));


            if (vp!=nullptr) {
                AB_INFO("turn off LED");
                slave->setPortValue(vp, 0);
            }
//            master.setVirtualPortValue(3, vp.id, 0);

            this_thread::sleep_for(chrono::milliseconds(100));

            AB_INFO("refresh");
            slave->refreshPortValues(true);
//            master.refreshVirtualPorts(slave);

//            master.setVirtualPortValue(3, vp.id, 1);
            if (vp!=nullptr) {
                AB_INFO("turn on LED");
                slave->setPortValue(vp, 1);
            }

            AB_INFO("virtual ports after remove");
            for (auto it = ports.begin(); it != ports.end(); it++) {
                SlaveVirtualPort *port = *it;
                AB_INFO("virtual ports " << dec << (int)port->getId() << " value=" << port->getValue());
            }

            SecondaryBus sb = slave->createSecondaryBus(2);

            slave->secondaryBusDetect(sb.id);

            slave->refreshVirtualItems();

            slave->secondaryBusRunCommand(sb.id, OnewireBusCommand::CONVERT);

            slave->removeSecondaryBus(sb.id);

            SlaveVirtualDevice *dev = slave->createVirtualDevice(VirtualDeviceType::THERMOSTAT, 7);

            if (dev!= nullptr)
                slave->removeVirtualDevice(dev->getId());

        }

    } catch (ExceptionBase &e) {
        AB_ERROR("Error:" << e.what());
    } catch (exception &e) {
        AB_ERROR("Error:" << e.what());
    }

    AB_INFO("end");
}
