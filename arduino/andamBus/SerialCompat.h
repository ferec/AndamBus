#include <Arduino.h>

class ISerial {
	public:
	virtual size_t write(uint8_t byte) = 0;
	virtual int read() = 0;
	virtual int available() = 0;
	virtual void flush() = 0;
	virtual size_t readBytes( char *buffer, size_t length) = 0;
};

class hwISerial : public ISerial {
	public:
		hwISerial(HardwareSerial &ser);
		virtual int available();
		virtual void flush();
		virtual size_t write(uint8_t byte);
		virtual int read();
		virtual size_t readBytes( char *buffer, size_t length);
	private:
		HardwareSerial &ser;
};