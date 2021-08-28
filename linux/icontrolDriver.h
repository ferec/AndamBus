#ifndef ICONTROLDRIVER_H_INCLUDED
#define ICONTROLDRIVER_H_INCLUDED

#include "../iControl/include/driver.h"
#include "AndamBusMaster.h"

#define ANDAMBUS_GENERIC_ICON_CODE "abus"
#define ANDAMBUS_SENSOR_ICON_CODE "therm_ab"
#define ANDAMBUS_CLICK_ICON_CODE "click"
#define ANDAMBUS_TIMER_ICON_CODE "timer"
#define ANDAMBUS_UNIT_ICON_CODE "unit"
#define ANDAMBUS_THERMOSTAT_ICON_CODE "ths"
#define ANDAMBUS_BLINDS_CONTROL_ICON_CODE "blinds"
#define ANDAMBUS_DIFF_THERMOSTAT_ICON_CODE "diffths"
#define ANDAMBUS_CIRC_PUMP_ICON_CODE "circ_pump"
#define ANDAMBUS_SOLAR_THERMOSTAT_ICON_CODE "solar_ths"
#define ANDAMBUS_DIGITAL_POTENTIOMETER_ICON_CODE "digipot"
#define ANDAMBUS_I2C_DISPLAY_ICON_CODE "disp"
#define ANDAMBUS_HVAC_ICON_CODE "hvac"
#define ANDAMBUS_MODIFIER_ICON_CODE "modifier"


#define ANDAMBUS_UNIT_PREFIX 0x100
#define ANDAMBUS_BUSDEV_START 0x80

//#define TEST_UNIT_ADDRESS 3

#define ANDAMBUS_MAX_DEVPINS 0x16

#define DRIVER_VERSION 1
#define SERIAL_DEV_FILENAME "/dev/ttyUSB0"

#define ANDAMBUS_PROPERTY_PREFIX "ext:ab:"

#include "ChangeListener.h"

class ValueChgListener:public ChangeListener {
    virtual void valueChanged(AndamBusSlave *slave, SlaveVirtualPort *port, int32_t oldValue);
};

int unit2devId(uint8_t unitID);
int dev2devId(uint8_t unitId, SlaveVirtualDevice *dev);
int port2Id(uint8_t unitId, SlaveVirtualPort *port);
int port2portId(uint8_t unitId, SlaveVirtualPort *port);
int ordinal2portId(uint8_t ordinal);
bool convertMetadataType(MetadataType type, DeviceMetadataType &dtype, int value, int &dvalue);
DeviceClass abusDevType2DevCls(VirtualDeviceType tp);


const std::string devType2IconCode(VirtualDeviceType type);
SlaveVirtualDevice* devId2dev(int devId);
uint8_t devId2unit(int devId);
uint8_t devId2pin(int devId);
uint8_t portId2pin(int portId);
uint8_t portId2ordinal(int portID);
uint64_t devId2longaddr(int devId);
bool devIdIsBusDevice(int devId);
//uint64_t dev2longaddr(SlaveVirtualDevice *vdev);
int longaddr2devId(uint8_t unitID, uint64_t longaddr);
uint64_t toSimpleW1Addr(uint64_t addr);

std::string AndambusPropertyType2ConfigName(AndamBusPropertyType tp, uint8_t propId);
void ConfigName2AndambusPropertyType(AndamBusPropertyType &tp, uint8_t &propId, std::string configName);


/*
int pin2id(uint8_t unitID, uint8_t pin);
*/

#endif // ICONTROLDRIVER_H_INCLUDED
