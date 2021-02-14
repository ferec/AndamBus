#ifndef PERSISTENT_H_INCLUDED
#define PERSISTENT_H_INCLUDED


class Persistent {
	public:
		enum class Type:uint8_t {
			ArduinoDevice=1, ArduinoPin=2, W1Bus=3
		};
	
	
		virtual uint8_t getPersistData(uint8_t data[], uint8_t maxlen) = 0;
		virtual uint8_t restore(uint8_t data[], uint8_t length) = 0;
		virtual uint8_t typeId() = 0;
};

#endif //PERSISTENT_H_INCLUDED