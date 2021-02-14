#include "util.h"
#include "Arduino.h"

Logger logger;

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}

const ItemProperty* findPropertyByType(const ItemProperty props[], uint8_t size, AndamBusPropertyType type) {
    for (int i=0; i<size;i++) {
        if (props[i].type == type)
            return &props[i];
    }

    return nullptr;
}

void ntohHdr(AndamBusFrameHeader &hdr) {
    hdr.slaveAddress = ntohs(hdr.slaveAddress);
    hdr.counter = ntohs(hdr.counter);
    //hdr.crc32 = ntohl(hdr.crc32);
    hdr.payloadLength = ntohl(hdr.payloadLength);
    hdr.magicWord = ntohl(hdr.magicWord);
}

void htonHdr(AndamBusFrameHeader &hdr) {
    hdr.slaveAddress = htons(hdr.slaveAddress);
    hdr.counter = htons(hdr.counter);
//    hdr.crc32 = htonl(hdr.crc32);
    hdr.payloadLength = htonl(hdr.payloadLength);
    hdr.magicWord = htonl(hdr.magicWord);
}

void htonResp(AndamBusFrameResponse &resp) {
    if (resp.responseType == AndamBusResponseType::OK_DATA) {
        switch (resp.typeOkData.content) {
            case ResponseContent::NONE:
            case ResponseContent::RAWDATA:
            case ResponseContent::SECONDARY_BUS:
            case ResponseContent::VIRTUAL_DEVICE:
                break;
            case ResponseContent::PROPERTY:
                for (int i=0;i<resp.typeOkData.itemCount;i++)
                    htonProp(resp.typeOkData.responseData.props[i]);
                break;
            case ResponseContent::VIRTUAL_PORT:
                for (int i=0;i<resp.typeOkData.itemCount;i++)
                    resp.typeOkData.responseData.portItems[i].value = htonl(resp.typeOkData.responseData.portItems[i].value);
                break;
            case ResponseContent::VALUES:
                for (int i=0;i<resp.typeOkData.itemCount;i++)
                    resp.typeOkData.responseData.portItems[i].value = htonl(resp.typeOkData.responseData.portValues[i].value);
                break;
            case ResponseContent::MIXED:
                VirtualDevice *vdl = resp.typeOkData.responseData.deviceItems;
                VirtualPort *vpl = reinterpret_cast<VirtualPort*>(vdl+resp.typeOkData.itemCount);

                for (int i=0;i<resp.typeOkData.itemCount2;i++)
                    vpl[i].value = htonl(vpl[i].value);

                ItemProperty *ipl = reinterpret_cast<ItemProperty*>(vpl+resp.typeOkData.itemCount2);

                for (int i=0;i<resp.typeOkData.itemCount3;i++)
                    htonProp(ipl[i]);
                break;

        }
    }
}

void ntohProp(ItemProperty &prop) {
	prop.value = ntohl(prop.value);
	prop.type = static_cast<AndamBusPropertyType>(ntohs(static_cast<uint16_t>(prop.type)));
}	

void htonProp(ItemProperty &prop) {
	prop.value = htonl(prop.value);
	prop.type = static_cast<AndamBusPropertyType>(htons(static_cast<uint16_t>(prop.type)));
}	

void ntohCmd(AndamBusFrameCommand &cmd) {
    if (cmd.command == AndamBusCommand::RESTORE_CONFIG)
        return;

    for (int i=0;i<cmd.propertyCount;i++)
        ntohProp(cmd.props[i]);
}

uint8_t getItemSize(ResponseContent rc) {
    switch (rc) {
    case ResponseContent::PROPERTY:
        return sizeof(ItemProperty);
    case ResponseContent::NONE:
    case ResponseContent::RAWDATA:
    case ResponseContent::MIXED:
        return 0;
    case ResponseContent::SECONDARY_BUS:
        return sizeof(SecondaryBus);
    case ResponseContent::VIRTUAL_DEVICE:
        return sizeof(VirtualDevice);
    case ResponseContent::VIRTUAL_PORT:
        return sizeof(VirtualPort);
    case ResponseContent::VALUES:
        return sizeof(ItemValue);
    }
    return 0;
}

void hexdump(const unsigned char *buf, int len) {
//  char obuf[128];
//  int pos = 0;
  
  const static char msgSeparator[] PROGMEM = "-------";
  
  LOG_U((const __FlashStringHelper*)msgSeparator);
  for (int i=0;i<len;i++) {
/*    obuf[pos++] = ' ';
    obuf[pos++] = '0';
    obuf[pos++] = 'x';*/
	if (i%16 == 0)
		LOGGER_PRINT(F("\n"));
	LOGGER_PRINT(F(" 0x"));
	if (buf[i] < 16)
		LOGGER_PRINT('0');
    LOGGER_PRINT2(((unsigned int)buf[i])%0x100,HEX);
  }
  LOGGER_PRINT(F("\n"));
  LOG_U((const __FlashStringHelper*)msgSeparator );
  
}


