#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <vector>

#include "broadcastServer.h"
#include "../util.h"

using namespace std;

int srvfd;
bool quitting = false;

vector<int> clientList;

int startserver() {
    stringstream ss;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == 0) {
        ss << "create socket:" << strerror(errno);
        throw runtime_error(ss.str());
    }

    int reuse = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
        {
                perror("SO_REUSEADDR failed");
                return -1;
        }
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0)
        {
        ss << "setsockopt:" << strerror(errno);
        throw runtime_error(ss.str());
        }

    int flags = fcntl(sockfd, F_GETFL);

    if (flags == -1) {
        ss << "fcntl get:" << strerror(errno);
        throw runtime_error(ss.str());
    }

    int ret = fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    if (ret == -1) {
        ss << "fcntl set:" << strerror(errno);
        throw runtime_error(ss.str());
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK); //INADDR_ANY;
    address.sin_port = htons( ANDAMBUS_TEST_PORT );

    if (bind(sockfd, (struct sockaddr *)&address,
                                 sizeof(address))<0) {
        ss << "bind:" << strerror(errno);
        throw runtime_error(ss.str());
    }

        if (listen(sockfd, 3) < 0)
    {
        ss << "listen:" << strerror(errno);
        throw runtime_error(ss.str());
    }

    return sockfd;
}

void checkAccept() {
    stringstream ss;
    sockaddr_in csa;
    socklen_t css = sizeof(csa);
    int nsock = accept(srvfd, (struct sockaddr *)&csa,
                   (socklen_t*)&css);

    if (nsock == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
        return;

    if (nsock<=0)
    {
        ss << "accept:" << strerror(errno);
        throw runtime_error(ss.str());
    }

    char str[INET_ADDRSTRLEN];
    inet_ntop( AF_INET, &csa.sin_addr, str, INET_ADDRSTRLEN );

    cout << "addr:" << hex << str << endl;
    cout << "port:" << dec << ntohs(csa.sin_port) << endl;

    int flags = fcntl(nsock, F_GETFL);

    if (flags == -1) {
        ss << "fcntl get:" << strerror(errno);
        throw runtime_error(ss.str());
    }

    int ret = fcntl(nsock, F_SETFL, flags | O_NONBLOCK);

    if (ret == -1) {
        ss << "fcntl set:" << strerror(errno);
        throw runtime_error(ss.str());
    }

    clientList.push_back(nsock);
}


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

int read_client(int sock, char *buf, size_t s) {
    ssize_t r;
    r = read(sock, buf, s);

/*    if (r > 0)
        cout << "read:" << buf << endl;*/

    if (r == 0)
        cout << "closed socket" << endl;

    return r;
}

void write_other(int sock, char *buf, size_t r) {
    for (auto it=clientList.begin();it!=clientList.end();it++) {
        int s = *it;
        if (s == sock)
            continue;
        write(s, buf, r);
    }
}

void doCommand(char cmd) {
    switch (cmd) {
    case 'q':
        quitting=true;
        break;
    default:
        cout << "unknown command " << cmd << endl;
    }
}

void checkCommand() {
    char buf[16];
    int r = read(STDIN_FILENO, buf, 16);

    if (r > 0) {
        cout << "command:" << buf << endl;
        cout << "q - quit" << endl;
        doCommand(buf[0]);
    }
}

int main() {
    stringstream ss;
    nonblock_stdin();
    srvfd = startserver();

    int iter = 0;

    while (!quitting) {
        checkCommand();
        checkAccept();
        for (auto it=clientList.begin();it!=clientList.end();it++) {
            int sock = *it;
            cout << "checking sock=" << sock << endl;
            char buf[1024];
            int r = read_client(sock, buf, 1024);


            if (r == 0)
                it = clientList.erase(it);

            cout << "cl=" << clientList.size() << endl;

            if (it == clientList.end())
                break;

            if (r > 0)
                write_other(sock, buf, r);

        }

        if (iter % 100 == 0)
            cout << "iter=" << iter++ << endl;

        this_thread::sleep_for(chrono::milliseconds(50));
    }

    for (auto it=clientList.begin();it!=clientList.end();it++) {
        close(*it);
    }

    close(srvfd);
}
