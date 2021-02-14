#include "WindowShadingDevice.h"
#include "util.h"

WindowShadingDevice::WindowShadingDevice(uint8_t _id, uint8_t _pin, ArduinoAndamBusUnit *_abu):ArduinoDevice(_id, _pin),abu(_abu),
	detUp(_pin),detDown(0),portIdPosition(0),portIdShadow(0),
	actPosition(0),actShadow(0),reqPosition(0),reqShadow(0),lastUpdate(0),
	pinMove(0),pinDirDown(0),
	downTime(SHADING_DEFAULT_DOWN_TIME_SEC*100),shadeTime(SHADING_DEFAULT_SHADE_TIME_SEC*100),
	//manual(false)
	//boolPack(0)
	status(Status::UNINIT)
{
	detUp.setClickHandler(clickHandler, this);
	detDown.setClickHandler(clickHandler, this);
	
}

WindowShadingDevice::~WindowShadingDevice() {
	LOG_U("destruct");
	unblockPin(pinMove);
	unblockPin(pinDirDown);
	unblockPin(detDown.getPin());
}

uint8_t WindowShadingDevice::getPortCount() {
  return 2;
}

uint8_t WindowShadingDevice::getPortId(uint8_t idx) {
  if (idx == 0)
	return portIdPosition;
  if (idx == 1)
	return portIdShadow;
  return 0;
}

void WindowShadingDevice::setPortId(uint8_t idx, uint8_t id) {
  if (idx == 0)
	portIdPosition = id;
  if (idx == 1)
	portIdShadow = id;
}

VirtualPortType WindowShadingDevice::getPortType(uint8_t idx) {
  return VirtualPortType::ANALOG_OUTPUT;
}

uint8_t WindowShadingDevice::getPortIndex(uint8_t id) {
  if (id == portIdPosition)
	return 0;
  if (id == portIdShadow)
	return 1;
  return 0xff;
}

int16_t WindowShadingDevice::getPortValue(uint8_t idx) {
  if (idx == 0)
	return map((actPosition+5)/10, 0,downTime, 0,100);
  if (idx == 1)
	return map((actShadow+5)/10, 0, shadeTime, 0, 100);
  return 0;
}

void WindowShadingDevice::setPortValue(uint8_t idx, int16_t value) {
  switch (status) {
	  case Status::UNINIT:
	  case Status::INIT:
	  case Status::MANUAL:
		return;
  }
		
  if (value > 100)
	  value = 100;
  if (value < 0)
	  value = 0;
  
//  setChanged(true);
  if (idx == 0)
	setRequest(map(value, 0,100, 0, downTime), reqShadow);
  if (idx == 1)
	setRequest(reqPosition, map(value, 0,100, 0, shadeTime));
}

bool WindowShadingDevice::initPin(uint8_t pin) {
  if (ArduinoAndamBusUnit::pinValid(pin))
    if (blockPin(pin)) {
	  if (SHADING_DEBUG)
		LOG_U("initPin " << (int)pin);
	
      pinMode(pin, OUTPUT);
	  digitalWrite(pin, HIGH);
      return true;
    }
  return false;
}

void WindowShadingDevice::unblockPin(uint8_t pin) {
  if (abu == nullptr)
    return;
  if (!ArduinoAndamBusUnit::pinValid(pin))
    return;

  abu->unblockPin(pin);
}

bool WindowShadingDevice::blockPin(uint8_t pin) {
  if (abu == nullptr)
    return true;

  if (!abu->pinAvailable(pin))
    return false;

  abu->blockPin(pin);
  return true;
}

bool WindowShadingDevice::configMissing() {
	return !ArduinoAndamBusUnit::pinValid(pinMove) || !ArduinoAndamBusUnit::pinValid(pinDirDown) || !ArduinoAndamBusUnit::pinValid(detDown.getPin());
}

