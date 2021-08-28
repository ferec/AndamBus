#include "ClickSwitchDevice.h"
#include "util.h"

ClickSwitchDevice::ClickSwitchDevice(uint8_t _id, uint8_t _pin, ArduinoAndamBusUnit *_abu): ArduinoDevice(_id, _pin),MultiClickDetector(_pin),
	//highLogic(false),value1(false),value2(false),value3(false),
	boolPack(0),abu(_abu),
	portId1Click(0),portId2Click(0),portId3Click(0),
	tmOut1(0),tmOut2(0),tmOut3(0),tmOut(0),tmLastUpdate(0)	
{
}

uint8_t ClickSwitchDevice::getPortCount() {
  return 3;
}

uint8_t ClickSwitchDevice::getPortId(uint8_t idx) {
  if (idx == 0)
    return portId1Click;
  if (idx == 1)
    return portId2Click;
  if (idx == 2)
    return portId3Click;
  return 0;
}

void ClickSwitchDevice::setPortId(uint8_t idx, uint8_t id) {
  if (idx == 0)
    portId1Click = id;
  if (idx == 1)
    portId2Click = id;
  if (idx == 2)
    portId3Click = id;
}

VirtualPortType ClickSwitchDevice::getPortType(uint8_t idx) {
  return VirtualPortType::DIGITAL_INPUT;
}

uint8_t ClickSwitchDevice::getPortIndex(uint8_t id) {
  if (portId1Click == id)
    return 0;
  if (portId2Click == id)
    return 1;
  if (portId3Click == id)
    return 2;
  return 0xff;
}

int16_t ClickSwitchDevice::getPortValue(uint8_t idx) {
  if (idx > 2)
    return 0;

//  bool highLogic = BB_READ(boolPack, CLICKSW_HIGHLOGIC);

//  setChanged(false);
  
  DevPortHelper &dp = idx==0?dp1Click:(idx==1?dp2Click:dp3Click);
  int16_t val;
  if (dp.readValue(val)) {
	LOG_U("readValue=" << val);
    return (val!=0);
  }
  
  if (abu != nullptr)
	abu->endInterrupt();
  
  return (boolPack>>(idx+1))&1;
}

uint8_t ClickSwitchDevice::getPropertyList(ItemProperty propList[], uint8_t size) {
  int cnt = ArduinoDevice::getPropertyList(propList, size);

  uint8_t tid = dp1Click.getDevPortId();
  if (tid != 0) {
    propList[cnt].type = AndamBusPropertyType::ITEM_ID;
    propList[cnt].entityId = id;
    propList[cnt].propertyId = 0;
    propList[cnt++].value = tid;
  }

  tid = dp2Click.getDevPortId();
  if (tid != 0) {
    propList[cnt].type = AndamBusPropertyType::ITEM_ID;
    propList[cnt].entityId = id;
    propList[cnt].propertyId = 1;
    propList[cnt++].value = tid;
  }

  tid = dp3Click.getDevPortId();
  if (tid != 0) {
    propList[cnt].type = AndamBusPropertyType::ITEM_ID;
    propList[cnt].entityId = id;
    propList[cnt].propertyId = 2;
    propList[cnt++].value = tid;
  }

  propList[cnt].type = AndamBusPropertyType::PERIOD_SEC;
  propList[cnt].entityId = id;
  propList[cnt].propertyId = 0;
  propList[cnt++].value = tmOut;
	
/*  propList[cnt].type = AndamBusPropertyType::HIGHLOGIC;
  propList[cnt].entityId = id;
  propList[cnt].propertyId = 0;
  propList[cnt++].value = BB_READ(boolPack, CLICKSW_HIGHLOGIC);*/

  return cnt;
}

bool ClickSwitchDevice::setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId)
{
  if (type == AndamBusPropertyType::ITEM_ID && propertyId >= 0 && propertyId <= 2) {
    DevPortHelper &dp = propertyId==0?dp1Click:(propertyId==1?dp2Click:dp3Click);

    return dp.setDevPort(value);
  }


  if (type == AndamBusPropertyType::PERIOD_SEC && propertyId == 0) {
	tmOut = value;
  }	
  
/*  if (type == AndamBusPropertyType::HIGHLOGIC && propertyId == 0) {
	value==0?BB_FALSE(boolPack,CLICKSW_HIGHLOGIC):BB_TRUE(boolPack,CLICKSW_HIGHLOGIC);
//	highLogic = value;
  }*/

  return ArduinoDevice::setProperty(type, value, propertyId);
}

void ClickSwitchDevice::holdStarted(uint8_t cnt) {
  if (cnt > 0)
    return;

  int val = 0;
  
  dp1Click.setValue(val);
  dp2Click.setValue(val);
  dp3Click.setValue(val);

  LOG_U("val=" << val);
  setValue(0, val);
  setValue(1, val);
  setValue(2, val);
  
  setChanged(true);
}

