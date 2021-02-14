#include <iostream>

#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>

using namespace std;

bool setupSerial(int port) {
    /* *** Configure Port *** */
    struct termios tty;
    memset (&tty, 0, sizeof tty);

    if ( tcgetattr ( port, &tty ) != 0 )
        {
        perror("get attr");
        return false;
        }

    cfsetospeed (&tty, B9600);
    cfsetispeed (&tty, B9600);


    /* Setting other Port Stuff */
    tty.c_cflag     &=  ~PARENB;        // Make 8n1
    tty.c_cflag     &=  ~CSTOPB;
    tty.c_cflag     &=  ~CSIZE;
    tty.c_cflag     |=  CS8;
    tty.c_cflag     &=  ~CRTSCTS;       // no flow control
    tty.c_lflag     =   0;          // no signaling chars, no echo, no canonical processing
    tty.c_oflag     =   0;                  // no remapping, no delays
    tty.c_cc[VMIN]      =   0;                  // read doesn't block
    tty.c_cc[VTIME]     =   5;                  // 0.5 seconds read timeout

    tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines
    tty.c_iflag     &=  ~(IXON | IXOFF | IXANY);// turn off s/w flow ctrl
    tty.c_lflag     &=  ~(ICANON | ECHO | ECHOE | ISIG); // make raw
    tty.c_oflag     &=  ~OPOST;              // make raw

        /* Flush Port, then applies attributes */
    tcflush( port, TCIFLUSH );

    if ( tcsetattr ( port, TCSANOW, &tty ) != 0)
    {
    perror("set attr");
    return false;
    }

    return true;
}

int main()
{
    int port = open( "/dev/ttyUSB0", O_RDWR| O_NONBLOCK | O_NDELAY );

    if (port < 0) {
        perror("open");
    }

    setupSerial(port);


    while(1) {
     string s;
     cin >> s;

    write( port, s.c_str(), s.length());

    }

    cout << "Hello world!" << endl;
    return 0;
}
