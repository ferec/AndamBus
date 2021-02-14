#ifndef MIDEA_H
#define MIDEA_H

#define MIDEA_COOL_HEAT
#define MIDEA_DEFAULT_TEMP_TARGET 5
#define MIDEA_DEFAULT_TEMP_TARGET_CLIMA 17
#define MIDEA_DEFAULT_TEMP_TARGET_HEATING 30

#define MIDEA_MARK_LIMIT    4200 
#define MIDEA_MARK_LIMIT2   550
#define MIDEA_SPACE_LIMIT   4500 
#define MIDEA_START_BYTE    0xB2
#define MIDEA_MARK          450
#define MIDEA_SPACE         1700
#define MIDEA_SPACE2        600 
#define MIDEA_FREQUENCY     38

//#include <MideaIR.h>
#include <IRremote.h>
#include "ArduinoDevice.h"
#include "ArduinoAndamBusUnit.h"
#include "DevPortHelper.h"
#include "BusDeviceHelper.h"

class Midea:public ArduinoDevice {
  public:
    enum Modes{
        mode_cool        = 0x0,
        mode_no_humidity = 0x4,
        mode_auto        = 0x8,
        #ifdef MIDEA_COOL_HEAT
        mode_heat        = 0xC,
        #endif
        mode_ventilate   = 0xF
    };
    
    enum FanSpeed{
        fan_auto    = 0xB,
        fan_speed_1 = 0x9,
        fan_speed_2 = 0x5,
        fan_speed_3 = 0x3
    };

  
    Midea(uint8_t id, uint8_t pin, ArduinoAndamBusUnit *_abu);
    ~Midea();

/*    virtual uint8_t getPortCount();
    virtual uint8_t getPortId(uint8_t idx);
    virtual void setPortId(uint8_t idx, uint8_t id);
    virtual VirtualPortType getPortType(uint8_t idx);
    virtual void setPortValue(uint8_t idx, int16_t value);*/
    virtual VirtualDeviceType getType() { return VirtualDeviceType::HVAC; }

    virtual uint8_t getPropertyList(ItemProperty propList[], uint8_t size);
    virtual bool setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId);
    virtual uint8_t getPersistData(uint8_t data[], uint8_t maxlen);
    virtual uint8_t restore(uint8_t data[], uint8_t length);

    virtual void doWork();

  private:
    IRsend irSend;
//    MideaIR mideaIR;
    ArduinoAndamBusUnit *abu;

    DevPortHelper dpPot, dpTarget;
    BusDeviceHelper tmTarget, tmOut;
    Modes mode;
    FanSpeed fanSpeed;

    int8_t tempTargetClima;
    uint8_t //portIdOnOff, portIdHeatCool, portIdTemp, 
      pinOn, pinHeat, pinVentReq, pinVent;
    bool running;

    uint8_t calculateDigipot();
    int8_t getTempTarget();
    void setTempTarget(int8_t);
    void setMode();
    bool initPin(uint8_t pin);
    bool initPin(uint8_t pin, uint8_t mode);
    bool blockPin(uint8_t pin);
    void unblockPin(uint8_t pin);

    void turnON();
    void turnOFF();
    void emit(uint8_t cmd_byte1, uint8_t cmd_byte2);
    void emitLL(uint8_t cmd_byte1, uint8_t cmd_byte2);
    void emitByte(uint8_t data);
};


#endif // MIDEA_H
