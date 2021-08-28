#include "ArduinoPin.h"
#include "Arduino.h"
#include "util.h"

ArduinoPin::ArduinoPin()://active(false),blocked(false)//,value(0) 
	boolPack(0)
{
}

bool ArduinoPin::isInput() {
  return type == VirtualPortType::DIGITAL_INPUT || type == VirtualPortType::DIGITAL_INPUT_PULLUP || type == VirtualPortType::ANALOG_INPUT;
}

bool ArduinoPin::isAnalog() {
  return type == VirtualPortType::ANALOG_OUTPUT || type == VirtualPortType::ANALOG_INPUT || type == VirtualPortType::ANALOG_OUTPUT_PWM;
}

bool ArduinoPin::isOutput() {
  return type == VirtualPortType::DIGITAL_OUTPUT || type == VirtualPortType::DIGITAL_OUTPUT_REVERSED || type == VirtualPortType::ANALOG_OUTPUT || type == VirtualPortType::ANALOG_OUTPUT_PWM;
}

bool ArduinoPin::isReversed() {
  return type == VirtualPortType::DIGITAL_OUTPUT_REVERSED;
}

int ArduinoPin::getValue() {
  int lowVal = isReversed()?HIGH:LOW;
	
  if (isInput())
    return isAnalog()?analogRead(pin):(digitalRead(pin)!=lowVal);

  //LOG_U("read pin=" << digitalRead(pin) << " " << lowVal << " " << value);
  return value;
}

void ArduinoPin::setValue(int16_t _value) {
  if (!isOutput())
	  return;

  if (value != _value)
	  setChanged(true);

  value = _value;
  if (_value < 0)
    value = 0;
/*  if (_value > 255)
    value = 255;*/

  if (type == VirtualPortType::DIGITAL_OUTPUT)
    digitalWrite(pin, value?HIGH:LOW);
  else {
	if (type == VirtualPortType::DIGITAL_OUTPUT_REVERSED)
		digitalWrite(pin, value?LOW:HIGH);
	else
		analogWrite(pin, value);
  }
}

void ArduinoPin::setType(VirtualPortType _type) {
  type = _type;
  
  if (isOutput())
    pinMode(pin, OUTPUT);
  
  if (type == VirtualPortType::DIGITAL_OUTPUT) {
    digitalWrite(pin, LOW);
  }

  if (type == VirtualPortType::DIGITAL_OUTPUT_REVERSED) {
    digitalWrite(pin, HIGH);
  }
  
  if (type == VirtualPortType::ANALOG_OUTPUT || type == VirtualPortType::ANALOG_OUTPUT_PWM) {
//    pinMode(pin, OUTPUT);
    analogWrite(pin, 0);
  }
  value = 0;
  
  if (type == VirtualPortType::DIGITAL_INPUT || type == VirtualPortType::ANALOG_INPUT)
    pinMode(pin, INPUT);
  if (type == VirtualPortType::DIGITAL_INPUT_PULLUP)
    pinMode(pin, INPUT_PULLUP);
}

uint8_t ArduinoPin::getPersistData(uint8_t data[], uint8_t maxlen)
{
    data[0]=pin;
    data[1]=static_cast<uint8_t>(type);

    return 2;
}

bool ArduinoPin::isBlocked() {
	return BB_READ(boolPack, AR_PIN_BLOCKED);
}

void ArduinoPin::setBlocked(bool val) {
	if (val)
		BB_TRUE(boolPack,AR_PIN_BLOCKED);
	else
		BB_FALSE(boolPack,AR_PIN_BLOCKED);
}

bool ArduinoPin::isChanged() {
	if (isOutput())
		return BB_READ(boolPack, AR_PIN_CHANGED);

//	LOG_U("isChanged " << getValue() << " " << value);
	return getValue() != value;
}

void ArduinoPin::setChanged(bool val) {
	if (isOutput()) {
		if (val)
			BB_TRUE(boolPack,AR_PIN_CHANGED);
		else
			BB_FALSE(boolPack,AR_PIN_CHANGED);
	} else {
		value = getValue();
	}
}

bool ArduinoPin::isActive() {
	return BB_READ(boolPack, AR_PIN_ACTIVE);
}

void ArduinoPin::setActive(bool val) {
	if (val)
		BB_TRUE(boolPack,AR_PIN_ACTIVE);
	else
		BB_FALSE(boolPack,AR_PIN_ACTIVE);
}

uint8_t ArduinoPin::restore(uint8_t data[], uint8_t length)
{
    pin=data[0];

    setType(static_cast<VirtualPortType>(data[1]));

    value = 0; //(type == VirtualPortType::DIGITAL_OUTPUT?1:0);

//    active = true;
	setActive(true);

    return 2;
}

uint8_t ArduinoPin::typeId()
{
  return (uint8_t)Persistent::Type::ArduinoPin;
}
