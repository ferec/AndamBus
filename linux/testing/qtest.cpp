#include "AndamBusMaster.h"
#include "SerialBroadcastSocket.h"
#include "testing/LocalBroadcastSocket.h"


#include <curses.h>
#include <string.h>

#include <thread>
#include <chrono>

#include <iostream>
#include <fcntl.h>

using namespace std;

WINDOW *log_win = nullptr;

auto qtest_start = chrono::system_clock::now();

void logger(LogLevel level, const string &msg) {
    if (log_win == nullptr) {
        cerr << "logger not initialized" << endl;
        return;
    }
    auto now = chrono::system_clock::now();
    wprintw(log_win, "%ul %s %s\n", chrono::duration_cast<chrono::milliseconds>(now - qtest_start).count(), LogLevelString(level), msg.c_str());
    wrefresh(log_win);
}

AndamBusMaster *abu = nullptr;


/*void nonblock_stdin() {
    stringstream ss;
    int flags = fcntl(STDIN_FILENO, F_GETFL);

    if (flags == -1) {
        ss << "fcntl get:" << strerror(errno);
        throw runtime_error(ss.str());
    }

    int ret = fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    if (ret == -1) {
        ss << "fcntl set:" << strerror(errno);
        throw runtime_error(ss.str());
    }
}*/

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cout << "usage:\nqtest <dev> <unit address>" << endl;
        return 1;
    }

//    char mesg[]="Status";
//    ERROR("test err");

    string tstSerial("test");

    BroadcastSocket *bs = nullptr;
    try {
        if (argv[1] == tstSerial)
            bs = new LocalBroadcastSocket();
        else
            bs = new SerialBroadcastSocket(argv[1]);

        abu = new AndamBusMaster(*bs);
    } catch (ExceptionBase &e) {
        //ERROR("Error:" << e.what());
        wgetch(log_win);
        setDebugCallback(nullptr);
        endwin();
        return 1;
    }

//    printw("yyyy");

    int row,col;

    initscr();
    getmaxyx(stdscr,row,col);

    log_win = newwin(row-3, col, 0, 0);
    scrollok(log_win, true);

    nonblock_stdin();
    setLogLevel(LogLevel::WARNING);
    setDebugCallback(logger);


//    wprintw(log_win, "xxxx");
    char buf[16];

    uint8_t unit_id = stoi(argv[2]);

    for (int i=0;i<100000;i++) {
        mvwprintw(stdscr, row-1,0,"Sent:%lu Received:%lu Timeout:%lu Cmd err:%lu Inv frm:%lu",abu->getSentCount(), abu->getReceivedCount(), abu->getBusTimeoutCount(), abu->getCommandErrorCount(), abu->getInvalidFrameCount());
        wrefresh(stdscr);

        int r = read(STDIN_FILENO, buf, 16);

        if (r==1 && buf[0] =='q')
            break;

        this_thread::sleep_for(chrono::milliseconds(100));

        try {
            abu->detectSlave(unit_id);
        } catch (ExceptionBase &e) {
        }
        //WARNING("Test warn");
    }

//    char x =
    wgetch(log_win);

//    wprintw(log_win, "%c", x);
//    wrefresh(log_win);

//    wgetch(log_win);
    setDebugCallback(nullptr);

    endwin();

    cout << "Communication quality test results for unit " << (int)unit_id << endl;
    cout << "Sent:" << abu->getSentCount() <<
        " Received:" << abu->getReceivedCount() << " Timeout:" << abu->getBusTimeoutCount() <<
        " Cmd err:" << abu->getCommandErrorCount() << " Inv frm:" << abu->getInvalidFrameCount() << endl;

//    cout << "deleting" << endl;

//cout << "bs=" << bs << endl;

    if (abu != nullptr)
        delete abu;

    if (bs != nullptr)
        delete bs;

}
