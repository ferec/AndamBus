#include "LocalBroadcastSocket.h"
#include "broadcastServer.h"
#include "shared/AndamBusExceptions.h"
#include "util.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>



#include <sstream>
#include <thread>

using namespace std;

LocalBroadcastSocket::LocalBroadcastSocket():sock(-1)
{
    stringstream ss;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == 0) {
        ss << "create socket:" << strerror(errno);
        throw IOException(ss.str().c_str());
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( ANDAMBUS_TEST_PORT );

    int ret = connect(sock, (struct sockaddr *)&address,
                             sizeof(address));

    if (ret < 0) {
        ss << "connect:" << strerror(errno);
        throw IOException(ss.str().c_str());
    }

    int flags = fcntl(sock, F_GETFL);

    if (flags == -1) {
        ss << "fcntl get:" << strerror(errno);
        throw IOException(ss.str().c_str());
    }

    ret = fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    if (ret == -1) {
        ss << "fcntl set:" << strerror(errno);
        throw IOException(ss.str().c_str());
    }

}

LocalBroadcastSocket::~LocalBroadcastSocket()
{
    if (sock >= 0)
        close(sock);
}

void LocalBroadcastSocket::send(uint32_t bytes, unsigned const char *buffer) {
//    hexdump(reinterpret_cast<const char*>(buffer), bytes);
    ssize_t l = write(sock, buffer, bytes);

    if (l!=static_cast<ssize_t>(bytes))
        AB_ERROR("Written " << l << " instead of " << bytes);
    #warning Handle if not written at once
}

bool LocalBroadcastSocket::receive(uint32_t bytes, unsigned char *buffer, unsigned int timeoutMs) {
//    hexdump(reinterpret_cast<const char*>(buffer), bytes);

    auto start = chrono::system_clock::now();
    uint32_t pos = 0;

    do {
        int l = read(sock, buffer+pos, bytes-pos);

        if (l==-1)
            this_thread::sleep_for(chrono::milliseconds(20));
        if (l>0)
            pos+=l;

        if (pos == bytes)
            return true;
    } while (chrono::system_clock::now()-start <= chrono::milliseconds(timeoutMs));


    throw BusTimeoutException("Cannot read");
}

/*void LocalBroadcastSocket::clean() {
    char buf[1024];
    int l;
    do {
        l = read(sock, buf, 1024);
    } while (l>0);
}*/

void LocalBroadcastSocket::startTimer() {
    tmStart = chrono::system_clock::now();
}

bool LocalBroadcastSocket::hasTimeoutExpired(uint32_t tmout) {
    return chrono::system_clock::now()-tmStart > chrono::milliseconds(tmout);
}

char LocalBroadcastSocket::readByte() {
    char buf;
    int l = read(sock, &buf, 1);

    if (l==1)
        return buf;

    stringstream ss;
    ss << "Error reading byte:" << strerror(errno);
    throw IOException(ss.str().c_str());
}

bool LocalBroadcastSocket::available() {
    struct pollfd fds = {};

    fds.fd = sock;
    fds.events = POLLIN;

    return poll(&fds, 1, 50)>0;
}

void LocalBroadcastSocket::cleanReadBuffer() {
    char buf[1024];
    while (read(sock, &buf, 1024) > 0) {};
}

void LocalBroadcastSocket::dropBytes(int32_t cnt)
{
    char buf[1024];

    do {
        int l = read(sock, &buf, cnt);
        if (l<0)
            return;
        cnt-=l;
    } while (cnt > 0);

}

uint32_t LocalBroadcastSocket::crc(uint32_t init, const unsigned char* data, uint32_t len) {
    return crc32(init, data, len);
}