uint8_t WindowShadingDevice::getPropertyList(ItemProperty propList[], uint8_t size) {
	int cnt = ArduinoDevice::getPropertyList(propList, size);

	uint8_t pin = detDown.getPin();
    if (ArduinoAndamBusUnit::pinValid(pin)) {
		propList[cnt].type = AndamBusPropertyType::PIN;
		propList[cnt].entityId = id;
		propList[cnt].propertyId = 0;
		propList[cnt++].value = pin;
	}

	pin = pinMove;
    if (ArduinoAndamBusUnit::pinValid(pin)) {
		propList[cnt].type = AndamBusPropertyType::PIN;
		propList[cnt].entityId = id;
		propList[cnt].propertyId = 1;
		propList[cnt++].value = pin;
	}

	pin = pinDirDown;
    if (ArduinoAndamBusUnit::pinValid(pin)) {
		propList[cnt].type = AndamBusPropertyType::PIN;
		propList[cnt].entityId = id;
		propList[cnt].propertyId = 2;
		propList[cnt++].value = pin;
	}
	
	propList[cnt].type = AndamBusPropertyType::PERIOD_MS;
	propList[cnt].entityId = id;
	propList[cnt].propertyId = 0;
	propList[cnt++].value = ((int32_t)downTime)*10;
	
	propList[cnt].type = AndamBusPropertyType::PERIOD_MS;
	propList[cnt].entityId = id;
	propList[cnt].propertyId = 1;
	propList[cnt++].value = shadeTime*10;

	return cnt;
}

bool WindowShadingDevice::setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId) {
	uint8_t dpin = detDown.getPin();

	if (type == AndamBusPropertyType::PIN && propertyId == 0) {
		if (dpin != value) {
		  unblockPin(dpin);
		  if (blockPin(value)) {
			detDown.setPin(value);
			return true;
			}
		}
	}

	if (type == AndamBusPropertyType::PIN && propertyId == 1) {
		if (pinMove != value) {
		  unblockPin(pinMove);
		  if (initPin(value))
			pinMove = value;
		  else
			pinMove = 0;
		}
	}

	if (type == AndamBusPropertyType::PIN && propertyId == 2) {
		if (pinDirDown != value) {
		  unblockPin(pinDirDown);
		  if (initPin(value))
			pinDirDown = value;
		  else
			pinDirDown = 0;
		}
	}

	if (type == AndamBusPropertyType::PERIOD_MS && propertyId == 0) {
		if (value >= 0 && value < 650000)
			downTime = value/10; // 1/100th of a second
		
		if (SHADING_DEBUG)
			LOG_U("downTime=" << iom::dec << downTime);
	}

	if (type == AndamBusPropertyType::PERIOD_MS && propertyId == 1) {
		if (value >= 0 && value < 650000)
			shadeTime = value/10; // 1/100th of a second
	}

  return ArduinoDevice::setProperty(type, value, propertyId);
}

uint8_t WindowShadingDevice::restore(uint8_t data[], uint8_t length)
{
  uint8_t cnt32 = 0, cnt16 = 0, cnt8 = 0;
  uint8_t used = ArduinoDevice::restore(data, length);
  uint32_t *data32 = (uint32_t*)(data + used);

  uint16_t *data16 = (uint16_t*)(data32 + cnt32);

  if (length >= used + cnt32 * 4 + cnt16 * 2 + 2*2) {
	downTime = data16[cnt16++];
	shadeTime = data16[cnt16++];
  }
  

  uint8_t *data8 = (uint8_t*)(data16 + cnt16);

  if (length >= used + cnt32 * 4 + cnt16 * 2 + cnt8 + 3) {
    uint8_t dpin = data8[cnt8++];
	if (SHADING_DEBUG)
		LOG_U("init pin " << (int)dpin);
	
	detDown.setPin(dpin);

    pinMove = data8[cnt8++];
    pinDirDown = data8[cnt8++];
	
	initPin(pinMove);
	initPin(pinDirDown);
  }


  return used + cnt32 * 4 + cnt16 * 2 + cnt8;
}

uint8_t WindowShadingDevice::getPersistData(uint8_t data[], uint8_t maxlen)
{
  uint8_t cnt32 = 0, cnt16 = 0, cnt8 = 0;
  uint8_t len = ArduinoDevice::getPersistData(data, maxlen);

  uint32_t *data32 = (uint32_t*)(data + len);
  
  uint16_t *data16 = (uint16_t*)(data32 + cnt32);

  data16[cnt16++]=downTime;
  data16[cnt16++]=shadeTime;

  uint8_t *data8 = (uint8_t*)(data16 + cnt16);
  
  data8[cnt8++]=detDown.getPin();
  data8[cnt8++]=pinMove;
  data8[cnt8++]=pinDirDown;

  return len + cnt32 * 4 + cnt16 * 2 + cnt8;
}

