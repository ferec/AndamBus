#include "AndamBusExceptions.h"
#include "util.h"

using namespace std;

const string& ExceptionBase::what() {
    return msg;
    }

CommandException::CommandException(AndamBusCommandError error, AndamBusCommand cmd): ExceptionBase(responseErrorString(error, cmd)),err(error),command(cmd) {
}

/*const char* CommandException::what() const throw () {
    return msg.c_str();
}*/
