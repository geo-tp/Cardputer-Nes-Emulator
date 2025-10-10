#include "VerticalSelector.h"
#include <algorithm>
#include <cctype>

VerticalSelector::VerticalSelector(CardputerView& display, CardputerInput& input)
    : display(display), input(input) {}


int VerticalSelector::select(
        const std::string& title, 
        const std::vector<std::string>& options, 
        bool subMenu, 
        bool searchBar, 
        const std::vector<std::string>& options2,
        const std::vector<std::string>& shortcuts, 
        bool visibleMention,
        bool handleInactivity) 
{
    int currentIndex = 0, lastIndex = -1, lastQuerySize = 0;
    char key = KEY_NONE;
    std::string searchQuery;
    std::vector<std::string> filteredOptions = options;

    // Marquee state
    std::string mBase;
    size_t offset = 0;
    uint32_t lastMs = 0, pauseUntil = 0;

    // Scrolling
    const uint32_t STEP_MS = 200;
    const size_t VISIBLE_CHARS = 20;
    const int VISIBLE_ROWS = 4;

    while (true) {
        const bool selectionChanged = (lastIndex != currentIndex) || (lastQuerySize != (int)searchQuery.size());

        // Full redraw
        if (selectionChanged) {
            display.topBar(searchQuery.empty() ? title : searchQuery, subMenu, searchBar);
            display.verticalSelection(filteredOptions, currentIndex, VISIBLE_ROWS, options2, shortcuts, visibleMention);
            if (!filteredOptions.empty()) mBase = filteredOptions[currentIndex];
            offset = 0;
            lastMs = millis();
            lastIndex = currentIndex;
            lastQuerySize = (int)searchQuery.size();
        } else if (!filteredOptions.empty()) {
            // Redraw partial
            const std::string& base = filteredOptions[currentIndex];
            uint32_t now = millis();

            size_t startRow = (currentIndex / VISIBLE_ROWS) * VISIBLE_ROWS;
            uint16_t rowInPage = currentIndex - startRow;

            if (base.size() > VISIBLE_CHARS  && now - lastMs >= STEP_MS) {
                lastMs = now;

                if (offset >= base.size() - VISIBLE_CHARS) {
                    offset = 0;
                } else {
                    offset++;
                }

                std::string view = base.substr(offset);
                display.drawSelectedRowMarquee(view, rowInPage, VISIBLE_ROWS);
            }            
        }

        // INPUT
        key = input.handler();

        if (!shortcuts.empty()) {
            int si = checkShortcut(shortcuts, key);
            if (si != -1) return si;
        }

        switch (key) {
            case KEY_ARROW_UP:
                if (!filteredOptions.empty())
                    currentIndex = (currentIndex > 0) ? currentIndex - 1 : (int)filteredOptions.size() - 1;
                break;
            case KEY_ARROW_DOWN:
                if (!filteredOptions.empty())
                    currentIndex = (currentIndex < (int)filteredOptions.size() - 1) ? currentIndex + 1 : 0;
                break;
            case KEY_OK:
                if (!filteredOptions.empty())
                    for (size_t i = 0; i < options.size(); ++i)
                        if (options[i] == filteredOptions[currentIndex]) return (int)i;
                break;
            case KEY_ESC_CUSTOM:
            case KEY_ARROW_LEFT:
                return -1;
            case KEY_DEL:
                if (searchBar && !searchQuery.empty()) {
                    searchQuery.pop_back();
                    filteredOptions = filterOptions(options, searchQuery);
                    currentIndex = 0;
                } else filteredOptions = options;
                break;
            default:
                if (searchBar && std::isalnum((unsigned char)key)) {
                    searchQuery += key;
                    filteredOptions = filterOptions(options, searchQuery);
                    currentIndex = 0;
                }
                break;
        }
    }
}

std::string VerticalSelector::toLowerCase(const std::string& input) {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::vector<std::string> VerticalSelector::filterOptions(const std::vector<std::string>& options, const std::string& query) {
    std::vector<std::string> filtered;
    std::string lowerQuery = toLowerCase(query);
    for (const auto& option : options) {
        if (toLowerCase(option).find(lowerQuery) != std::string::npos) {
            filtered.push_back(option);
        }
    }
    return filtered;
}

int VerticalSelector::checkShortcut(const std::vector<std::string>& shortcuts, char key) {
    for (size_t i = 0; i < shortcuts.size(); ++i) {
        if (!shortcuts[i].empty() && std::tolower(key) == std::tolower(shortcuts[i][0])) {
            return i;
        }
    }
    return -1;
}

