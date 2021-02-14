#include "Display.h"
#include "util.h"
#include "ArduinoAndamBusUnit.h"

Display::Display(uint8_t _id, uint8_t _pin, ArduinoAndamBusUnit *_abu): ArduinoDevice(_id, _pin),MultiClickDetector(_pin),lcd(DEFAULT_DISPLAY_I2C_SLAVE_ADDR,20,LCD_ROWS),lbls{0},initialized(false),
//  t_addrh{0},t_addrl{0},
devPin{0},portIdx{0},value{0},changed{0},portId{0},tBacklight(0),backlight(false),backlightPermanent(false),backlightPeriodSec(LCD_BACKLIGHT_TIMEOUT_SEC)
{
/*  for (int i=0;i<LCD_THERMOMETER_COUNT;i++) {
    tBusIndex[i] = 0xff;
    tDevIndex[i] = 0xff;
  }*/

  value[6]=-128;
  value[7]=-128;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Starting..."));
  lcd.backlight();
}

bool Display::setData(const char *data, uint8_t size) {
  if (!hasContent())
    lcd.clear();
    
  if (size >= 2) {
    uint8_t len = (size-2)>LCD_MAX_LABEL_LENGTH?LCD_MAX_LABEL_LENGTH:size-2;
    uint8_t row = data[0]%LCD_ROWS;
    if (row >= 0 && row < LCD_ROWS) {
      memcpy(lbls[row],data+2,len);
      lbls[row][len] = '\0';
      printLabel(row);
    }
  }
  
//  LOG_U("set data " << (int)size << " " << data);
}

void Display::printLabel(uint8_t row) {
    lcd.setCursor(0, row);
    lcd.print(F("        "));
    lcd.setCursor(0, row);
    lcd.print(lbls[row]);
}

void Display::init() {
  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  backlight=true;
  tBacklight = millis();

  lcd.clear();
  initialized=true;
  for (int row=0;row<LCD_ROWS;row++) {
    printLabel(row);
    changed[row] = true;
  }

  char buf[12];
  if (value[6]!=-128) {
    lcd.setCursor(LCD_VAL1_POSITION, 3);
    sprintf (buf, "%3i" LCD_DEGREE_CHAR, (int)value[6]);
    lcd.print(buf);
  }
  if (value[7]!=-128) {
    lcd.setCursor(LCD_VAL2_POSITION, 3);
    sprintf (buf, "%3i" LCD_DEGREE_CHAR, (int)value[7]);
    lcd.print(buf);
  }

}

void Display::refreshData() {
  for (int i=0;i<LCD_THERMOMETER_COUNT;i++) {
/*    if (tBusIndex[i] == 0xff && t_addrh[i] != 0 && t_addrl[i] != 0)
      if (!pairBusDev(t_addrh[i], t_addrl[i], tBusIndex[i], tDevIndex[i])) {
        LOG_U(F("Thermometer not paired"));
      }*/

    int16_t temp = 0;
    bool hasT = tm[i].readTemp(temp);
    if (hasT) {
	  int8_t v = (temp+50)/100;
	  
	  if (value[i] != v) {
		value[i]=v;
		changed[i/2] = true;
		setChanged(true);
	  }
    } 
  }
}

bool Display::hasContent() {
  for (int i=0;i<LCD_ROWS;i++)
    if (lbls[i][0] != 0)
      return true;

  return false;
}

void Display::refreshDisplayContent() {
  char buf[12];
  
  for (int row=0;row<LCD_ROWS;row++) {
      if (!changed[row])
        continue;

//      bool act1 = row<=2 && tBusIndex[row*2] < MAX_SECONDARY_BUSES && tDevIndex[row*2] != 0xff;
//      bool act2 = row<=2 && tBusIndex[row*2+1] < MAX_SECONDARY_BUSES && tDevIndex[row*2+1] != 0xff;
		bool act1 = row<=2 && tm[row*2].isLinked();
		bool act2 = row<=2 && tm[row*2+1].isLinked();

      if (!act1 && !act2)
        return;

      if (act1 && act2)
        sprintf (buf, "%3i" LCD_DEGREE_CHAR " %3i" LCD_DEGREE_CHAR, (int)value[row*2],(int)value[row*2+1]);
      if (act1 && !act2)
        sprintf (buf, "%3i" LCD_DEGREE_CHAR, (int)value[row*2]);
      if (!act1 && act2)
        sprintf (buf, "%3i" LCD_DEGREE_CHAR, (int)value[row*2+1]);

      if (act1)
        lcd.setCursor(LCD_VAL1_POSITION, row);
      else
        lcd.setCursor(LCD_VAL2_POSITION, row);
      lcd.print(buf);
      changed[row] = false;
  }
}

