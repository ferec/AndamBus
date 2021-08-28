#ifndef ANDAMBUSCONST_H_INCLUDED
#define ANDAMBUSCONST_H_INCLUDED

#define ANDAMBUS_API_VERSION_MAJOR 1
#define ANDAMBUS_API_VERSION_MINOR 1

#define ANDAMBUS_MAGIC_WORD 0x58613a5e

#define ANDAMBUS_MAX_ITEM_COUNT 0x40
#define ANDAMBUS_MAX_BUS_DEVS 16
#define ANDAMBUS_MAX_DEV_PORTS 16
#define ANDAMBUS_MAX_SECONDARY_BUSES 4

#define UNIT_MAX_ID 0xf8

#define ANDAMBUS_TIMEOUT_MS 200

#include <inttypes.h>

enum class AndamBusCommand:uint8_t {
NONE = 0,
DETECT_SLAVE = 0x02, // detectes arduino devices on RS485 bus
METADATA = 0x03, // provides slave metadata about commands supported, device supplier, telemetry information, etc.
PROPERTY_LIST = 0x16, // provides list of properties on slave/bus/device/port level
GET_DATA = 0x04, // provides RAW (item-specific) data on slave/bus/device/port level
SET_DATA = 0x05, // sets RAW (item-specific) data slave/bus/device/port level
PERSIST = 0x06, // stores configuration to EEPROM
RESET = 0x07, // do SW reset of the slave device
GET_CONFIG = 0x08, // provides slave configuration BLOB
RESTORE_CONFIG = 0x09, // restores slave configuration BLOB
SET_PROPERTY = 0x18, // sets property value on slave/bus/device/port level

// following commands are for arduino device specified by address
SECONDARY_BUS_CREATE = 0x11, // creates 1wire bus for specified pin
SECONDARY_BUS_DETECT = 0x12, // executes the device autodetection procedure on all 1wire buses
SECONDARY_BUS_REMOVE = 0x13, // removes the 1wire bus identified by pin number
SECONDARY_BUS_LIST = 0x15, // provides list of 1wire buses
SECONDARY_BUS_COMMAND = 0x17, // executes the convert command on 1wire bus

// following commands are for arduino device specified by address and virtual device specified by ID
VIRTUAL_DEVICE_CREATE = 0x33, // creates a virtual device of specified types with attribute list on specified pins, returns virtual device full spec
VIRTUAL_DEVICE_REMOVE = 0x34, // destroys virtual device and frees all resources (pin, allocated memory)
VIRTUAL_DEVICE_CHANGE = 0x35, // modifies virtual device (set parameter value,
VIRTUAL_DEVICE_LIST = 0x37, // // provides list of virtual devices like thermometers, blind controls, thermostats etc including ports, properties and values

// following commands are for arduino device specified by address and port specified by ID
VIRTUAL_PORT_CREATE = 0x61, // creates virtual port of specified type (digital, analog, PWM, input/output)
VIRTUAL_PORT_REMOVE = 0x62, // removes virtual port (applies only for ports not created as part of a virtual device)
VIRTUAL_PORT_LIST = 0x64, // provides list of virtual ports with definition
VIRTUAL_PORT_SET_VALUE = 0x63, // provides virtual port value (analog/digital)
VALUES_CHANGED = 0x31, // provides list on virtual port and its values changed from last read

VIRTUAL_ITEMS_CREATED = 0x36, // provides list of newly created virtual devices and ports based on counter
VIRTUAL_ITEMS_REMOVED = 0x38 // provides list of deleted virtual devices and ports based on counter
};

enum class SlaveHwType : uint8_t {
    UNKNOWN = 0x4,
    ARDUINO_UNO = 0x5,
    ARDUINO_DUE = 0x6,
    ARDUINO_MEGA = 0x7,
    TEST = 0x8
};

/*
    in use bit 0: active=1,inactive=0
    direction bit 1: input=0,output=1
    type bit 2: digital=0,analog=1
    pullup bit 3: pullup=1,no pullup=0
    pwm bit 4: pwm=1, no pwm=0
*/

enum class VirtualPortType : uint8_t {
    NONE=0x0,
    DIGITAL_INPUT=0x1,
    DIGITAL_INPUT_PULLUP = 0x9,
    DIGITAL_OUTPUT = 0x3,
	DIGITAL_OUTPUT_REVERSED = 0x04,
    ANALOG_INPUT = 0x5,
    ANALOG_OUTPUT = 0x7,
    ANALOG_OUTPUT_PWM = 0x17
};

