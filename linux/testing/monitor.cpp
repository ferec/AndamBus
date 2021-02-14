#include <iostream>
#include <thread>

#include "../SerialBroadcastSocket.h"
#include "util.h"
#include "../shared/AndamBusExceptions.h"

#include "LocalBroadcastSocket.h"

using namespace std;


#define TEST_PORT_NAME string("test")
#define BUFFER_SIZE 2048

char buffer[BUFFER_SIZE];

int main(int argc, char **argv) {
    setLogLevel(LogLevel::WARNING);
    setDebugCallback(defaultLogger);

    if (argc < 2) {
        cout << "usage: abmonitor <portname>" << endl;
        return 1;
    }

    bool stop = false;

    try {
        char *ttyname = argv[1];

        BroadcastSocket *bs;
        if (TEST_PORT_NAME == ttyname)
            bs = new LocalBroadcastSocket();
        else
            bs = new SerialBroadcastSocket(ttyname);

        AndamBusFrame &frm = reinterpret_cast<AndamBusFrame&>(buffer);

        while (!stop) {
            this_thread::sleep_for(chrono::milliseconds(50));
            try {
                if (bs->isSynchronized()) {
                    bs->receiveFrame(frm, 0xffff, BUFFER_SIZE);

                    if (frm.header.slaveAddress == 0)
                        ntohResp(frm.response);
                    else
                        ntohCmd(frm.command);

                    printFrame(frm);
                    int l = frm.header.payloadLength + sizeof(AndamBusFrameHeader);
                    hexdump(reinterpret_cast<char*>(&frm), l, LogLevel::DEBUG);
                }
                else
                    bs->trySync();
            } catch (ExceptionBase &e) {
                AB_DEBUG("ex:" << e.what());
            }
        }
        delete bs;
    } catch(ExceptionBase &e) {
        AB_ERROR(e.what());
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    cout << "monitor finished" << endl;
}
