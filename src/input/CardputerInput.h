#ifndef CARDPUTER_INPUT_H
#define CARDPUTER_INPUT_H

#include <map>
#include <M5Cardputer.h>

#define KEY_OK '\n'
#define KEY_DEL '\b'
#define KEY_ESC_CUSTOM '`'
#define KEY_NONE '\0'
#define KEY_RETURN_CUSTOM '\r'
#define KEY_ARROW_UP ';'
#define KEY_ARROW_DOWN '.'
#define KEY_ARROW_LEFT ','
#define KEY_ARROW_RIGHT '/'

class CardputerInput {
public:
    char handler();
    void waitPress();
    void flushInput(size_t ms);
};
    
#endif // CARDPUTER_INPUT_H
