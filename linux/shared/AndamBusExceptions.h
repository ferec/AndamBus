#ifndef ANDAMBUSEXCEPTIONS_H_INCLUDED
#define ANDAMBUSEXCEPTIONS_H_INCLUDED

//#include <stdexcept>
//#include <sstream>

#include <string>

#include "AndamBusTypes.h"


class ExceptionBase
{
    public:
        ExceptionBase(std::string _msg):msg(_msg) {}

        virtual void dummy() {}

        const std::string& what();
    protected:
        const std::string msg;
};

// thrown if slave not answered to the call in the specified time period
class BusTimeoutException : public ExceptionBase
{
    public:
    BusTimeoutException (std::string _msg): ExceptionBase(_msg) {}

//    virtual ~BusTimeoutException();

};

// thrown if the magic word has invalid value
class SynchronizationLostException : public ExceptionBase
{
    public:
    SynchronizationLostException  (std::string _msg): ExceptionBase(_msg) {}

//    virtual ~SynchronizationLostException();

};

// thrown in case the calculated and received CRC does not match
struct InvalidFrameException : public ExceptionBase
{
    public:
    InvalidFrameException (std::string _msg): ExceptionBase(_msg) {}

};

// thrown when the frame is for different unit
struct DifferentTargetException : public ExceptionBase
{
    public:
    DifferentTargetException (uint16_t frmAddr, uint16_t trgAddr): ExceptionBase("address"),frameAddr(frmAddr),targetAddr(trgAddr) {}
    const uint16_t getTargetAddress() { return targetAddr; }
    const uint16_t getFrameAddress() { return frameAddr; }

    private:
        const uint16_t frameAddr, targetAddr;
};

// thrown if slave sent the ERROR response
struct CommandException : public ExceptionBase
{
    private:
        AndamBusCommandError err;
        AndamBusCommand command;
//        std::string msg;
//        const char * cmdMsg;
    public:
    CommandException (AndamBusCommandError error, AndamBusCommand cmd);


//    const char* getCommandString() { return commandString(command); }

//    virtual const char* what() const throw ();

};

// thrown in case of low-level errors
struct IOException : public ExceptionBase
{
    public:
    IOException (std::string _msg): ExceptionBase(_msg) {}

};

// thrown in case property does not exists
struct PropertyException : public ExceptionBase
{
    public:
    PropertyException (std::string _msg): ExceptionBase(_msg) {}

};

// thrown in case property does not exists
struct InvalidContentException : public ExceptionBase
{
    public:
    InvalidContentException (std::string _msg): ExceptionBase(_msg) {}

};

#endif // ANDAMBUSEXCEPTIONS_H_INCLUDED
