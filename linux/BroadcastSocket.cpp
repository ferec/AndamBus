#include "BroadcastSocket.h"
#include "shared/AndamBusExceptions.h"
#include "util.h"

#include <zlib.h>
#include <thread>

using namespace std;

const uint32_t syncNint = htonl(ANDAMBUS_MAGIC_WORD);
const char *syncN = reinterpret_cast<const char*>(&syncNint);

BroadcastSocket::BroadcastSocket():synchronized(false),syncIndex(0)//lastFrame(chrono::system_clock::now()),
//    frmCounter(0)
{
}

BroadcastSocket::~BroadcastSocket()
{
}

void BroadcastSocket::startFrame(uint32_t size, uint16_t slaveAddress, uint16_t counter) {
    if (!synchronized) {
        cleanReadBuffer();
        synchronized = true;
    }

    AB_DEBUG("sendFrame size=" << size << ",slave=" << slaveAddress);

    AndamBusFrameHeader hdr;

    hdr.magicWord = ANDAMBUS_MAGIC_WORD;
    hdr.payloadLength =size;
    hdr.slaveAddress = slaveAddress;
    hdr.counter = counter;
    hdr.apiVersion.major = ANDAMBUS_API_VERSION_MAJOR;
    hdr.apiVersion.minor = ANDAMBUS_API_VERSION_MINOR;
    htonHdr(hdr);

//    frm.header.crc32 = 0;
    frmCrc = crc32(0L, reinterpret_cast<const unsigned char*>(&hdr), sizeof(AndamBusFrameHeader));

//    frm.header.crc32 = htonl(crc32(0L, reinterpret_cast<const unsigned char*>(&frm), size+sizeof(AndamBusFrameHeader)));

    cleanReadBuffer();
    send(sizeof(AndamBusFrameHeader), reinterpret_cast<const unsigned char*>(&hdr));
}

void BroadcastSocket::frameData(uint32_t bytes, unsigned const char *buffer)
{
    frmCrc = crc32(frmCrc, buffer, bytes);
    send(bytes, buffer);

//    AB_ERROR("frameData " << bytes);
//    hexdump((const char*)buffer, bytes, LogLevel::AB_INFO);
}

void BroadcastSocket::finishFrame()
{
//    AB_ERROR("finish crc=" << hex << frmCrc);
    frmCrc = htonl(frmCrc);
    send(sizeof(frmCrc), reinterpret_cast<const unsigned char*>(&frmCrc));
}

void BroadcastSocket::sendFrame(AndamBusFrame &frm, uint32_t size, uint16_t slaveAddress, uint16_t counter) {
    auto start = chrono::system_clock::now();
    startFrame(size, slaveAddress, counter);
    //send content

    auto dur = chrono::system_clock::now()-start;
//    AB_INFO("startframe duration " << chrono::duration_cast<chrono::milliseconds>(dur).count() << "ms");

    frameData(size, reinterpret_cast<const unsigned char*>(&frm)+sizeof(AndamBusFrameHeader));

//    AB_ERROR("sendFrame crc=" << hex << frmCrc);

    dur = chrono::system_clock::now()-start;
    //AB_INFO("frameData duration " << chrono::duration_cast<chrono::milliseconds>(dur).count() << "ms");

    frmCrc = htonl(frmCrc);
    // send CRC32
    send(sizeof(uint32_t), reinterpret_cast<const unsigned char*>(&frmCrc));


//    AB_ERROR("sending " << sizeof(AndamBusFrameHeader) << " + " << size << " bytes");
//    lastFrame = chrono::system_clock::now();
    dur = chrono::system_clock::now()-start;
//    AB_INFO("sendFrame duration " << chrono::duration_cast<chrono::milliseconds>(dur).count() << "ms");

}

