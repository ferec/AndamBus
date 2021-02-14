#ifndef SERIALBROADCASTSOCKET_H
#define SERIALBROADCASTSOCKET_H

#include "BroadcastSocket.h"
#include <string>
#include <chrono>

class SerialBroadcastSocket : public BroadcastSocket
{
    public:
        SerialBroadcastSocket(std::string _tty);
        virtual ~SerialBroadcastSocket();

        virtual void send(uint32_t  bytes, unsigned const char *buffer);
        virtual bool receive(uint32_t  bytes, unsigned char *buffer, unsigned int timeoutMs);
//        virtual void clean();

        static void setupSerial(int port);
        void setupSerial();

    protected:
        size_t getAvailableBytes();

        virtual char readByte();
        virtual bool available();
        virtual void cleanReadBuffer();
        virtual void startTimer();
        virtual bool hasTimeoutExpired(uint32_t timeoutMs);
        virtual void dropBytes(int32_t cnt);

    private:
        int ttyHandle;
        std::chrono::time_point<std::chrono::system_clock> tmStart;
};

#endif // SERIALBROADCASTSOCKET_H
