#ifndef DISPLAY_H
#define DISPLAY_H

#include "ArduinoDevice.h"
#include "BusDeviceHelper.h"
#include <LiquidCrystal_I2C.h>
#include "MultiClickDetector.h"

#define DEFAULT_DISPLAY_I2C_SLAVE_ADDR 0x27
#define LCD_MAX_LABEL_LENGTH 8
#define LCD_VAL1_POSITION 10
#define LCD_VAL2_POSITION 15
#define LCD_DEGREE_CHAR "\xDF"
#define LCD_VALUE_COUNT 8
#define LCD_ROWS 4
#define LCD_THERMOMETER_COUNT 6
#define LCD_DEVPORT_REF_COUNT 4
#define LCD_BACKLIGHT_TIMEOUT_SEC 12

class Display:public ArduinoDevice,protected MultiClickDetector {
  public:
    Display(uint8_t id, uint8_t pin, ArduinoAndamBusUnit *_abu);

    virtual VirtualDeviceType getType() { return VirtualDeviceType::I2C_DISPLAY; }
    void setLabel(uint8_t row, uint8_t col, char *lbl, uint8_t len);
    void associateValue(uint8_t row, uint8_t col, uint8_t id);

    virtual bool setData(const char *data, uint8_t size);

    virtual uint8_t getPortCount();
    virtual uint8_t getPortId(uint8_t idx);
    virtual void setPortId(uint8_t idx, uint8_t id);
    virtual VirtualPortType getPortType(uint8_t idx);
    virtual uint8_t getPortIndex(uint8_t id);
    virtual void setPortValue(uint8_t idx, int16_t value);
    virtual int16_t getPortValue(uint8_t idx);
    virtual bool isHighPrec() { return true; }

    bool setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId);
    virtual uint8_t getPropertyList(ItemProperty propList[], uint8_t size);

    virtual uint8_t getPersistData(uint8_t data[], uint8_t maxlen);
    virtual uint8_t restore(uint8_t data[], uint8_t length);

    virtual void clicked(uint8_t cnt);
    virtual void holdStarted(uint8_t cnt);


    void init();

    virtual void doWork();
    virtual void doWorkHighPrec();
    
  private:
    LiquidCrystal_I2C lcd;
    char lbls[LCD_ROWS][LCD_MAX_LABEL_LENGTH+1];
    bool initialized, backlight, backlightPermanent;
    unsigned long tBacklight;
    unsigned int backlightPeriodSec;
    
//    uint32_t t_addrh[LCD_THERMOMETER_COUNT], t_addrl[LCD_THERMOMETER_COUNT];
   // uint8_t tBusIndex[LCD_THERMOMETER_COUNT], tDevIndex[LCD_THERMOMETER_COUNT];
    uint8_t devPin[LCD_DEVPORT_REF_COUNT], portIdx[LCD_DEVPORT_REF_COUNT];
    int8_t value[LCD_VALUE_COUNT];
    uint8_t changed[LCD_ROWS], portId[LCD_VALUE_COUNT];

    void refreshData();
    void refreshDisplayContent();
    void printLabel(uint8_t row);
    void handleBacklight();
    bool hasContent();
	
	BusDeviceHelper tm[LCD_THERMOMETER_COUNT];
};

#endif // DISPLAY_H
