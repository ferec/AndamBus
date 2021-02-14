#include "SerialBroadcastSocket.h"

#include "shared/AndamBusExceptions.h"
#include "util.h"

#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>
#include <sys/ioctl.h>
#include <poll.h>
#include <linux/serial.h>

#include <chrono>

using namespace std;

SerialBroadcastSocket::SerialBroadcastSocket(string _tty):ttyHandle(-1)
{
    ttyHandle = open(_tty.c_str(), O_RDWR); //| O_NONBLOCK | O_NDELAY );
    AB_DEBUG(dec << "tty handle=" << ttyHandle);
    if (ttyHandle == -1) {
        AB_ERROR("error opening file " << _tty << ":" << strerror(errno));
        throw IOException(_tty);
    }
    setupSerial();
}

SerialBroadcastSocket::~SerialBroadcastSocket()
{
    if (ttyHandle >= 0) {
        AB_DEBUG("closing " << ttyHandle);
        close(ttyHandle);
    }
}

void SerialBroadcastSocket::setupSerial() {
    setupSerial(ttyHandle);
}

void SerialBroadcastSocket::setupSerial(int port) {
    /* *** Configure Port *** */
    struct termios tty;
    memset (&tty, 0, sizeof tty);

    if ( tcgetattr ( port, &tty ) != 0 )
        throw runtime_error(strerror(errno));

    if (cfsetospeed (&tty, B115200) != 0)
        throw runtime_error(strerror(errno));
    if (cfsetispeed (&tty, B115200) != 0)
        throw runtime_error(strerror(errno));

//    cfmakeraw(&tty);

    tty.c_cflag &= ~PARENB;   /* Disables the Parity Enable bit(PARENB),So No Parity   */
    tty.c_cflag &= ~CSTOPB;   /* CSTOPB = 2 Stop bits,here it is cleared so 1 Stop bit */
    tty.c_cflag &= ~CSIZE;	 /* Clears the mask for setting the data size             */
    tty.c_cflag |=  CS8;      /* Set the data bits = 8                                 */

    tty.c_cflag &= ~CRTSCTS;       /* No Hardware flow Control                         */
    tty.c_cflag |= CREAD | CLOCAL; /* Enable receiver,Ignore Modem Control lines       */


    tty.c_iflag &= ~(IXON | IXOFF | IXANY);          /* Disable XON/XOFF flow control both i/p and o/p */
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

    tty.c_oflag &= ~(OPOST|ONLCR);/*No Output Processing*/

    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO; // Disable echo
    tty.c_lflag &= ~ECHOE; // Disable erasure
    tty.c_lflag &= ~ECHONL; // Disable new-line echo
    tty.c_lflag &= ~ISIG;

    tty.c_cc[VMIN]      =   0;                  // read doesn't block
    tty.c_cc[VTIME]     =   1;                  // 0.1 seconds read timeout

        /* Flush Port, then applies attributes */
    if (tcflush( port, TCIFLUSH ) != 0 )
        throw runtime_error(strerror(errno));

    if ( tcsetattr ( port, TCSANOW, &tty ) != 0)
        throw runtime_error(strerror(errno));


}

/*
can be tested with virtual terminal
socat -d -d pty,raw,echo=0 pty,raw,echo=0
*/

void SerialBroadcastSocket::send(uint32_t  bytes, unsigned const char *buffer) {
    size_t pos = 0, cnt=0;

    if (available())
        AB_WARNING("data available while sending");

    while (pos < bytes) {
        int ret = write(ttyHandle, buffer+pos, bytes-pos);
        if (ret == -1)
            throw IOException(strerror(errno));
        if (ret == 0)
            AB_DEBUG("0 bytes sent");
        pos+=ret;

        if (cnt++ > 10)
            AB_WARNING("SerialBroadcastSocket::send count high=" << cnt);
    }

    if (tcflush( ttyHandle, TCIFLUSH ) != 0 )
        AB_ERROR("problem flushing");

//    fsync(ttyHandle);

    AB_DEBUG("sent " << bytes << " bytes");
//    hexdump((char*)buffer, bytes);

}

bool SerialBroadcastSocket::receive(uint32_t bytes, unsigned char *buffer, unsigned int timeoutMs) {
    size_t pos = 0;

    int iter = 0;

    auto start = chrono::system_clock::now();

    while (pos < bytes) {
        AB_DEBUG("read pos:" << pos);
        int ret = read(ttyHandle, buffer+pos, bytes-pos);
        if (ret == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            AB_DEBUG("errno:" << errno);
            throw IOException(strerror(errno));
        }
        if (ret == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            buffer[pos] = 0;
            AB_DEBUG("no data received");
            throw BusTimeoutException("Cannot read");
        }
        if (ret == 0) {
            AB_DEBUG("0 bytes received");
            if (!available() && chrono::system_clock::now()-start >= chrono::milliseconds(timeoutMs)) {
                stringstream ss;
                ss << "Received " << pos << " bytes of " << bytes;
                throw BusTimeoutException(ss.str());
            }
        }

        AB_DEBUG("received:" << ret << " bytes of " << bytes);

        pos+=ret;

        if (iter++ > 10)
            AB_WARNING("receive iterations high iter=" << iter << "; timeoutMs=" << timeoutMs << " ret=" << ret << " pos=" << pos << " diff=" << (chrono::system_clock::now()-start >= chrono::milliseconds(timeoutMs)));
    }

    hexdump((char*)buffer, bytes);
    return true;
}

size_t SerialBroadcastSocket::getAvailableBytes() {
    size_t cnt;

    if (ioctl(ttyHandle, FIONREAD, &cnt) == -1)
            throw IOException(strerror(errno));
    return cnt;
}

void SerialBroadcastSocket::cleanReadBuffer() {
    int cnt;

    do {
        cnt = getAvailableBytes();

        cnt = cnt>1024?1024:cnt;

        char buf[1024];

        for (int i=0;i<cnt;) {
            i+=read(ttyHandle, buf, cnt);
            AB_INFO("Dropped bytes " << i);
            hexdump(buf, i);
        }
    } while (cnt > 0);
}

void SerialBroadcastSocket::dropBytes(int32_t cnt)
{
    char buf[1024];

    auto start = chrono::system_clock::now();

    do {
        int l = read(ttyHandle, &buf, cnt);
        if (l<0)
            return;
        cnt-=l;

        if (chrono::system_clock::now()-start >= chrono::milliseconds(ANDAMBUS_TIMEOUT_MS)) {
            AB_INFO("drop bytes timeout remaining cnt=" << cnt);
            return;
        }
    } while (cnt > 0);
}

char SerialBroadcastSocket::readByte() {
    char buf;
    int l = read(ttyHandle, &buf, 1);

    if (l==1)
        return buf;

    stringstream ss;
    ss << "Error reading byte:" << strerror(errno);
    throw IOException(ss.str().c_str());
}

bool SerialBroadcastSocket::available() {
    struct pollfd fds = {};

    fds.fd = ttyHandle;
    fds.events = POLLIN;

    return poll(&fds, 1, 2)>0;
}

void SerialBroadcastSocket::startTimer() {
    tmStart = chrono::system_clock::now();
}

bool SerialBroadcastSocket::hasTimeoutExpired(uint32_t tmout) {
    return chrono::system_clock::now()-tmStart > chrono::milliseconds(tmout);
}
