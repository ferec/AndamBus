#include "Midea.h"
#include "util.h"

Midea::Midea(uint8_t _id, uint8_t _pin, ArduinoAndamBusUnit *_abu):ArduinoDevice(_id, _pin),abu(_abu),irSend(),dpPot(),dpTarget(),tmTarget(0,0),tmOut(0,0),
  //portIdOnOff(0),portIdHeatCool(0),portIdTemp(0),
  tempTargetClima(MIDEA_DEFAULT_TEMP_TARGET_CLIMA),mode(mode_cool),pinOn(0),pinHeat(0),pinVentReq(0),pinVent(0),running(false)
{
}

Midea::~Midea() {
  unblockPin(pinOn);
  unblockPin(pinHeat);
  unblockPin(pinVentReq);  
  unblockPin(pinVent);  
}

/*uint8_t Midea::getPortCount() {
  return 2;
}

uint8_t Midea::getPortId(uint8_t idx) {
  switch(idx) {
    case 0:
      return portIdOnOff;
    case 1:
      return portIdHeatCool;
  }
  return ArduinoDevice::getPortId(idx);
}

void Midea::setPortId(uint8_t idx, uint8_t id) {
  switch(idx) {
    case 0:
      portIdOnOff = id;
      break;
    case 1:
      portIdHeatCool = id;
      break;
  }
}

VirtualPortType Midea::getPortType(uint8_t idx) {
  if (idx <= 1)
    return VirtualPortType::DIGITAL_OUTPUT;
  return VirtualPortType::NONE;
}

void Midea::setPortValue(uint8_t idx, int16_t value) {
//  LOG_U("setPortValue=" << value << " " << (int)idx);

  Modes m;
  int temp;
  switch(idx) {
    case 0:
      temp = mode==Modes::mode_cool?MIDEA_DEFAULT_TEMP_TARGET_CLIMA:MIDEA_DEFAULT_TEMP_TARGET_HEATING;
      tempTargetClima = temp; // hutesnel 17 fok 
//      fanSpeed = fan_auto;
      value!=0?turnON():turnOFF();
      break;
    case 1:
      temp = value==0?MIDEA_DEFAULT_TEMP_TARGET_CLIMA:MIDEA_DEFAULT_TEMP_TARGET_HEATING;
      tempTargetClima = temp; // futesnel 30 fok 
//      fanSpeed = fan_auto;
      m = value==0?Modes::mode_cool:Modes::mode_heat;
      mode = m;
      turnON();
      break;
  }
}*/

uint8_t Midea::calculateDigipot() {
  int16_t temp;
  if (!tmTarget.readTemp(temp))
    return 0;

  int8_t tempTarget = getTempTarget();
//  LOG_U(F("readTemp=") << iom::dec << temp << " tempTargetClima=" << tempTargetClima << " tempTarget=" << tempTarget);
  int tempCorr = (temp+50)/100 + tempTargetClima - tempTarget;

//  LOG_U(F("tempCorr=") << iom::dec << tempCorr);

  if (tempCorr < 17)
    tempCorr = 17;
  if (tempCorr > 31)
    tempCorr = 31;
  
  int val = map(tempCorr, 17, 31, 246, 67);

  return val;
}

void Midea::setMode() {
  // mode pins not set
  if (pinHeat == 0 || pinHeat >= MAX_VIRTUAL_PORTS || pinOn == 0 || pinOn >= MAX_VIRTUAL_PORTS)
    return;
    
  bool reqOn = digitalRead(pinOn) == HIGH;

  bool ventOnlyMode = false;
  bool ventOnly = false;
  
  int16_t tempOut;

  bool res = tmOut.readTemp(tempOut);

//  LOG_U("res=" << res << " tempOut=" << tempOut << " pinVent=" << (int)pinVent << " pinVentReq=" << (int)pinVentReq);
  if (pinVent != 0 && pinVent < MAX_VIRTUAL_PORTS && pinVentReq != 0 && pinVentReq < MAX_VIRTUAL_PORTS && res) {
    ventOnlyMode = digitalRead(pinVentReq) == HIGH;

    int8_t tempTarget = getTempTarget();

    bool ventRunning = digitalRead(pinVent)==LOW;

//    LOG_U("ventRunning="<<ventRunning << " tempTarget=" << tempTarget << " tempOut=" << tempOut);
    if ((tempOut+50)/100 + (ventRunning?2:5) <= tempTarget) {
      digitalWrite(pinVent, LOW);
      ventOnly = ventOnlyMode;
    } else
      digitalWrite(pinVent, HIGH);
  }

  if (ventOnly) {
    if (running)
      turnOFF();

    return;
  }
  
  Modes reqMode = (digitalRead(pinHeat) == HIGH)?mode_heat:mode_cool;

//  LOG_U("setmode " << reqMode << " " << reqOn);

  if (mode != reqMode) {
    mode = reqMode;

    if (running && reqOn) 
      turnON();
  }

  if (!running && reqOn) {
    turnON();
  }
  if (running && !reqOn) {
    turnOFF();
  }
}

