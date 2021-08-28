#ifndef WINDOWSHADINGDEVICE_H
#define WINDOWSHADINGDEVICE_H

#include "ArduinoDevice.h"
#include "ArduinoAndamBusUnit.h"
#include "MultiClickDetector.h"

#define SHADING_DEBUG true
#define SHADING_DIR_DELAY_MS 10
#define SHADING_DEFAULT_DOWN_TIME_SEC 40
#define SHADING_DEFAULT_SHADE_TIME_SEC 3
#define SHADING_DIFF_THRESHOLD_MS 50

#define RELATED_SHUTTER_COUNT_MAX 4
//#define SHADING_DIFF_THRESHOLD_MS_PREC 20

//#define WINDOWSHADE_MANUAL 1
//#define WINDOWSHADE_INITIALIZED 2

class WindowShadingDevice:public ArduinoDevice {
  public:
    WindowShadingDevice(uint8_t id, uint8_t pin, ArduinoAndamBusUnit *_abu);
    ~WindowShadingDevice();

    virtual uint8_t getPortCount();
    virtual uint8_t getPortId(uint8_t idx);
    virtual void setPortId(uint8_t idx, uint8_t id);
    virtual VirtualPortType getPortType(uint8_t idx);
    virtual uint8_t getPortIndex(uint8_t id);
    virtual int16_t getPortValue(uint8_t idx);
    virtual void setPortValue(uint8_t idx, int16_t value);
    virtual VirtualDeviceType getType() { return VirtualDeviceType::BLINDS_CONTROL; }
    virtual bool isHighPrec() { return true; }

    virtual uint8_t getPropertyList(ItemProperty propList[], uint8_t size);
    virtual bool setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId);
    virtual uint8_t getPersistData(uint8_t data[], uint8_t maxlen);
    virtual uint8_t restore(uint8_t data[], uint8_t length);
	
	virtual void doWorkHighPrec();

	static void clickHandler(int cnt, MultiClickDetector *det, void *obj, MultiClickDetector::EventType evtType);
	
	
	enum class Direction:uint8_t {NONE, UP, DOWN};
	enum class Status:uint8_t {UNINIT, INIT, IDLE, MOVING, MANUAL};
	
  protected:
	ArduinoAndamBusUnit *abu;
//	uint8_t boolPack;
	MultiClickDetector detUp, detDown;
	uint8_t portIdPosition, portIdShadow,
		pinMove, pinDirDown; // move down
	Status status;
	
	uint32_t actPosition, actShadow; // in millisecond
	uint16_t reqPosition, reqShadow, // 1/100th of a second
		downTime, shadeTime, // 1/100th of a second
		lastUpdate;
		
	uint8_t holdClickCnt;

	uint8_t relatedShutters[RELATED_SHUTTER_COUNT_MAX]; // main pin of related shutters

	virtual bool configMissing();
	
  private:
    void clicked(uint8_t cnt, Direction dir, bool related);
    void holdStart(uint8_t cnt, Direction dir, bool related);
    void holdFinished(uint8_t cnt, Direction dir, bool related);
	
	bool initPin(uint8_t pin);
    bool blockPin(uint8_t pin);
    void unblockPin(uint8_t pin);
	
	
	Direction getMovingDirection();
	Direction getRequestDirection();

	void handleRequest();
	void setRequest(uint16_t pos, uint16_t shd);
//	void setReqShadow(uint16_t shd);
	void updateActuals();
	void stop();
	void updateRequestFromActual();

	void start(Direction dir);
	
	bool onPosition();
	bool onShade();
	
	void setStatus(Status stat);

};

#endif //WINDOWSHADINGDEVICE_H