void ClickSwitchDevice::clicked(uint8_t cnt) {
  if (cnt > 3)
    return;

//	LOG_U("clicked");
  setChanged(true);

  int16_t val;
  DevPortHelper &dp = cnt==1?dp1Click:(cnt==2?dp2Click:dp3Click);
  if (!dp.readValue(val)) {
	val = (boolPack>>(cnt))&1;
	setValue(cnt-1, !val);
	
	if (abu != nullptr)
		abu->startInterrupt();
	
//	LOG_U("boolPack=0x" << iom::hex << boolPack);
    return;
  }

//  LOG_U("readValue=" << val);

  bool bval = val!=0;
  dp.setValue(!bval);
}

void ClickSwitchDevice::handleTimeout(uint16_t &to, uint8_t idx, uint32_t now) {
	if (to > 0) {
		uint16_t diff = now - tmLastUpdate;
		if (to <= diff) {
			setValue(idx, false);
//			LOG_U("timeout after " << diff << " on " << (int)idx << " to=" << to);
		}
		else
			to -= diff;
	}
}

void ClickSwitchDevice::doWork() {
	uint32_t now = millis()/1000;
	
	if (tmOut > 0 && now > tmLastUpdate && tmLastUpdate != 0) {
		handleTimeout(tmOut1, 0, now);
		handleTimeout(tmOut2, 1, now);
		handleTimeout(tmOut3, 2, now);
	}

	tmLastUpdate = now;
}
  
void ClickSwitchDevice::doWorkHighPrec() {
  MultiClickDetector::doWorkHighPrec();
}

void ClickSwitchDevice::setValue(uint8_t idx, bool value) {
  if (idx >= 3)
	  return;
  
  if (value == true) {
	  if (idx==0)
		  tmOut1 = tmOut;
	  if (idx==1)
		  tmOut2 = tmOut;
	  if (idx==2)
		  tmOut3 = tmOut;
	  tmLastUpdate = millis()/1000;
  } else {
	  if (idx==0)
		  tmOut1 = 0;
	  if (idx==1)
		  tmOut2 = 0;
	  if (idx==2)
		  tmOut3 = 0;
  }
  
  if (value)
	boolPack |= (CLICKSW_VALUE1<<idx);
  else
	boolPack &= ~(CLICKSW_VALUE1<<idx);
  setChanged(true);
}

uint8_t ClickSwitchDevice::getPersistData(uint8_t data[], uint8_t maxlen)
{
  uint8_t cnt32=0, cnt16=0, cnt8=0;
  uint8_t used = ArduinoDevice::getPersistData(data, maxlen);
  
  uint32_t *data32 = (uint32_t*)(data + used);

  uint16_t *data16 = (uint16_t*)(data32 + cnt32);

  data16[cnt16++] = tmOut;

  uint8_t *data8 = (uint8_t*)(data16 + cnt16);

  
  data8[cnt8++] = dp1Click.getDevPin();
  data8[cnt8++] = dp1Click.getDevPortIndex();
  data8[cnt8++] = dp2Click.getDevPin();
  data8[cnt8++] = dp2Click.getDevPortIndex();
  data8[cnt8++] = dp3Click.getDevPin();
  data8[cnt8++] = dp3Click.getDevPortIndex();
//  data8[cnt8++] = BB_READ(boolPack, CLICKSW_HIGHLOGIC);

  return used+cnt32*sizeof(uint32_t)+cnt16*sizeof(uint16_t) + cnt8;
}

uint8_t ClickSwitchDevice::restore(uint8_t data[], uint8_t length)
{
  uint8_t cnt32=0, cnt16=0,cnt8=0;
  uint8_t used = ArduinoDevice::restore(data, length);
  
  uint32_t *data32 = (uint32_t*)(data + used);

  uint16_t *data16 = (uint16_t*)(data32 + cnt32);

  if (length >= used+cnt32*4+cnt16*2+8) {
	  tmOut = data16[cnt16++];
  }

  uint8_t *data8 = (uint8_t*)(data16 + cnt16);

  if (length >= used+cnt32*4+cnt16*2+cnt8+6) {
  uint8_t ppin = data8[cnt8++];
  uint8_t pidx = data8[cnt8++];
  dp1Click.setDevPort(ppin, pidx);
  ppin = data8[cnt8++];
  pidx = data8[cnt8++];
  dp2Click.setDevPort(ppin, pidx);
  ppin = data8[cnt8++];
  pidx = data8[cnt8++];
  dp3Click.setDevPort(ppin, pidx);
    
  }

/*  if (length >= used+cnt32*4+cnt16*2+cnt8+1) {
	  data8[cnt8++]==0?BB_FALSE(boolPack,CLICKSW_HIGHLOGIC):BB_TRUE(boolPack,CLICKSW_HIGHLOGIC);
  }*/

  return used+cnt32*4+cnt16*2+cnt8;
}
