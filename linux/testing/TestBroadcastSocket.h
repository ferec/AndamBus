#ifndef TESTBROADCASTSOCKET_H
#define TESTBROADCASTSOCKET_H

#include "BroadcastSocket.h"

#include "shared/AndamBusTypes.h"


#define TESTSLAVE_SW_VERSION_MAJOR 0
#define TESTSLAVE_SW_VERSION_MINOR 7

class TestBroadcastSocket : public BroadcastSocket
{
    public:
        TestBroadcastSocket();
        virtual ~TestBroadcastSocket();

        virtual void send(uint32_t bytes, unsigned const char *buffer);
        virtual bool receive(uint32_t bytes, unsigned char *buffer, unsigned int timeoutMs);
        virtual void clean();

    protected:
        void setHeader(AndamBusFrame &frm, uint32_t payload_size);
        void setHeader(AndamBusFrame &frm);
        void setResponse(AndamBusFrame &frm);

        virtual char readByte();
        virtual bool available();
        virtual void cleanReadBuffer();
        virtual void startTimer();
        virtual bool hasTimeoutExpired(uint32_t timeoutMs);

    private:
        unsigned char ibuffer[1024];
        int bufPos, bufSize;

        void testResponseSlavePresent(AndamBusFrame &resp);
        void testResponseOk(AndamBusFrame &cmd, AndamBusFrame &resp);
        void testPortList(AndamBusFrameResponse &resp);
        void testBusList(AndamBusFrameResponse &resp);
        void testDevList(AndamBusFrameResponse &resp);
        void testResponseError(AndamBusFrame &resp);
        void testProps(AndamBusFrame &resp);
        void testPortCreate(const AndamBusFrameCommand &cmd, AndamBusFrameResponse &resp);
        void testDeviceCreate(const AndamBusFrameCommand &cmd, AndamBusFrameResponse &resp);
        void testBusCreate(const AndamBusFrameCommand &cmd, AndamBusFrameResponse &resp);

        int portValue[10];

        int portIdMax;

        static int ID;
};

#endif // TESTBROADCASTSOCKET_H
