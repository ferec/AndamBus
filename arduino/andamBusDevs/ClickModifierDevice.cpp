#include "ClickModifierDevice.h"
#include "util.h"

ClickModifierDevice::ClickModifierDevice(uint8_t _id, uint8_t _pin, ArduinoAndamBusUnit *_abu): ArduinoDevice(_id, _pin),MultiClickDetector(_pin),step(DEFAULT_CLICK_MOD_DEV_STEP) {
  pinMode(_pin, INPUT_PULLUP);
}

void ClickModifierDevice::clicked(uint8_t cnt) {
  //LOG_U("clicked " << (int)cnt);
//  if (cnt == 1) {
    int16_t val;
    if (dpTrg.readValue(val))
      dpTrg.setValue(val+cnt*step);
//  }
}
  
void ClickModifierDevice::doWorkHighPrec() {
  MultiClickDetector::doWorkHighPrec();
}

uint8_t ClickModifierDevice::getPropertyList(ItemProperty propList[], uint8_t size) {
  int cnt = ArduinoDevice::getPropertyList(propList, size);

  uint8_t tid = dpTrg.getDevPortId();
//  LOG_U("tid="<<(int)tid << " getDevPin=" << (int)dpTrg.getDevPin() << " getDevPortIndex=" << (int)dpTrg.getDevPortIndex());
  if (tid != 0) {
    propList[cnt].type = AndamBusPropertyType::ITEM_ID;
    propList[cnt].entityId = id;
    propList[cnt].propertyId = 0;
    propList[cnt++].value = tid;
  }

  if (step != DEFAULT_CLICK_MOD_DEV_STEP) {
    propList[cnt].type = AndamBusPropertyType::STEP;
    propList[cnt].entityId = id;
    propList[cnt].propertyId = 0;
    propList[cnt++].value = step;
  }
  
  return cnt;
}

bool ClickModifierDevice::setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId)
{
  if (type == AndamBusPropertyType::ITEM_ID && propertyId == 0) {
    dpTrg.setDevPort(value);
  }

  if (type == AndamBusPropertyType::STEP && propertyId == 0) {
    step = value;
  }

  return ArduinoDevice::setProperty(type, value, propertyId);
}

uint8_t ClickModifierDevice::getPersistData(uint8_t data[], uint8_t maxlen)
{
  uint8_t cnt32=0, cnt16=0, cnt8=0;
  uint8_t len = ArduinoDevice::getPersistData(data, maxlen);
  
  uint32_t *data32 = (uint32_t*)(data + len);

  uint16_t *data16 = (uint16_t*)(data32 + cnt32);

  uint8_t *data8 = (uint8_t*)(data16 + cnt16);
  
  data8[cnt8++] = dpTrg.getDevPin();
  data8[cnt8++] = dpTrg.getDevPortIndex();
  data8[cnt8++] = step;

  return len+cnt32*sizeof(uint32_t)+cnt16*sizeof(uint16_t) + cnt8;
}

uint8_t ClickModifierDevice::restore(uint8_t data[], uint8_t length)
{
  uint8_t cnt32=0, cnt16=0,cnt8=0;
  uint8_t used = ArduinoDevice::restore(data, length);
  
  uint32_t *data32 = (uint32_t*)(data + used);

  uint16_t *data16 = (uint16_t*)(data32 + cnt32);
  uint8_t *data8 = (uint8_t*)(data16 + cnt16);

  uint8_t ppin = data8[cnt8++];
  uint8_t pidx = data8[cnt8++];
  dpTrg.setDevPort(ppin, pidx);

  if (length >= used+cnt32*4+cnt16*2+cnt8+1)
    step = data8[cnt8++];

  return used+cnt32*4+cnt16*2+cnt8;
}