void Display::doWorkHighPrec() {
  MultiClickDetector::doWorkHighPrec();
}

void Display::handleBacklight() {
  if (!backlightPermanent && backlight && (millis() - tBacklight > backlightPeriodSec *1000 || millis() < tBacklight)) {
    lcd.noBacklight();
    backlight=false;
  }
  
}

void Display::doWork() {
  if (!initialized)
    init();

  handleBacklight();

  if (!hasContent()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Andam"));
    lcd.setCursor(0, 1);
    lcd.print(F("info@andam.sk"));
    return;
  }

//  LOG_U("prerefresh " << iom::dec << millis());
  refreshData();
  refreshDisplayContent();
//  LOG_U("postrefresh " << iom::dec << millis());
}

void Display::setPortValue(uint8_t idx, int16_t val) {
  if (idx >= LCD_VALUE_COUNT)
    return;

//  LOG_U("setting idx " << (int)idx << " value to " << val);
  int8_t v = val>127?127:(val<-128?-128:val);

  if (v == value[idx])
    return;

  setChanged(true);

  value[idx] = v;
  changed[idx/2]=true;

  char buf[12];
  
  if (idx == 6 || idx == 7) {
    lcd.setCursor(idx == 6?LCD_VAL1_POSITION:LCD_VAL2_POSITION, 3);
    sprintf (buf, "%3i" LCD_DEGREE_CHAR " ", (int)value[idx]);
    lcd.print(buf);

    if (!backlight) {
      backlight=true;
      lcd.backlight();
    }
    tBacklight=millis();
  }
  
}

uint8_t Display::getPortCount() {
  return LCD_VALUE_COUNT;
}

uint8_t Display::getPortId(uint8_t idx) {
  if (idx < LCD_VALUE_COUNT)
    return portId[idx];
  return ArduinoDevice::getPortId(idx);
}

void Display::setPortId(uint8_t idx, uint8_t id) {
  if (idx < LCD_VALUE_COUNT)
      portId[idx] = id;
}

VirtualPortType Display::getPortType(uint8_t idx) {
    return VirtualPortType::ANALOG_OUTPUT;
}

uint8_t Display::getPortIndex(uint8_t id) {
  for (int i=0;i<LCD_VALUE_COUNT;i++)
    if (portId[i] == id)
      return i;
  return 0xff;
}

int16_t Display::getPortValue(uint8_t idx) {
  if (idx < LCD_VALUE_COUNT)
    return value[idx];
  return 0;
}

void Display::clicked(uint8_t cnt) {
  if (cnt == 1) {
    if (backlight)
      lcd.noBacklight();
    else {
      lcd.backlight();
      tBacklight=millis();
    }
    backlight=!backlight;
    backlightPermanent=false;
  }
    
  if (cnt == 2) {
    if (!backlightPermanent) {
      lcd.backlight();
      backlight=true;
      backlightPermanent=true;
    } else {
      tBacklight=millis();
      backlightPermanent=false;
    }
  }
}

uint8_t Display::getPropertyList(ItemProperty propList[], uint8_t size) {
  int cnt = ArduinoDevice::getPropertyList(propList, size);

  if (cnt >= size)
    return size;

  if (backlightPeriodSec != LCD_BACKLIGHT_TIMEOUT_SEC) {
    propList[cnt].type = AndamBusPropertyType::PERIOD_SEC;
    propList[cnt].entityId = id;
    propList[cnt].propertyId = 0;
    propList[cnt++].value = backlightPeriodSec;

    if (cnt >= size)
      return size;
  }

  for (int i=0;i<LCD_THERMOMETER_COUNT;i++) {
    if (tm[i].configMissing())
      continue;
      
//    W1Slave *dev = getBusDeviceByIndex(tBusIndex[i], tDevIndex[i]);
    uint8_t tid = tm[i].getThermometerId();
  
    if (tid != 0) {
		propList[cnt].type = AndamBusPropertyType::ITEM_ID;
		propList[cnt].entityId = id;
		propList[cnt].propertyId = i;
		propList[cnt++].value = tid;
	}

    if (cnt >=size)
      break;
  }

  return cnt;
}