enum class VirtualDeviceType : uint8_t {
    NONE=0x4,
    THERMOMETER=0x5, // thermometer device
    THERMOSTAT=0x6, // thermostat - analog input, digital output
    BLINDS_CONTROL=0x7, // analog input shading %, position %, digital out for motor down, motor up
    PUSH_DETECTOR=0x8, // digital input on pin, output detected click, doubleclick, push
    DIFF_THERMOSTAT=0x9, // 2 thermometer inputs, 1 digital output (pin)
    CIRC_PUMP=0xa, // 1 thermometer input, 1 digital output based on timer and temperature value
    SOLAR_THERMOSTAT=0xb, // 3 thermometer inputs, 2 digital outputs: pump and 3-way valve
    DIGITAL_POTENTIOMETER=0xc, // MCP4651 potentiometer with 2 channels
    I2C_DISPLAY=0xd, // LCD display with I2C driver 4x20
	HVAC=0xe, // climatization, heating or air-conditioning device
    TIMER=0xf, // timer
	MODIFIER=0x10, // device to modify value (increase/decrease) of a target analog port
};

enum class OnewireBusCommand : uint8_t {
    CONVERT=0x44
};

enum class AndamBusResponseType:uint8_t {
OK = 0x35,  // simply accepting command without additional information
OK_DATA = 0x36, // accepting command and returning additional info
ERROR = 0x65 // error during command execution
};

enum class AndamBusPropertyType:uint16_t {
    NONE = 0,
    PORT_TYPE = 0x01, // type of the virtual port with values
    PORT_VALUE = 0x02, // value of the port (analog 32bit integer)
    DEVICE_TYPE = 0x03, // type of the virtual device
    W1CMD = 0x04, // onewire command
    SLAVE_HW_TYPE = 0x05, // see SlaveHwType enum
    SW_VERSION = 0x06, // value contains major and minor version
    CUSTOM = 0x07, // custom property specific
    LONG_ADDRESS = 0x08, // 1wire address - high and low word distinguished by propertyId (0=HIGH,1=LOW)
    SLAVE_ADDRESS=0x09, // AndamBus unit address (default 1 for slave, =0 for master)
    SECONDARY_BUS_REFRESH=0xa, // calling 1wire bus convert command
    ITEM_ID = 0xb, // item identifier like port id, device id
    TEMPERATURE = 0xc, // temperature in degrees Celsius * 1000
    PERIOD_SEC = 0xd, // Period in seconds
    PIN = 0xe, // additional pin
    STEP = 0xf, // increase/decrease stepping
    PERIOD_MS = 0x10, // Period in milliseconds
    HIGHLOGIC = 0x11, // if 1, then logical true->1/false->0, if 0 then reversed logic
};

enum class MetadataType:uint16_t {
    // metadata
    DIGITAL_INPUT_PORTS = 0x10, // bitmask for pins that can be set as digital input
    ANALOG_INPUT_PORTS = 0x11, // bitmask for pins that can be set as analog input
    DIGITAL_OUTPUT_PORTS = 0x12, // bitmask for pins that can be set as digital output
    ANALOG_OUTPUT_PORTS = 0x13, // bitmask for pins that can be set as analog output, including PWM

    TRY_COUNT = 0x30, // device refresh try count

    // telemetry
    FREE_MEMORY = 0x60, // returns number of free bytes on unit
    UPTIME = 0x61,     // returns uptime in seconds
    TICK = 0x62,     // returns ticks/iterations from start
	MAX_CYCLE_DURATION = 0x63, // returns main cycle work duration maximum is ms
	CYCLE_DURATION_RANGE = 0x64, // returns main cycle duration range counts 0: 10ms, 1:30ms, 2: 50ms, 3: 100ms, 4: 150ms, 5: 200ms

    ERROR_COUNT_CRC = 0x80,  // returns CRC errors count
    ERROR_COUNT_SYNC = 0x81,  // returns sync lost errors count
    ERROR_COUNT_LONG = 0x82,  // returns packet too long errors count
};

struct UnitMetadata {
    MetadataType type; // type of information
    uint16_t propertyId; // id of property for multiple values
    uint32_t value; // value of the metadata/telemetry property
};

struct ItemProperty {
    AndamBusPropertyType type; // 2 bytes
    uint8_t entityId;   // ID of the entity: SecodaryBusId/VirtualDeviceID/VirtualPortId
    uint8_t propertyId; // type of custom property
    union {
        VirtualPortType portType; // 1 byte
        VirtualDeviceType deviceType; // 1 byte
        OnewireBusCommand cmd; // 1 byte
        SlaveHwType hwType; // 1 byte
        int32_t value;
    };
};

