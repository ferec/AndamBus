#include "BroadcastSocket.h"

#include "util.h"

const uint32_t syncNint = htonl(ANDAMBUS_MAGIC_WORD);
const char *syncN = reinterpret_cast<const char*>(&syncNint);

BroadcastSocket::BroadcastSocket(HardwareSerial &ser, uint8_t _pinTransmit):synchronized(false),syncIndex(0),serial(ser),pinTransmit(_pinTransmit),recCount(0) {
  ser.begin(115200, SERIAL_8N1);
  ser.setTimeout(ANDAMBUS_TIMEOUT);
  pinMode(pinTransmit, OUTPUT);
  digitalWrite(pinTransmit, LOW);
}

/*BroadcastSocket::BroadcastSocket(SoftwareSerial &ser, uint8_t _pinTransmit):synchronized(false),syncIndex(0),serial(ser),pinTransmit(_pinTransmit),recCount(0) {
  ser.begin(115200);
  ser.setTimeout(ANDAMBUS_TIMEOUT);
  pinMode(pinTransmit, OUTPUT);
  digitalWrite(pinTransmit, LOW);
}*/

BroadcastSocket::~BroadcastSocket()
{}

void BroadcastSocket::startFrame(uint32_t size, uint16_t slaveAddress, uint16_t counter) {
	AndamBusFrameHeader hdr;
    hdr.magicWord = ANDAMBUS_MAGIC_WORD;
    hdr.payloadLength =size;
    hdr.slaveAddress = slaveAddress;
    hdr.counter = counter;
    hdr.apiVersion.major = ANDAMBUS_API_VERSION_MAJOR;
    hdr.apiVersion.minor = ANDAMBUS_API_VERSION_MINOR;
    htonHdr(hdr);

//    frm.header.crc32 = 0;
	crc.reset();
	crc.update(reinterpret_cast<const unsigned char*>(&hdr), sizeof(AndamBusFrameHeader));

//    frm.header.crc32 = htonl(crc32(0L, reinterpret_cast<const unsigned char*>(&frm), size+sizeof(AndamBusFrameHeader)));

//	LOG_U("startFrame size " << size);
    cleanReadBuffer();
    send(sizeof(AndamBusFrameHeader), reinterpret_cast<const unsigned char*>(&hdr));
	
//	LOG_U(F("bs1 mem:") << iom::dec << freeMemory());
}

void BroadcastSocket::frameData(uint32_t bytes, unsigned const char *buffer)
{
//	LOG_U("frameData size " << bytes);
	crc.update(buffer, bytes);
//    frmCrc = crc32(frmCrc, buffer, bytes);
    send(bytes, buffer);

//	LOG_U(F("bs2 mem:") << iom::dec << freeMemory());
}

void BroadcastSocket::finishFrame()
{
//    ERROR("finish crc=" << hex << frmCrc);
    uint32_t frmCrc = htonl(crc.finalize());
    send(sizeof(frmCrc), reinterpret_cast<const unsigned char*>(&frmCrc));
//	LOG_U(F("bs3 mem:") << iom::dec << freeMemory());
}

void BroadcastSocket::sendFrame(AndamBusFrame &frm, uint32_t size, uint16_t slaveAddress, uint16_t counter) {
	LOG_U(F("sendFrame size ") << size);
    startFrame(size, slaveAddress, counter);
    //send content
    frameData(size, reinterpret_cast<const unsigned char*>(&frm)+sizeof(AndamBusFrameHeader));

//    ERROR("sendFrame crc=" << hex << frmCrc);

    uint32_t frmCrc = htonl(crc.finalize());
    // send CRC32
    send(sizeof(uint32_t), reinterpret_cast<const unsigned char*>(&frmCrc));


//    ERROR("sending " << sizeof(AndamBusFrameHeader) << " + " << size << " bytes");
//    lastFrame = chrono::system_clock::now();
}

