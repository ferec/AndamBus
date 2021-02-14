#ifndef ARDUINOPIN_H
#define ARDUINOPIN_H

#include <inttypes.h>
#include "AndamBusTypes.h"
#include "Persistent.h"

#define AR_PIN_ACTIVE       1
#define AR_PIN_BLOCKED    2
#define AR_PIN_CHANGED     4

class ArduinoPin:public Persistent {
  public:
    ArduinoPin();

    virtual uint8_t getPersistData(uint8_t data[], uint8_t maxlen);
    virtual uint8_t restore(uint8_t data[], uint8_t length);
    virtual uint8_t typeId();

    void setType(VirtualPortType type);

    bool isInput();
    bool isOutput();
    bool isAnalog();
	bool isReversed();

    void setValue(int16_t value);
    int16_t getValue();
	
	bool isActive();
	void setActive(bool val);
	bool isBlocked();
	void setBlocked(bool val);
	bool isChanged();
	void setChanged(bool val);
  
    uint8_t pin;
    VirtualPortType type;
//    bool active, blocked;
  protected:
    int16_t value;
	uint8_t boolPack;
};

#endif // ARDUINOPIN_H
