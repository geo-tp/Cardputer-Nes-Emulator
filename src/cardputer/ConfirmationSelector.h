#ifndef CONFIRMATION_SELECTOR_H
#define CONFIRMATION_SELECTOR_H

#include <string>
#include <Arduino.h>
#include "cardputer/CardputerInput.h"
#include "cardputer/CardputerView.h"

class ConfirmationSelector {
public:
    ConfirmationSelector(CardputerView& display, CardputerInput& input);
    bool select(const std::string& title, const std::string& description);

private:
    CardputerView& display;
    CardputerInput& input;
};

#endif // CONFIRMATION_SELECTOR_H
