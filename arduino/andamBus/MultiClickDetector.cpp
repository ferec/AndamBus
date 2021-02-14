#include <Arduino.h>
#include "MultiClickDetector.h"
#include "util.h"
#include "ArduinoAndamBusUnit.h"

MultiClickDetector::MultiClickDetector(uint8_t _pin):pin(_pin),clickCount(0),lastChg(0),state(DetectorState::Idle),detCb(nullptr),trgObj(nullptr) {
  initPin();
}

void MultiClickDetector::initPin() {
  if (pin == 0 || pin >= MAX_VIRTUAL_PORTS)
	return;

  pinMode(pin, INPUT_PULLUP);
  prevValue = digitalRead(pin);
}

void MultiClickDetector::setPin(uint8_t _pin) {
	pin = _pin;
	
	initPin();
	
	LOG_U("setpin " << (int)pin);
}

void MultiClickDetector::doWorkHighPrec() {
  if (pin == 0 || pin >= MAX_VIRTUAL_PORTS)
	return;

//  Serial.println(i++);
  int val = digitalRead(pin);
  unsigned long now = millis();

  if (handleNoise(val, now))
    return;

  switch (state) {
      case DetectorState::Disabled:
       return;
      case DetectorState::Idle:
       checkPressed(val);
       break;
      case DetectorState::WaitRelease:
       waitForRelease(val, now);
       break;
      case DetectorState::WaitPress:
       waitForPress(val, now);
       break;
      case DetectorState::Hold:
       waitForHoldRelease(val);
       break;
      default:
        LOG_U("Error status " << (int)state);
    } 

  if (prevValue != val) {
    prevValue = val;
    lastChg = now;
  }
}

void MultiClickDetector::waitForHoldRelease(int val)
{
  if (val && !prevValue)
  {
    holdFinished();
    state = DetectorState::Idle;
    clickCount = 0;
  }  
}

void MultiClickDetector::waitForPress(int val, unsigned long now)
{
  if (now - lastChg > DBLCLICK_DELAY)
  {
    clicked(clickCount);

    clickCount=0;
    state = DetectorState::Idle;
    return;
  }
  
  if (!val && prevValue)
  {
    state = DetectorState::WaitRelease;
  }
}

void MultiClickDetector::waitForRelease(int val, unsigned long now)
{
  if (now - lastChg > CLICK_TIMEOUT_MS)
  {
    state = DetectorState::Hold;
    holdStarted(clickCount);
    return;
  }
    
  if (val && !prevValue)
  {
    state = DetectorState::WaitPress;
    clickCount++;
  }
}

void MultiClickDetector::checkPressed(int val)
{
  if (!val && prevValue)
  {
    state = DetectorState::WaitRelease;
  }
}

bool MultiClickDetector::handleNoise(int val,  unsigned long now) {
  if (prevValue != val && (now - lastChg) < NOISE_THRESHOLD_MS) {
    // noise - cancelling click
//    Serial.print("noise detected on ");
//    Serial.println((int)pin);
    prevValue = val;
    lastChg = now;
    return true;
  }

  return false;
}

void MultiClickDetector::clicked(uint8_t cnt) {
//  LOG_U("clicked");
  if (detCb != nullptr)
	  detCb(cnt, this, trgObj, EventType::Click);
}

void MultiClickDetector::holdStarted(uint8_t cnt) {
  if (detCb != nullptr)
	  detCb(cnt, this, trgObj, EventType::HoldStart);
}

void MultiClickDetector::holdFinished() {
  if (detCb != nullptr)
	  detCb(0, this, trgObj, EventType::HoldFinished);
}
