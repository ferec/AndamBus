#ifndef LOCALBROADCASTSOCKET_H
#define LOCALBROADCASTSOCKET_H

#include "BroadcastSocket.h"

#include <chrono>

#include <zlib.h>

class LocalBroadcastSocket : public BroadcastSocket
{
    public:
        LocalBroadcastSocket();
        virtual ~LocalBroadcastSocket();

        virtual void send(uint32_t bytes, unsigned const char *buffer);
        virtual bool receive(uint32_t bytes, unsigned char *buffer, unsigned int timeoutMs);

    protected:
        virtual char readByte();
        virtual bool available();
        virtual void cleanReadBuffer();
        virtual void dropBytes(int32_t cnt);

        virtual void startTimer();
        virtual bool hasTimeoutExpired(uint32_t timeoutMs);
        virtual uint32_t crc(uint32_t init, const unsigned char* data, uint32_t len);

    private:
        int sock;
        std::chrono::time_point<std::chrono::system_clock> tmStart;
};

#endif // LOCALBROADCASTSOCKET_H
