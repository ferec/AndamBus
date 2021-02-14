#include <iostream>
#include <sstream>
#include <thread>

#include "shared/AndamBusExceptions.h"
#include "util.h"

#include "broadcastServer.h"
#include "LocalBroadcastSocket.h"
#include "TestAndamBusUnit.h"

using namespace std;

auto t_start = chrono::system_clock::now();

void unitLogger(LogLevel level, const string &msg) {
    auto now = chrono::system_clock::now();
    cout << chrono::duration_cast<chrono::milliseconds>(now - t_start).count() << " " << LogLevelString(level) << ":" << msg << endl;
}

int main() {
    setLogLevel(LogLevel::INFO);
    setDebugCallback(unitLogger);

    LocalBroadcastSocket bs;
    TestAndamBusUnit abu(bs, 3);

    nonblock_stdin();
    char buf[16];

    while (true) {
        int r = read(STDIN_FILENO, buf, 16);

        if (r> 0 && buf[0] =='q')
            break;

        try {
            abu.doWork();
            this_thread::sleep_for(chrono::milliseconds(100));
        } catch (ExceptionBase &e) {
            AB_WARNING("ExceptionBase:" << e.what());
            this_thread::sleep_for(chrono::milliseconds(500));
        }
    }

    AB_INFO("Finish");
}
