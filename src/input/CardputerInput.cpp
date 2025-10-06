#include "CardputerInput.h"

char CardputerInput::handler() {

    // Update keyboard state
    M5Cardputer.update();

    // Bouton GO
    if (M5Cardputer.BtnA.isPressed()) {
        delay(150); // debounce
        return KEY_ESC_CUSTOM;
    }
    
    if (M5Cardputer.Keyboard.isChange()) {

        if (M5Cardputer.Keyboard.isPressed()) {
            Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

            if (status.enter) { // go to next menu
                return KEY_OK;
            }
            if (status.del) { 
                return KEY_DEL;
            }
            
            if(M5Cardputer.Keyboard.isKeyPressed(KEY_ARROW_LEFT)) { // go back to previous menu
                return KEY_ARROW_LEFT;
            }

            if(M5Cardputer.Keyboard.isKeyPressed(KEY_ARROW_RIGHT)) { // go to next menu
                return KEY_ARROW_RIGHT;
            }

            for (auto c : status.word) {
                // Issue with %, the only key that requires 2 inputs to display
                if (c == '%') {
                    return '5'; 
                }
                return c; // return first char only
            }
        }
    }
    delay(10); // debounce
    return KEY_NONE;
}

void CardputerInput::waitPress() {
  while(1){
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange()) {
      if (M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
        return;
      }
    }
    delay(10);
  }
}

void CardputerInput::flushInput(size_t ms) {
    unsigned long start = millis();
    while(millis() - start < ms){
        M5Cardputer.update();
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
        status.reset();
        delay(1);
    }
}