void Midea::doWork() {
  setMode();
  
  int16_t value = calculateDigipot();

  int16_t pVal;
  if (!dpPot.readValue(pVal))
    return;

  if (pVal == value) {
  // skipping set value - already set
//    LOG_U(F("skip setting val=") << iom::dec << value << " vs " << pVal);
    return;
  }

//  LOG_U(F("Midea set digipot value=") << iom::dec << value);

  bool ret = dpPot.setValue(value);

  if (!ret)
    return;

//  LOG_U(F("Finished value=")<< value);
}

void Midea::unblockPin(uint8_t pin) {
  if (abu == nullptr)
    return;
  if (pin == 0 || pin >= MAX_VIRTUAL_PORTS)
    return;

  abu->unblockPin(pin);
}

bool Midea::blockPin(uint8_t pin) {
  if (abu == nullptr)
    return false;

  if (!abu->pinAvailable(pin))
    return false;

  abu->blockPin(pin);
  return true;
}

bool Midea::initPin(uint8_t pin) {
  return initPin(pin, INPUT_PULLUP);
}

bool Midea::initPin(uint8_t pin, uint8_t mode) {
  if (pin > 0 && pin <= MAX_VIRTUAL_PORTS)
    if (blockPin(pin)) {
      pinMode(pin, mode);
      return true;
    }
  return false;
}

uint8_t Midea::getPropertyList(ItemProperty propList[], uint8_t size) {
  int cnt = ArduinoDevice::getPropertyList(propList, size);

  uint8_t tid = tmTarget.getThermometerId();
  if (tid != 0) {
    propList[cnt].type = AndamBusPropertyType::ITEM_ID;
    propList[cnt].entityId = id;
    propList[cnt].propertyId = 0;
    propList[cnt++].value = tid;
  }

  tid = tmOut.getThermometerId();
  if (tid != 0) {
    propList[cnt].type = AndamBusPropertyType::ITEM_ID;
    propList[cnt].entityId = id;
    propList[cnt].propertyId = 1;
    propList[cnt++].value = tid;
  }

  tid = dpPot.getDevPortId();
//  LOG_U("tid=" << tid);
  if (tid != 0) {
    propList[cnt].type = AndamBusPropertyType::ITEM_ID;
    propList[cnt].entityId = id;
    propList[cnt].propertyId = 2;
    propList[cnt++].value = tid;
  }

  tid = dpTarget.getDevPortId();
//  LOG_U("tid=" << tid);
  if (tid != 0) {
    propList[cnt].type = AndamBusPropertyType::ITEM_ID;
    propList[cnt].entityId = id;
    propList[cnt].propertyId = 3;
    propList[cnt++].value = tid;
  }

  int8_t tempTarget = getTempTarget();
  if (tempTarget != MIDEA_DEFAULT_TEMP_TARGET) {
    propList[cnt].type = AndamBusPropertyType::TEMPERATURE;
    propList[cnt].entityId = id;
    propList[cnt].propertyId = 0;
    propList[cnt++].value = static_cast<int32_t>(tempTarget)*1000;
  }

  if (tempTargetClima != MIDEA_DEFAULT_TEMP_TARGET_CLIMA) {
    propList[cnt].type = AndamBusPropertyType::TEMPERATURE;
    propList[cnt].entityId = id;
    propList[cnt].propertyId = 1;
    propList[cnt++].value = static_cast<int32_t>(tempTargetClima)*1000;
  }

  if (pinOn != 0 && pinOn <= MAX_VIRTUAL_PORTS) {
    propList[cnt].type = AndamBusPropertyType::PIN;
    propList[cnt].entityId = id;
    propList[cnt].propertyId = 0;
    propList[cnt++].value = pinOn;
  }

  if (pinHeat != 0 && pinHeat <= MAX_VIRTUAL_PORTS) {
    propList[cnt].type = AndamBusPropertyType::PIN;
    propList[cnt].entityId = id;
    propList[cnt].propertyId = 1;
    propList[cnt++].value = pinHeat;
  }

  if (pinVentReq != 0 && pinVentReq <= MAX_VIRTUAL_PORTS) {
    propList[cnt].type = AndamBusPropertyType::PIN;
    propList[cnt].entityId = id;
    propList[cnt].propertyId = 2;
    propList[cnt++].value = pinVentReq;
  }

  if (pinVent != 0 && pinVent <= MAX_VIRTUAL_PORTS) {
    propList[cnt].type = AndamBusPropertyType::PIN;
    propList[cnt].entityId = id;
    propList[cnt].propertyId = 3;
    propList[cnt++].value = pinVent;
  }

  return cnt;
}

