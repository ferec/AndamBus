#ifndef MULTICLICK_DETECTOR_H_INCLUDED
#define MULTICLICK_DETECTOR_H_INCLUDED

#include <Arduino.h>
#include <inttypes.h>

#define NOISE_THRESHOLD_MS 50
#define CLICK_TIMEOUT_MS 500
#define DBLCLICK_DELAY 300


class MultiClickDetector {
  public:
    enum class DetectorState:byte {
      Disabled,Idle,WaitRelease,WaitPress,Hold
    };
	
	enum class EventType:uint8_t {
		Click, HoldStart, HoldFinished
	};
  
    MultiClickDetector(uint8_t pin);

    virtual void doWorkHighPrec();

    virtual bool isHighPrec() { return true; }
	
	uint8_t getPin() { return pin; }
	void setPin(uint8_t pin);

	typedef void (*detectorCallback)(int,MultiClickDetector*, void*, EventType);

	void setClickHandler(detectorCallback cb, void* obj) { detCb = cb; trgObj = obj; }

  protected:
    bool handleNoise(int val,  unsigned long now);

    void checkPressed(int val);
    void waitForRelease(int val, unsigned long now);
    void waitForPress(int val, unsigned long now);
    void waitForHoldRelease(int val);
    
    virtual void clicked(uint8_t cnt);
    virtual void holdStarted(uint8_t cnt);
    virtual void holdFinished();
    
  private:
    uint8_t pin, clickCount;
    unsigned long lastChg;
    int prevValue;
    DetectorState state;
	detectorCallback detCb;
	void *trgObj;
	
	void initPin();
};

#endif // MULTICLICK_DETECTOR_H_INCLUDED