struct SecondaryBus {
    uint8_t id;
    uint8_t pin;
    uint8_t status; // bit 0: missing bus devs
};


struct VirtualDevice {
    uint8_t id;
    uint8_t busId;
    VirtualDeviceType type;
    uint8_t pin;
    uint8_t status; // bit 0: not configured, bit 1: missing input dev, bit 2: input dev error
};

struct VirtualPort {
    int32_t value;
    uint8_t id;
    uint8_t deviceId;
    VirtualPortType type;
    uint8_t pin;
    uint8_t ordinal;
    uint8_t reserved[3];
};

struct ItemValue {
    int32_t value;
    uint8_t id;
    uint8_t age;
    uint8_t reserved[2];
};


struct AndamBusFrameHeader {
uint32_t magicWord; // constant value defined as ANDAMBUS_MAGIC_WORD, to help synchronize/recover after error
uint32_t payloadLength; // length of the rest of frame excluding header (frames to ignore for other slaves)
uint16_t slaveAddress; // master device has the address 0, 0xff is broadcast
uint16_t counter; // communication counter - command and response must be correlated. Also used for incremental commands (devs/ports added from last read)
uint16_t reserved;
struct {
    uint8_t major;
    uint8_t minor;
} apiVersion;
//uint32_t crc32; // 32-bit cyclic redundancy check value of all fields with this field value = 0
};

struct AndamBusFrameCommand {
    AndamBusCommand command;
    uint8_t id; // (depending on command type) ID of the addressed item: virtual device ID in case of virtual device or port ID in case of virtual port
    uint8_t pin; // determines pin for the specified command if approppriate, for GET_DATA/SET_DATA used as data type
    uint8_t propertyCount; // count of the property sent/number of bytes sent as raw data
    union {
        ItemProperty props[ANDAMBUS_MAX_ITEM_COUNT];
        char data[ANDAMBUS_MAX_ITEM_COUNT];
    };
};

enum class ResponseContent:uint8_t {
    NONE = 0x77,
    SECONDARY_BUS = 0x78, // AndamBusResponseData.busItems
    VIRTUAL_DEVICE = 0x79, //AndamBusResponseData.deviceItems
    VIRTUAL_PORT = 0x7a, //AndamBusResponseData.portItems
    PROPERTY = 0x7b, // AndamBusResponseData.props
    VALUES = 0x7c, //AndamBusResponseData.portValues
    MIXED = 0x7d, //AndamBusResponseData.mixed
    RAWDATA = 0x7e, // AndamBusResponseData.data
    METADATA = 0x7f // AndamBusResponseData.metadata
};

struct AndamBusResponseData {
    uint8_t itemCount; // in case of MIXED content type number of devices
    ResponseContent content;
    uint8_t itemCount2; // in case of MIXED content type number of ports, RAWDATA internal type
    uint8_t itemCount3; // in case of MIXED content type number of properties, RAWDATA internal type
    union {
        ItemProperty props[0];
        VirtualPort portItems[0];
        SecondaryBus busItems[0];
        VirtualDevice deviceItems[0];
        ItemValue portValues[0];
        char data[0];
        UnitMetadata metadata[0]; // metadata and telemetry information
    } responseData;
};

enum class AndamBusCommandError:uint8_t {
    OK = 0,
    NOT_IMPLEMENTED = 1,
    ITEM_DOES_NOT_EXIST = 2,
    MISSING_ARGUMENT = 3,
    CRC_MISMATCH = 4,
    INVALID_API_VERSION = 5,
    PIN_ALREADY_USED = 6,
    PIN_DIRECTION = 7,
    INVALID_PIN = 8,
    ITEM_LIMIT_REACHED = 9,
    UNKNOWN_DEVICE_TYPE = 10,
    UNKNOWN_PORT_TYPE = 11,
    NOT_REFRESHED = 12, // thrown in case a unit is reinitialized and needs a full refresh to do any other operation
    NON_REMOVABLE = 13, // device port tried to remove

	OTHER_ERROR = 0x41
};

struct AndamBusFrameResponse {
    AndamBusResponseType responseType;
    uint8_t reserved[3];
    union {
        AndamBusResponseData typeOkData;
        struct {
            AndamBusCommandError errorCode;
        } typeError;
    };
};

struct AndamBusFrame {
    AndamBusFrameHeader header;
    union {
        AndamBusFrameCommand command;
        AndamBusFrameResponse response;
    };
};

#endif // ANDAMBUSCONST_H_INCLUDED
