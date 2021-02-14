#ifndef BROADCASTSOCKET_H
#define BROADCASTSOCKET_H

#define ANDAMBUS_TIMEOUT 200

#include "Arduino.h"

#include <inttypes.h>
#include <CRC32.h>

#include "AndamBusTypes.h"
#include "SerialCompat.h"

//#include <SoftwareSerial.h>

class BroadcastSocket
{
    public:
        BroadcastSocket(HardwareSerial &ser, uint8_t pinTransmit);
//        BroadcastSocket(SoftwareSerial &ser, uint8_t pinTransmit);
        virtual ~BroadcastSocket();



        int receiveFrame(AndamBusFrame &frm, uint16_t slaveAddress, uint32_t maxSize);
        void sendFrame(AndamBusFrame &frm, uint32_t size, uint16_t slaveAddress, uint16_t counter);
        void startFrame(uint32_t size, uint16_t slaveAddress, uint16_t counter);
        void frameData(uint32_t bytes, unsigned const char *buffer);
        void finishFrame();


        void trySync(); // non-blocking, uses available()
        bool isSynchronized() { return synchronized; }

        const int getPinTransmit();

    protected:
        void send(uint32_t bytes, unsigned const char *buffer);
        bool receive(uint32_t  bytes, unsigned char *buffer, uint32_t timeoutMs);
        char readByte();
        bool available();
        void cleanReadBuffer();
        void startTimer();
        bool hasTimeoutExpired(uint32_t timeoutMs);

        bool justSynchronized() { return synchronized && syncIndex == 4; }
    private:
		bool synchronized;
		uint8_t syncIndex;
//		uint32_t frmCrc;
		
		CRC32 crc;
		
    Stream &serial;
    const int pinTransmit;

    uint32_t recCount;

    unsigned long tmStart;
};

#endif // BROADCASTSOCKET_H
