#include "SerialCompat.h"

hwISerial::hwISerial(HardwareSerial &_ser):ser(_ser) {
}

int hwISerial::available() {
	return ser.available();
}

void hwISerial::flush() {
	ser.flush();
}

size_t hwISerial::write(uint8_t byte) {
	return ser.write(byte);
}

int hwISerial::read() {
	return ser.read();
}

size_t hwISerial::readBytes( char *buffer, size_t length) {
	ser.readBytes(buffer,length);
}
