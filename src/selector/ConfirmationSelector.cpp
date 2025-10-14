#include "ConfirmationSelector.h"

ConfirmationSelector::ConfirmationSelector(CardputerView& display, CardputerInput& input)
    : display(display), input(input) {}

bool ConfirmationSelector::select(const std::string& title, const std::string& description) {
    char key = KEY_NONE;
    display.topBar(title, false, false);
    display.confirmationPrompt(description);
    while (true) {
        key = input.readChar();
        if (key == KEY_OK) {
            return true;
        }
        if (key == KEY_ESC_CUSTOM || key == KEY_ARROW_LEFT) {
            return false;
        }
        delay(5);
    }
}
