#ifndef UTIL_H
#define UTIL_H

#include "AndamBusTypes.h"
#include <WString.h>

#define BB_TRUE(bp,bb)    bp |= bb
#define BB_FALSE(bp,bb)   bp &= ~(bb)
#define BB_READ(bp,bb)    bool(bp & bb)

#define LOG_U(x) { logger << x << "\n"; }

#ifdef DEBUG
#undef DEBUG
#endif

#define DEBUG(x)
#define ERROR(x) LOG_U("ERR:" << x);


//#if defined(__AVR_ATmega2560__) // Arduino Mega2560 config
#define LOGGER_PRINT(msg) Serial.print(msg)
#define LOGGER_PRINT2(msg,mode) Serial.print(msg,mode)
/*#else
#define LOGGER_PRINT(msg)
#define LOGGER_PRINT2(msg,mode)
#endif*/


#ifndef htons
#define htons(x) ( ((x)<< 8 & 0xFF00) | \
                   ((x)>> 8 & 0x00FF) )
#endif

#ifndef ntohs
#define ntohs(x) htons(x)
#endif

#ifndef htonl
#define htonl(x) ( ((uint32_t)(x)<<24 & 0xFF000000UL) | \
                   ((uint32_t)(x)<< 8 & 0x00FF0000UL) | \
                   ((x)>> 8 & 0x0000FF00UL) | \
                   ((x)>>24 & 0x000000FFUL) )
#endif

#ifndef ntohl
#define ntohl(x) htonl(x)
#endif


int freeMemory();

void ntohProp(ItemProperty &prop);
void htonProp(ItemProperty &prop);

void ntohHdr(AndamBusFrameHeader &hdr);
void htonHdr(AndamBusFrameHeader &hdr);
void ntohCmd(AndamBusFrameCommand &cmd);
void htonResp(AndamBusFrameResponse &resp);

const char msgNewLine[] PROGMEM = "\n";
const __FlashStringHelper* commandString(AndamBusCommand cmd);
uint8_t getItemSize(ResponseContent rc);

const ItemProperty* findPropertyByType(const ItemProperty props[], uint8_t size, AndamBusPropertyType type);

void hexdump(const unsigned char *buf, int len);

enum class iom:char { dec,hex };

class Logger {
  public:
    Logger();
    Logger& operator  << (uint32_t x);
    Logger& operator  << (uint16_t x);
    Logger& operator  << (int32_t x);
    Logger& operator  << (int16_t x);
    Logger& operator  << (void *s);
//    Logger& operator  << (const int x);
    Logger& operator  << (const char *s);
	Logger& operator  << (const __FlashStringHelper *textPtr);
//    Logger& operator  << (const unsigned char *s);
    Logger& operator  << (const iom m);

  private:
    bool hex;
};

extern Logger logger;

#endif