bool Display::setProperty(AndamBusPropertyType type, int32_t value, uint8_t propertyId) {
  if (type == AndamBusPropertyType::ITEM_ID && propertyId < LCD_THERMOMETER_COUNT) {
    LOG_U(F("setting bus dev to ") << iom::hex << "0x" << (uint32_t)value);
//    setThermometer(propertyId, value, t_addrh[propertyId], t_addrl[propertyId], tBusIndex[propertyId], tDevIndex[propertyId]);
	tm[propertyId].setThermometer(value);

//    LOG_U("prop " << iom::hex << t_addrh[propertyId] << t_addrl[propertyId]);
//    LOG_U("bus " << iom::hex << tBusIndex[propertyId] << " " << tDevIndex[propertyId]);
  }

  if (type == AndamBusPropertyType::PERIOD_SEC && propertyId == 0)
    backlightPeriodSec = value;

  return ArduinoDevice::setProperty(type, value, propertyId);
}

uint8_t Display::getPersistData(uint8_t data[], uint8_t maxlen)
{
  uint8_t cnt32 = 0, cnt16 = 0;
  uint8_t len = ArduinoDevice::getPersistData(data, maxlen);

  int needLen = LCD_THERMOMETER_COUNT*8 +sizeof(int)+ LCD_MAX_LABEL_LENGTH*LCD_ROWS;
  if (maxlen-len < needLen) {
    LOG_U(F("cannot persist display ") << needLen << " vs " << (maxlen-len));
    return len;
  }
      
  uint32_t *data32 = (uint32_t*)(data + len);
  for (int i=0;i<LCD_THERMOMETER_COUNT;i++) {
    data32[cnt32++] = tm[i].getAddressH();
    data32[cnt32++] = tm[i].getAddressL();
  }

  uint16_t *data16 = (uint16_t*)(data32 + cnt32);
  data16[cnt16++] = backlightPeriodSec;

  uint8_t *data8 = (uint8_t*)(data16 + cnt16);

  for (int i=0;i<LCD_ROWS;i++) {
    memcpy(data8+i*LCD_MAX_LABEL_LENGTH, lbls[i], LCD_MAX_LABEL_LENGTH);
  }

  data8[LCD_ROWS*LCD_MAX_LABEL_LENGTH] = (uint8_t)value[6];
  data8[LCD_ROWS*LCD_MAX_LABEL_LENGTH+1] = (uint8_t)value[7];

  return len + cnt32 * 4 + cnt16 * 2 + LCD_ROWS*LCD_MAX_LABEL_LENGTH+2;
}

uint8_t Display::restore(uint8_t data[], uint8_t length)
{
  LOG_U(F("restore display"));
  uint8_t cnt32 = 0, cnt16 = 0, bytes = 0;
  uint8_t used = ArduinoDevice::restore(data, length);

  uint32_t *data32 = (uint32_t*)(data + used);

  if (length >= used + LCD_THERMOMETER_COUNT*8)
    for (int i=0;i<LCD_THERMOMETER_COUNT;i++) {
      //t_addrh[i] = data32[cnt32++];
      //t_addrl[i] = data32[cnt32++];
	  uint32_t ah = data32[cnt32++];
	  uint32_t al = data32[cnt32++];
	  tm[i].setThermometerAddress(ah, al);
    }

  LOG_U(F("restored therms ") << (int)cnt32);

  uint16_t *data16 = (uint16_t*)(data32 + cnt32);

  if (length >= used + LCD_THERMOMETER_COUNT*8 + sizeof(int))
    backlightPeriodSec = data16[cnt16++];

  LOG_U(F("restored timeout ") << cnt16);

  uint8_t *data8 = (uint8_t*)(data16 + cnt16);

  if (length >= used + LCD_THERMOMETER_COUNT*8 + cnt16*sizeof(int) + LCD_ROWS*LCD_MAX_LABEL_LENGTH)
    for (int i=0;i<LCD_ROWS;i++) {
      memcpy(lbls[i], data8+i*LCD_MAX_LABEL_LENGTH, LCD_MAX_LABEL_LENGTH);
      bytes = LCD_ROWS*LCD_MAX_LABEL_LENGTH;
    }

  LOG_U(F("restored labels ") << (int)bytes);

  value[6] = (int8_t)data8[LCD_ROWS*LCD_MAX_LABEL_LENGTH];
  value[7] = (int8_t)data8[LCD_ROWS*LCD_MAX_LABEL_LENGTH+1];

  return used + LCD_THERMOMETER_COUNT*8 + cnt16 * sizeof(int) + bytes + 2;
}

void Display::holdStarted(uint8_t cnt) {
  LOG_U(F("persist"));

  if (BusDeviceHelper::getAbu() != nullptr) {
    BusDeviceHelper::getAbu()->doPersist();
    if (backlight) {
      lcd.noBacklight();
      delay(200);
    }

    lcd.backlight();
    delay(200);
    lcd.noBacklight();

    if (backlight) {
      delay(200);
      lcd.backlight();
    }
  }
}