void WindowShadingDevice::clickHandler(int cnt, MultiClickDetector *det, void *obj, MultiClickDetector::EventType evtType) {
	
	WindowShadingDevice *wsd = reinterpret_cast<WindowShadingDevice*>(obj);
	if (wsd != nullptr) {
		Direction dir = (det == &wsd->detUp)?(Direction::UP):(Direction::DOWN);
		
		switch (evtType) {
			case MultiClickDetector::EventType::Click:
				wsd->clicked(cnt, dir);
				break;
			case MultiClickDetector::EventType::HoldStart:
				wsd->holdStart(cnt, dir);
				break;
			case MultiClickDetector::EventType::HoldFinished:
				wsd->holdFinished(cnt, dir);
				break;
		}
	}
}

void WindowShadingDevice::doWorkHighPrec() {
	detUp.doWorkHighPrec();
	detDown.doWorkHighPrec();
	
	if (status == Status::UNINIT || status == Status::IDLE)
		return;
		
	updateActuals();
	
	if(status != Status::MANUAL)
		handleRequest();
}

void WindowShadingDevice::setRequest(uint16_t pos, uint16_t shd) {
	if (SHADING_DEBUG)
		LOG_U("setRequest " << pos << " " << shd);

	reqPosition = (pos<=downTime?pos:downTime);
	reqShadow = (shd<=shadeTime?shd:shadeTime);

	handleRequest();
}

void WindowShadingDevice::setStatus(Status stat) {
	if (SHADING_DEBUG)
		LOG_U("setStatus " << (int)status << "->" << (int)stat);
	status = stat;
	lastUpdate = millis();
}

void WindowShadingDevice::handleRequest() {

	switch(status) {
		case Status::MANUAL:
			return;
	}

	Direction actDir = getMovingDirection();
	Direction reqDir = getRequestDirection();
	
	
	if (actDir == reqDir)
		return;
	
	if (reqDir == Direction::NONE) {
		stop();
		setStatus(Status::IDLE);
		return;
	}

	if (status == Status::UNINIT)
		setStatus(Status::INIT);
	else
		setStatus(Status::MOVING);
	start(reqDir); 
}

WindowShadingDevice::Direction WindowShadingDevice::getMovingDirection() {
	if (!ArduinoAndamBusUnit::pinValid(pinDirDown))
		return Direction::NONE;

	if (ArduinoAndamBusUnit::pinValid(pinDirDown) && digitalRead(pinDirDown) == LOW)
		return Direction::DOWN;

	if (ArduinoAndamBusUnit::pinValid(pinMove) && digitalRead(pinMove) == LOW)
		return Direction::UP;

	return Direction::NONE;
}

void WindowShadingDevice::updateActuals() {
	unsigned int now = millis();

	int diff; // in millis
	if (lastUpdate <= now)
		diff = now - lastUpdate;
	else
		diff = 0x10000 - (lastUpdate - now);
	
	if (diff < 20)
		return;

	int diffx = diff;
	
	if (getMovingDirection() == Direction::DOWN) {
		if (diff <= ((int32_t)shadeTime)*10 - actShadow) {
			actShadow += diff;
			diff = 0;
			setChanged(true);
		}
		else {
			diff -= (((int32_t)shadeTime)*10-actShadow);
			actShadow = shadeTime*10;
			setChanged(true);
		}
		
		if (diff <= ((int32_t)downTime)*10-actPosition) {
			actPosition += diff;
			setChanged(true);
		}
		else {
			actPosition = ((int32_t)downTime)*10;
			setChanged(true);
		}

	}

	if (getMovingDirection() == Direction::UP) {
		if (actShadow >= diff) {
			actShadow -= diff;
			diff = 0;
			setChanged(true);
		}
		else {
			diff -= actShadow;
			actShadow = 0;
			setChanged(true);
		}
		
		if (actPosition >= diff) {
			actPosition -= diff;
			setChanged(true);
		}
		else {
			actPosition = 0;
			setChanged(true);
		}
	}
	
	if (SHADING_DEBUG && now % 1000 < 30)
		LOG_U("update diff:" << iom::dec << diffx << " now=" << now << " actShadow=" << actShadow << " actPosition=" << actPosition << " lastUpdate=" << lastUpdate << " reqPosition=" << reqPosition);
	
	lastUpdate = now;
}

