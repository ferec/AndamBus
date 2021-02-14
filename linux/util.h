#ifndef UTIL_H
#define UTIL_H



#include "shared/AndamBusTypes.h"
#include "shared/AndamBusExceptions.h"
#include "ChangeListener.h"

#include <string>
#include <cstring>
#include <vector>
#include <map>

class AndamBusSlave;

enum class LogLevel:uint8_t { ERROR, WARNING, INFO, DEBUG, FINE1, FINE2 };
typedef void (*fnLog)(LogLevel level, const std::string &msg);

extern LogLevel logLevel;

#define AB_DEBUG(msg) AB_LOG(LogLevel::DEBUG, msg)
#define AB_INFO(msg) AB_LOG(LogLevel::INFO, msg)
#define AB_WARNING(msg) AB_LOG(LogLevel::WARNING, msg)
#define AB_ERROR(msg) AB_LOG(LogLevel::ERROR, msg)

#define LOG_U(x)

#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_MEGA2560)
#include "arduino/ArduinoLogger.h"
#else
#include <sstream>
#include <netinet/in.h>
#define AB_LOG(lvl,msg)   if (lvl<=logLevel) {  stringstream LLLss; \
                LLLss << msg; \
                Log(lvl, LLLss.str()); }

#endif // defined


const char* LogLevelString(LogLevel lvl);

void setLogLevel(LogLevel lvl);

void setDebugCallback(fnLog logger);

void Log(LogLevel lvl, const std::string &msg);

void defaultLogger(LogLevel level, const std::string &msg);

void hexdump(const char *buf, uint16_t size);
void hexdump(const char *buf, uint16_t size, LogLevel ll);

void printFrame(AndamBusFrame &frm);
std::string unitInfoString(uint16_t addr, std::string sHwType, SlaveHwType hwType, std::string sSwVersionString, std::string sApiVersionString);

std::string commandString(AndamBusCommand cmd);

std::string responseTypeString(AndamBusResponseType rtp);
std::string responseContentString(ResponseContent cont);
const std::string responseErrorString(AndamBusCommandError err, AndamBusCommand cmd);
std::string virtualDeviceTypeString(VirtualDeviceType type);
std::string virtualPortTypeString(VirtualPortType type);
std::string virtualPortTypeStringShort(VirtualPortType type);
std::string propertyTypeString(AndamBusPropertyType type);
std::string metadataTypeString(MetadataType type);
const std::string itemTypeString(ChangeListener::ItemType type);

std::string getDevicePropertyString(VirtualDeviceType devType, AndamBusPropertyType propType, uint8_t idx);

char* concat(const char* s1, const char *s2);

std::vector<std::string> split(const std::string& str, const std::string& delim);

const char* int_to_string(int i);

void nonblock_stdin();
void blocking_stdin();

uint64_t stripLongAddress(uint64_t);

const ItemProperty* findPropertyByType(const ItemProperty props[], uint8_t size, AndamBusPropertyType type);
void ntohCmd(AndamBusFrameCommand &cmd);
void htonCmd(AndamBusFrameCommand &cmd);
void ntohResp(AndamBusFrameResponse &resp);
void htonResp(AndamBusFrameResponse &resp);
void ntohHdr(AndamBusFrameHeader &hdr);
void htonHdr(AndamBusFrameHeader &hdr);
void ntohProp(ItemProperty &prop);
void htonProp(ItemProperty &prop);

#endif // UTIL_H