int BroadcastSocket::receiveFrame(AndamBusFrame &frm, uint16_t slaveAddress, uint32_t maxSize) {
//	unsigned long start = millis();
	
    if (!serial.available())
      return 12;

    if (!synchronized)
        return 1; // not synchronized

    if (maxSize < sizeof(AndamBusFrameHeader))
      return 4; // frame too large
    uint32_t crc = 0;

    if (justSynchronized()) {
            // if just synchronized, do not receive magic number, it was already received
        frm.header.magicWord = syncNint;
        if (!receive(sizeof(AndamBusFrameHeader)-sizeof(uint32_t), reinterpret_cast<unsigned char*>(&frm.header.payloadLength), ANDAMBUS_TIMEOUT_MS))
          return 10;
        syncIndex = 0;
    }
    else
        if (!receive(sizeof(AndamBusFrameHeader), reinterpret_cast<unsigned char*>(&frm), ANDAMBUS_TIMEOUT_MS))
          return 11;

    if(frm.header.magicWord != htonl(ANDAMBUS_MAGIC_WORD)) {
        synchronized = false;
        syncIndex = 0;

//        LOG_U(recCount << " magic " << iom::hex << frm.header.magicWord << iom::dec);
        return 2; // sync lost
    }


/*    if (hasTimeoutExpired(ANDAMBUS_TIMEOUT_MS))
        return 3; // timeout
*/
    uint32_t plen = htonl(frm.header.payloadLength);

//    LOG_U("receivedheader time " << (millis()-start));

    if (plen > maxSize - sizeof(AndamBusFrameHeader)) {
      synchronized = false;
      syncIndex = 0;
      return 4; // frame too large
    }

    if (!receive(plen, reinterpret_cast<unsigned char*>(&frm.command), ANDAMBUS_TIMEOUT_MS))
      return 12;

//    LOG_U("received cmd time " << (millis()-start));

    receive(sizeof(uint32_t), reinterpret_cast<unsigned char*>(&crc), 200);

    uint32_t crcReceived = ntohl(crc);
//    frm.header.crc32 = 0;

    uint32_t crcCalculated = CRC32::calculate(reinterpret_cast<const unsigned char*>(&frm), plen+sizeof(AndamBusFrameHeader));

    if (crcReceived != crcCalculated) {
      return 5; // CRC error
    }

    ntohHdr(frm.header);

    recCount++;
    if(frm.header.slaveAddress != slaveAddress)
      return 6; // different slave

    if (frm.header.apiVersion.major != ANDAMBUS_API_VERSION_MAJOR)
        return 7; // invalid API version

//	LOG_U(F("bs4 mem:") << iom::dec << freeMemory());
    return 0;
}

void BroadcastSocket::cleanReadBuffer() {
  while(serial.read() > 0);
}

const int BroadcastSocket::getPinTransmit() { 
	return pinTransmit; 
}

void BroadcastSocket::send(uint32_t bytes, unsigned const char *buffer) {
      digitalWrite(pinTransmit, HIGH);
//      delay(30);
      int l = serial.write(buffer, bytes);

//      LOG_U(millis() << " written " << l);

      serial.flush();

//      delay(30);
      digitalWrite(pinTransmit, LOW);

//      hexdump(buffer, bytes);
}

bool BroadcastSocket::receive(uint32_t  bytes, unsigned char *buffer, uint32_t timeoutMs) {
//  unsigned long start = millis();
  
  serial.setTimeout(timeoutMs);
  int len = serial.readBytes(buffer, bytes);

//      hexdump(buffer, bytes);

//  if (len == bytes)
//    LOG_U("receive  time " << (millis()-start) << " bytes " << len);
	  
  return len == bytes;
}

void BroadcastSocket::trySync() {
  
    while (serial.available()) {
      char b = serial.read();
        
      if (b != syncN[syncIndex++])
          syncIndex=0;
      if (syncIndex == 4) {
          synchronized = true;
          return;
      }
   }
}

bool BroadcastSocket::hasTimeoutExpired(uint32_t timeoutMs) {
  return false;
}

void BroadcastSocket::startTimer() {
  tmStart = millis();
}

char BroadcastSocket::readByte() {
  return serial.read();
}

bool BroadcastSocket::available() {
  return serial.available();
}