bool Midea::setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId)
{
  if (type == AndamBusPropertyType::ITEM_ID && propertyId == 0) {
    tmTarget.setThermometer(value);
  }

  if (type == AndamBusPropertyType::ITEM_ID && propertyId == 1) {
    tmOut.setThermometer(value);
  }

  if (type == AndamBusPropertyType::ITEM_ID && propertyId == 2) {
    dpPot.setDevPort(value);
  }

  if (type == AndamBusPropertyType::ITEM_ID && propertyId == 3) {
    dpTarget.setDevPort(value);
  }

  if (type == AndamBusPropertyType::TEMPERATURE && propertyId == 0) {
    setTempTarget(value/1000);
  }

  if (type == AndamBusPropertyType::TEMPERATURE && propertyId == 1) {
    tempTargetClima=value/1000;
  }

  if (type == AndamBusPropertyType::PIN && propertyId == 0) {
    if (pinOn != value) {
      unblockPin(pinOn);
      if (initPin(value))
        pinOn = value;
      else
        pinOn = 0;
    }
  }

  if (type == AndamBusPropertyType::PIN && propertyId == 1) {
    if (pinHeat != value) {
      unblockPin(pinHeat);
      if (initPin(value))
        pinHeat = value;
      else
        pinHeat = 0;
    }
  }

  if (type == AndamBusPropertyType::PIN && propertyId == 2) {
    if (pinVentReq != value) {
      unblockPin(pinVentReq);
      if (initPin(value))
        pinVentReq = value;
      else
        pinVentReq = 0;
    }
  }

  if (type == AndamBusPropertyType::PIN && propertyId == 3) {
    if (pinVent != value) {
      unblockPin(pinVent);
      if (initPin(value, OUTPUT))
        pinVent = value;
      else
        pinVent = 0;
    }
  }

  return ArduinoDevice::setProperty(type, value, propertyId);
}

uint8_t Midea::restore(uint8_t data[], uint8_t length)
{
  uint8_t cnt32 = 0, cnt16 = 0, cnt8 = 0;
  uint8_t used = ArduinoDevice::restore(data, length);
  uint32_t *data32 = (uint32_t*)(data + used);

  if (length - used >= 8) {
    uint32_t ah = data32[cnt32++];
    uint32_t al = data32[cnt32++];
    tmTarget.setThermometerAddress(ah, al);
  }

  if (length - used - cnt32*4 >= 8) {
    uint32_t ah = data32[cnt32++];
    uint32_t al = data32[cnt32++];
    tmOut.setThermometerAddress(ah, al);
  }

  uint16_t *data16 = (uint16_t*)(data32 + cnt32);

  if (length >= used + cnt32 * 4 + cnt16 * 2 + 2)
    tempTargetClima = data16[cnt16++];
  if (length >= used + cnt32 * 4 + cnt16 * 2 + 2)
    setTempTarget(data16[cnt16++]);
  
  uint8_t *data8 = (uint8_t*)(data16 + cnt16);

  if (length >= used + cnt32 * 4 + cnt16 * 2 + cnt8 + 2) {
    uint8_t dpin = data8[cnt8++];
    uint8_t idx = data8[cnt8++];
    dpPot.setDevPort(dpin, idx);
  }

  if (length >= used + cnt32 * 4 + cnt16 * 2 + cnt8 + 2) {
    uint8_t dpin = data8[cnt8++];
    uint8_t idx = data8[cnt8++];
    dpTarget.setDevPort(dpin, idx);
  }

  if (length >= used + cnt32 * 4 + cnt16 * 2 + cnt8 + 4) {
    pinOn=data8[cnt8++];
    pinHeat=data8[cnt8++];
    pinVentReq=data8[cnt8++];
    pinVent=data8[cnt8++];

    initPin(pinOn);
    initPin(pinHeat);
    initPin(pinVentReq);
    initPin(pinVent, OUTPUT);
/*    blockPin(pinOn);
    blockPin(pinHeat);
    blockPin(pinVent);*/
  } else {
//    LOG_U(F("not enough bytes for vent"));
  }

  return used + cnt32 * 4 + cnt16 * 2 + cnt8;
}


