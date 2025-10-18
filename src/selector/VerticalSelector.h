#ifndef VERTICAL_SELECTOR_H
#define VERTICAL_SELECTOR_H

#include <Arduino.h>
#include <string>
#include <vector>
#include <cardputer/CardputerView.h>
#include <cardputer/CardputerInput.h>

class VerticalSelector {
public:
    VerticalSelector(CardputerView& display, CardputerInput& input);
    int select(const std::string& title, const std::vector<std::string>& options, bool subMenu = false, bool searchBar = false,  const std::vector<std::string>& options2={},  const std::vector<std::string>& shortcuts={}, bool visibleMention=false, bool handleInactivity=true);

private:
    CardputerView& display;
    CardputerInput& input;

    std::string toLowerCase(const std::string& input);
    std::vector<std::string> filterOptions(const std::vector<std::string>& options, const std::string& query);
    int checkShortcut(const std::vector<std::string>& shortcuts, char key);
};

#endif // VERTICAL_SELECTOR_H
