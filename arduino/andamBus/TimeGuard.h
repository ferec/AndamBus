#ifndef TIMEGUARD_H
#define TIMEGUARD_H

#include <stdint.h>

class TimeGuard {
  public:
    TimeGuard(uint8_t pin);

    void setNoSwitchPeriod(uint16_t sec) { noSwitchPeriod = sec; }
    uint16_t getNoSwitchPeriod() { return noSwitchPeriod; }
    uint8_t getTGPin() {return pPin;}
    void setTGPin(uint8_t _pin);
    bool isTGActive();
    bool tgTooFast();
    void activateTGPin(bool run);
    bool isTGValid();

    uint8_t getPortId() { return tgPortId; }
    void setPortId(uint8_t id);
  protected:


    unsigned int getSecondsFromLastSwitch();

    uint8_t tgPortId;

  private:
    uint8_t pPin;
    uint16_t noSwitchPeriod; // in seconds; will not change output pin value until noSwitchPeriod seconds
    unsigned long lastSwitch;
};

#endif // TIMEGUARD_H
