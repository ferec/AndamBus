#include <AndamBusTypes.h>
#include <AndamBusUnit.h>

#include "ArduinoAndamBusUnit.h"
#include "BroadcastSocket.h"

//#include <EEPROM.h>
#include "util.h"

#define UNIT_ADDRESS 3
#define RS485_TRANSMIT_PIN 2

// #define ANDAMBUS_TEST

//#ifdef ArduinoMega // defined(__AVR_ATmega2560__) // Arduino Mega2560 config
//hwISerial hws(Serial2);
ArduinoAndamBusUnit abu(Serial2, RS485_TRANSMIT_PIN, UNIT_ADDRESS);
/*#else
#include <SoftwareSerial.h>

SoftwareSerial mySerial(4, 5);

ArduinoAndamBusUnit abu(mySerial, RS485_TRANSMIT_PIN, UNIT_ADDRESS);
#endif*/


void setup() {
//  int pinx = 15;
//  pinMode(pinx,INPUT_PULLUP);

//  *digitalPinToPCICR(pinx) |= (1<<digitalPinToPCICRbit(pinx));
//  *digitalPinToPCMSK(pinx) |= (1<<digitalPinToPCMSKbit(pinx));

//  sbi (PCICR, PCIE0);
//  sbi (PCMSK0, PCINT1);
  
//#ifdef ArduinoMega
  Serial.begin(115200);
//#endif


//  Serial.println("Andambus x");
  LOG_U(F("AndamBus version ") << ANDAMBUS_UNIT_SW_VERSION_MAJOR << "." << ANDAMBUS_UNIT_SW_VERSION_MINOR << "." << ANDAMBUS_UNIT_SW_VERSION_BUILD);
  LOG_U(F("AndamBus build date ") << ANDAMBUS_UNIT_SW_BUILD_DATE);

  abu.init();

#ifdef ANDAMBUS_TEST
  abu.createTestItems();
#endif

  LOG_U(F("mem:") << iom::dec << freeMemory());


  LOG_U(F("sizeof abu=") << iom::dec << sizeof(ArduinoAndamBusUnit));
}


long int iter = 0;


void loop() {
  
  if (iter % 100000 == 0)
    LOG_U(iom::dec << iter);

/*  #ifdef ANDAMBUS_TEST
  if (iter % 1000 == 0)
    abu.doTestConvert();

  #endif //ANDAMBUS_TEST*/
  iter++;

  
  abu.doWork();

}
