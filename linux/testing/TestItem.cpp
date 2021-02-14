#include "TestItem.h"
#include "util.h"

using namespace std;

TestItem::TestItem(uint8_t _id, uint8_t _parentId, uint8_t _pin, uint8_t _type, uint16_t counter):id(_id),pin(_pin),type(_type),parentId(_parentId),lastChange(0),created(counter),value(0)
{
    //ctor
}

TestItem::~TestItem()
{
    //dtor
}

void TestItem::setValue(int _value, uint16_t counter) {
    value = _value;

    lastChange = counter;
}

void TestItem::setData(const char *data, uint8_t size) {
    AB_INFO("setData " << (int)size);
}