void WindowShadingDevice::updateRequestFromActual() {
  if (SHADING_DEBUG)
	LOG_U("updateRequestFromActual");

  reqPosition = (actPosition+5)/10;
  reqShadow = (actShadow+5)/10;
}	

void WindowShadingDevice::clicked(uint8_t cnt, Direction dir) {
	if (SHADING_DEBUG)
		LOG_U("clicked status=" << (int)status << " dir=" << (int)dir );
	
	switch (status) {
		case Status::MOVING:
		  setStatus(Status::IDLE);
		  stop();
		  updateRequestFromActual();
		  return;
		case Status::IDLE:
//			setStatus(Status::MOVING);
			
			if (dir == Direction::UP)
				setRequest(0, 0);
			if (dir == Direction::DOWN)
				setRequest(downTime, shadeTime);
			break;
		case Status::UNINIT:
			if (SHADING_DEBUG)
				LOG_U("init started");
			if (dir == Direction::UP) {
				actPosition = ((int32_t)downTime)*10;
				actShadow = ((int32_t)shadeTime)*10;
				setRequest(0, 0);
			}
			if (dir == Direction::DOWN) {
				actPosition = 0;
				actShadow = 0;
				setRequest(downTime, shadeTime);
			}
			break;
		case Status::INIT:
			setStatus(Status::UNINIT);
			stop();
			break;
		case Status::MANUAL:
		  if (SHADING_DEBUG)
		    LOG_U(F("INVALID STATUS"));
	}
}

void WindowShadingDevice::holdStart(uint8_t cnt, Direction dir) {
  switch(status) {
	  case Status::MOVING:
	  case Status::IDLE:
		setStatus(Status::MANUAL);
	  case Status::UNINIT:
		start(dir);
		break;
	  case Status::INIT:
		setStatus(Status::UNINIT);
		stop();
		break;
	  case Status::MANUAL:
		if (SHADING_DEBUG)
		  LOG_U(F("INVALID STATUS"));	  
  }
}

void WindowShadingDevice::holdFinished(uint8_t cnt, Direction dir) {
  stop();
  setStatus(Status::IDLE);
  updateRequestFromActual();
//  setRequest((actPosition+5)/10, (actShadow+5)/10);
}

void WindowShadingDevice::start(Direction dir) {
	if (!ArduinoAndamBusUnit::pinValid(pinDirDown) || !ArduinoAndamBusUnit::pinValid(pinMove))
		return;
	if (SHADING_DEBUG)
		LOG_U("start " << (int)dir);
	
	digitalWrite(pinDirDown, (dir==Direction::DOWN)?LOW:HIGH);
	delay(SHADING_DIR_DELAY_MS);
	digitalWrite(pinMove, LOW);
	lastUpdate = millis();
//	setRunning(true);
}
	
void WindowShadingDevice::stop() {
	if (SHADING_DEBUG)
		LOG_U("stop");
	
	if (ArduinoAndamBusUnit::pinValid(pinDirDown) && ArduinoAndamBusUnit::pinValid(pinMove)) {
		digitalWrite(pinMove, HIGH);
		delay(SHADING_DIR_DELAY_MS);
		digitalWrite(pinDirDown, HIGH);
	}
//	lastUpdate = 0;
//	setRunning(false);
}

WindowShadingDevice::Direction WindowShadingDevice::getRequestDirection() {
	if (onPosition() && onShade())
		return Direction::NONE;

	if (onPosition()) {
		if (actShadow < (long int)reqShadow*10) // more shadow request
			return Direction::DOWN;
		if (actShadow > (long int)reqShadow*10) // less  shadow request
			return Direction::UP;
		return Direction::NONE;
	}
	
	if (actPosition < (long int)reqPosition*10) // position down request
		return Direction::DOWN;
	if (actPosition > (long int)reqPosition*10) // position up request
		return Direction::UP;
		
	return Direction::NONE;
}

bool WindowShadingDevice::onPosition()
{
  return (abs((long int)actPosition-(long int)reqPosition*10) < SHADING_DIFF_THRESHOLD_MS);
}

bool WindowShadingDevice::onShade()
{
  return (abs((long int)actShadow-(long int)reqShadow*10) < SHADING_DIFF_THRESHOLD_MS);  
}
