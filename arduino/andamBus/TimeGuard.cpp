#include "TimeGuard.h"
#include <Arduino.h>
#include "util.h"
#include "ArduinoAndamBusUnit.h"

TimeGuard::TimeGuard(uint8_t _pin):lastSwitch(0),tgPortId(0),noSwitchPeriod(1) {
  
  setTGPin(_pin);
}

void TimeGuard::setTGPin(uint8_t _pin) {
  pPin = _pin;
  
  if (pPin > 0) {
    pinMode(pPin, OUTPUT);
    digitalWrite(pPin, HIGH);
  }
}

bool TimeGuard::isTGActive() {
  if (!isTGValid())
    return false;
    
  return digitalRead(pPin) == LOW;
}

void TimeGuard::activateTGPin(bool run) {
//  LOG_U(F("switching pump to ") << run << " " << isTGValid() << " " << pPin);

  if (!isTGValid())
    return;
  
  if (run && !isTGActive()) {
    digitalWrite(pPin, LOW);
    lastSwitch=millis();
  } 
  if (!run && isTGActive()) {
    digitalWrite(pPin, HIGH);
    lastSwitch=millis();    
  }
}

bool TimeGuard::tgTooFast() {
  unsigned long now = millis();
  return lastSwitch > 0 && (lastSwitch/1000 + noSwitchPeriod) > now/1000 && now >= lastSwitch;
}

unsigned int TimeGuard::getSecondsFromLastSwitch() {
  unsigned long now = millis();
  if (now >= lastSwitch) {
    uint32_t diff = (now - lastSwitch)/1000;
    return diff>0xffff?0xffff:diff;
  }
  return 0xffff;
}

void TimeGuard::setPortId(uint8_t id) {
  tgPortId = id;  
}

bool TimeGuard::isTGValid() { 
	return pPin > 0 && pPin < MAX_VIRTUAL_PORTS;
}