int BroadcastSocket::receiveFrame(AndamBusFrame &frm, uint16_t slaveAddress, uint32_t maxSize) {
    if (!synchronized)
        throw SynchronizationLostException("Not synchronized");

    if (maxSize < sizeof(AndamBusFrameHeader))
        throw PropertyException("maxsize too low");

    // if synchronized - read full header
    AB_DEBUG("Receiving header");

    uint32_t crc = 0;

    if (justSynchronized()) {
            // if just synchronized, do not receive magic number, it was already received
        startTimer();
        frm.header.magicWord = syncNint;
        receive(sizeof(AndamBusFrameHeader)-sizeof(uint32_t), reinterpret_cast<unsigned char*>(&frm.header.payloadLength), ANDAMBUS_TIMEOUT_MS);
        syncIndex = 0;
//        AB_INFO("Received1");
    }
    else {
        receive(sizeof(AndamBusFrameHeader), reinterpret_cast<unsigned char*>(&frm), ANDAMBUS_TIMEOUT_MS);
        startTimer();
//        AB_INFO("Received2");
    }

    if(frm.header.magicWord != htonl(ANDAMBUS_MAGIC_WORD))
        throw SynchronizationLostException("Sync lost");

    if (hasTimeoutExpired(ANDAMBUS_TIMEOUT_MS))
        throw BusTimeoutException("Bus timeout (after header)");


    uint32_t plen = htonl(frm.header.payloadLength);

//    AB_ERROR("received frame " << )

    if (plen > maxSize - sizeof(AndamBusFrameHeader)) {
        stringstream ss;
        ss << "Invalid frame size " << dec << plen << "(max " << maxSize << ")";
//        synchronized = false;
        dropBytes(plen+2);

        AB_ERROR("size=" << plen);
        throw InvalidFrameException("InvalidFrame size");
    }

    receive(plen, reinterpret_cast<unsigned char*>(&frm.command), ANDAMBUS_TIMEOUT_MS);
//    AB_DEBUG("Payload received " << frmCounter++);
//    AB_INFO("Received3");

    receive(sizeof(uint32_t), reinterpret_cast<unsigned char*>(&crc), 200);
//    crc=ntohl(crc);
    //AB_INFO("Received4");

    uint32_t crcReceived = ntohl(crc);
//    AB_ERROR("CRC received " << hex << crc);

//    frm.header.crc32 = 0;

    uint32_t crcCalculated = crc32(0, reinterpret_cast<const unsigned char*>(&frm), plen+sizeof(AndamBusFrameHeader));

    if (crcReceived != crcCalculated) {
        stringstream ss;
        ss << hex << "Calculated(0x"<<crcCalculated<<") and received(0x"<<crcReceived<<") CRC does not match";
        hexdump((char*)&frm, sizeof(AndamBusFrameHeader) + ntohl(frm.header.payloadLength));
        AB_ERROR("crcCalculated=" << hex << crcCalculated << " crcReceived=" << crcReceived << " plen=" << dec << plen);
        throw InvalidFrameException("InvalidFrame CRC");
    }

//    hexdump((char*)&frm, sizeof(AndamBusFrameHeader) + ntohl(frm.header.payloadLength), LogLevel::AB_ERROR);

    ntohHdr(frm.header);

    if(frm.header.slaveAddress != slaveAddress && slaveAddress != 0xffff) {
//        AB_DEBUG("Ignoring frame for different address" << frm.header.slaveAddress);
        throw DifferentTargetException(frm.header.slaveAddress, slaveAddress);
    }

    if (frm.header.apiVersion.major != ANDAMBUS_API_VERSION_MAJOR)
        throw InvalidFrameException("Received API version does not match");

    if (frm.header.apiVersion.minor != ANDAMBUS_API_VERSION_MINOR)
        AB_WARNING("API minor version different " << (int)ANDAMBUS_API_VERSION_MINOR << " vs " << (int)frm.header.apiVersion.minor);

    return 0;
}

void BroadcastSocket::trySync() {
    while (available()) {
        char b = readByte();
        if (b != syncN[syncIndex++])
            syncIndex=0;
        if (syncIndex == 4) {
            synchronized = true;
//            syncIndex=0;
            return;
        }
    }
}

const int BroadcastSocket::getPinTransmit() {
    return 0;
}
