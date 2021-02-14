#ifndef BROADCASTSOCKET_H
#define BROADCASTSOCKET_H

//#include <string>
#include <inttypes.h>
#include <chrono>

#include "shared/AndamBusTypes.h"

class BroadcastSocket
{
    public:
        BroadcastSocket();
        virtual ~BroadcastSocket();



        virtual int receiveFrame(AndamBusFrame &frm, uint16_t slaveAddress, uint32_t maxSize);
        virtual void sendFrame(AndamBusFrame &frm, uint32_t size, uint16_t slaveAddress, uint16_t counter);
        virtual void startFrame(uint32_t size, uint16_t slaveAddress, uint16_t counter);
        virtual void frameData(uint32_t bytes, unsigned const char *buffer);
        virtual void finishFrame();

        void trySync(); // non-blocking, uses available()
        bool isSynchronized() { return synchronized; }

        const int getPinTransmit();

    protected:
        virtual void send(uint32_t bytes, unsigned const char *buffer) = 0;
        virtual bool receive(uint32_t  bytes, unsigned char *buffer, uint32_t timeoutMs) = 0;
        virtual char readByte() = 0;
        virtual bool available() = 0;
        virtual void cleanReadBuffer() = 0;
        virtual void dropBytes(int32_t cnt) = 0;


        virtual void startTimer() = 0;
        virtual bool hasTimeoutExpired(uint32_t timeoutMs) = 0;

        bool justSynchronized() { return synchronized && syncIndex == 4; }

    private:
        bool synchronized;
        uint8_t syncIndex;
        uint32_t frmCrc;

};

#endif // BROADCASTSOCKET_H