uint8_t Midea::getPersistData(uint8_t data[], uint8_t maxlen)
{
  uint8_t cnt32 = 0, cnt16 = 0, cnt8 = 0;
  uint8_t len = ArduinoDevice::getPersistData(data, maxlen);

  uint32_t *data32 = (uint32_t*)(data + len);
  data32[cnt32++] = tmTarget.getAddressH();
  data32[cnt32++] = tmTarget.getAddressL();
  data32[cnt32++] = tmOut.getAddressH();
  data32[cnt32++] = tmOut.getAddressL();

  int8_t tempTarget = getTempTarget();
  
  uint16_t *data16 = (uint16_t*)(data32 + cnt32);
  data16[cnt16++] = tempTargetClima;
  data16[cnt16++] = tempTarget;

  uint8_t *data8 = (uint8_t*)(data16 + cnt16);
  data8[cnt8++]=dpPot.getDevPin();
  data8[cnt8++]=dpPot.getDevPortIndex();
  data8[cnt8++]=dpTarget.getDevPin();
  data8[cnt8++]=dpTarget.getDevPortIndex();

  data8[cnt8++]=pinOn;
  data8[cnt8++]=pinHeat;
  data8[cnt8++]=pinVentReq;
  data8[cnt8++]=pinVent;

  return len + cnt32 * 4 + cnt16 * 2 + cnt8;
}

int8_t Midea::getTempTarget() {
  int16_t val;
  if (dpTarget.readValue(val))
    return val;
  return MIDEA_DEFAULT_TEMP_TARGET;
}

void Midea::setTempTarget(int8_t value) {
  dpTarget.setValue(value);
}

void Midea::turnON() {
  if (mode == mode_cool) {
    emit(0xBF, 0x00);
    LOG_U(F("turnon cool"));
  }
  else {
    emit(0xBF, 0xBC);
    LOG_U(F("turnon heat"));
  }
  running = true;
}

void Midea::turnOFF() {
  emit(0x7B, 0xE0);

  LOG_U(F("turnoff"));
  running=false;
}

void Midea::emitByte(uint8_t data){
    int i;

    uint8_t cur = data;
    uint8_t mask = 0x80;


//    LOG_U("emit byte " << (int)data);
    
    // Emit the first Byte
    for (i = 0;i < 8;i++) {
        irSend.mark(MIDEA_MARK);
        if (cur & mask)
            irSend.space(MIDEA_SPACE);
        else
            irSend.space(MIDEA_SPACE2);
        mask >>= 1;
    }

    // Neg bitwise this byte with his mask
    cur = ~data;
    mask = 0x80;

    // And emit the same byte negate
    for (i = 0;i < 8;i++) {
        irSend.mark(MIDEA_MARK);
        if (cur & mask)
            irSend.space(MIDEA_SPACE);
        else
            irSend.space(MIDEA_SPACE2);
        mask >>= 1;
    }
}

void Midea::emitLL(uint8_t cmd_byte1, uint8_t cmd_byte2) {
    // Start Communication
    irSend.mark(MIDEA_MARK_LIMIT);
    irSend.space(MIDEA_SPACE_LIMIT);
    
    // Emit Some bytes
    emitByte(MIDEA_START_BYTE);
    emitByte(cmd_byte1);
    emitByte(cmd_byte2);
  
    // End Communication
    irSend.mark(MIDEA_MARK_LIMIT2);
    irSend.space(MIDEA_SPACE_LIMIT);
}

void Midea::emit(uint8_t cmd_byte1, uint8_t cmd_byte2) {
    irSend.enableIROut(MIDEA_FREQUENCY);

    emitLL(cmd_byte1, cmd_byte2);
    emitLL(cmd_byte1, cmd_byte2);
}
