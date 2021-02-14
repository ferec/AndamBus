#ifndef TESTITEM_H
#define TESTITEM_H

#include <inttypes.h>

class TestItem
{
    public:
        TestItem(uint8_t id, uint8_t parentId, uint8_t pin, uint8_t type, uint16_t counter);
        virtual ~TestItem();

        int getValue() { return value; }
        uint16_t getLastChange() { return lastChange; }
        uint16_t getCreated() { return created; }

        void setValue(int value, uint16_t counter);
        void setData(const char *data, uint8_t size);

        uint8_t id, pin, type, parentId;
    protected:

    private:
        uint16_t lastChange, created;
        int value;
};

#endif // TESTITEM_H