Logger::Logger():hex(false) {
}

Logger& Logger::operator  << (uint32_t i) {
  if (hex) {
    LOGGER_PRINT2(i,HEX);
  }
  else
    LOGGER_PRINT2(i,DEC);
   
  return *this;
}

Logger& Logger::operator  << (int32_t i) {
  if (hex) {
    LOGGER_PRINT2(i,HEX);
  }
  else
    LOGGER_PRINT2(i,DEC);
   
  return *this;
}

Logger& Logger::operator  << (int16_t i) {
  if (hex) {
    LOGGER_PRINT2(i,HEX);
  }
  else
    LOGGER_PRINT2(i,DEC);
   
  return *this;
}

Logger& Logger::operator  << (uint16_t i) {
  if (hex) {
    LOGGER_PRINT2(i,HEX);
  }
  else
    LOGGER_PRINT2(i,DEC);
   
  return *this;
}	

Logger& Logger::operator  << (void *s) {
	LOGGER_PRINT2((unsigned int)s,HEX);
}

Logger& Logger::operator  << (const iom m) {
/*  if (m==iom::hex)
	hex = true;
  else {
	hex = false;
	Serial.print("set dec ");
	Serial.print(hex);
  }*/
  
//  manip = m;
  
  hex = m==iom::hex;
  return *this;

/*  Serial.print("set hex ");
  Serial.print((int)m);
  Serial.print(" ");
  Serial.print(hex);
  Serial.print(" ");*/
}

Logger& Logger::operator << (const char *s) {
  LOGGER_PRINT(s);
  return *this;
}

Logger& Logger::operator  << (const __FlashStringHelper *s) {
  LOGGER_PRINT(s);
  return *this;
}	

/*Logger& Logger::operator  << (const unsigned char *s) {
  Serial.print((const char*)s);
  return *this;

}*/

//char cmdStringBuf[10];

const __FlashStringHelper* commandString(AndamBusCommand cmd) {

    switch (cmd) {
        case AndamBusCommand::NONE: return F("NONE");
        case AndamBusCommand::METADATA: return F("METADATA");
        case AndamBusCommand::GET_DATA: return F("GET_DATA");
        case AndamBusCommand::SET_DATA: return F("SET_DATA");
        case AndamBusCommand::PERSIST: return F("PERSIST");
        case AndamBusCommand::RESET: return F("RESET");
        case AndamBusCommand::DETECT_SLAVE: return F("DETECT_SLAVE");
        case AndamBusCommand::SECONDARY_BUS_CREATE: return F("SECONDARY_BUS_CREATE");
        case AndamBusCommand::SECONDARY_BUS_DETECT: return F("SECONDARY_BUS_DETECT");
        case AndamBusCommand::SECONDARY_BUS_COMMAND: return F("SECONDARY_BUS_COMMAND");
        case AndamBusCommand::SECONDARY_BUS_REMOVE: return F("SECONDARY_BUS_REMOVE");
        case AndamBusCommand::SECONDARY_BUS_LIST: return F("SECONDARY_BUS_LIST");
        case AndamBusCommand::VALUES_CHANGED: return F("VALUES_CHANGED");
        case AndamBusCommand::VIRTUAL_DEVICE_CREATE: return F("VIRTUAL_DEVICE_CREATE");
        case AndamBusCommand::VIRTUAL_DEVICE_REMOVE: return F("VIRTUAL_DEVICE_REMOVE");
        case AndamBusCommand::VIRTUAL_DEVICE_CHANGE: return F("VIRTUAL_DEVICE_CHANGE");
        case AndamBusCommand::VIRTUAL_DEVICE_LIST: return F("VIRTUAL_DEVICE_LIST");

        case AndamBusCommand::VIRTUAL_PORT_CREATE: return F("VIRTUAL_PORT_CREATE");
        case AndamBusCommand::VIRTUAL_PORT_REMOVE: return F("VIRTUAL_PORT_REMOVE");
        case AndamBusCommand::VIRTUAL_PORT_SET_VALUE: return F("VIRTUAL_PORT_SET_VALUE");
        case AndamBusCommand::VIRTUAL_PORT_LIST: return F("VIRTUAL_PORT_LIST");
        case AndamBusCommand::PROPERTY_LIST: return F("PROPERTY_LIST");
        case AndamBusCommand::VIRTUAL_ITEMS_CREATED: return F("VIRTUAL_ITEMS_CREATED");
        case AndamBusCommand::VIRTUAL_ITEMS_REMOVED: return F("VIRTUAL_ITEMS_REMOVED");

//            default:
    }
	
//	itoa((int)cmd, cmdStringBuf, 10);
    return nullptr;
}
