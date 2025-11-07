#ifndef CARDPUTER_VIEW_H
#define CARDPUTER_VIEW_H

#include <vector>
#include <string>
#include <cstring>
#include <M5Cardputer.h>

// SIZING
#define DEFAULT_MARGIN 5
#define DEFAULT_ROUND_RECT 5
#define TOP_BAR_HEIGHT 30

// PALETTE
#define BACKGROUND_COLOR TFT_BLACK
#define PRIMARY_COLOR 0xfc20
#define RECT_COLOR_DARK 0x0841
#define RECT_COLOR_LIGHT 0xd69a
#define TEXT_COLOR 0xef7d 
#define SMS_COLOR      0x1C9F  // Bleu clair (≈ #1E90FF)
#define GENESIS_COLOR 0x32BB // Bleu cobalt  (≈ #2a2a9aff)
#define NES_COLOR      0xFA47  // Rouge orange (≈ #FF4A3D)
#define GAMEGEAR_COLOR 0xC29F  // Violet/magenta  (≈ #C050FF)
#define NEOGEO_COLOR   0x05E8 // Vert (≈ #2a9333ff)
#define FOLDER_COLOR   0xFEC0  // Jaune pâle (≈ #F6EB61)

// TEXT SIZE
#define TEXT_BIG 2
#define TEXT_LARGE 1.9
#define TEXT_WIDE 1.4
#define TEXT_MEDIUM_LARGE 1.2
#define TEXT_MEDIUM 1.1
#define TEXT_SMALL 1.0
#define TEXT_TINY 0.9

class CardputerView {
public:
    void initialize();
    void setBrightness(uint16_t brightness);
    uint8_t getBrightness();
    void welcome();
    void showKeymapping(bool threeButtons = false);
    void topBar(const std::string& title, bool submenu, bool searchBar);
    void horizontalSelection(const std::vector<std::string>& options, uint16_t selectedIndex, const std::string& description1="", const std::string& description2="", const std::vector<std::string>& icons={});
    void verticalSelection(
        const std::vector<std::string>& options,
        uint16_t selectedIndex,
        size_t visibleRows = 4,
        const std::vector<std::string>& optionLabels = {},
        const std::vector<std::string>& shortcuts = {},
        bool visibleMention=false
    );
    void value(std::string label, std::string val);
    void subMessage(std::string message, int delayMs);
    void stringPrompt(std::string label, std::string value, bool backButton, size_t minLength);
    void confirmationPrompt(std::string label);
    void debug(const std::string& message);
    void drawVaultIcon(int x = 86, int y = 10, uint16_t color = PRIMARY_COLOR, size_t w=70, size_t h=50);
    void drawFileIcon(int x = 92, int y = 10);
    void drawSettingsIcon(int x = 80, int y = 5);
    void drawSdCardIcon(int x=90, int y=10);
    void drawPlusIcon(int x=120, int y=47, uint16_t color = PRIMARY_COLOR);
    void drawMinusIcon(int x=120, int y=47, uint16_t color = PRIMARY_COLOR);
    void drawLockIcon(int x=120, int y=78, uint16_t color = PRIMARY_COLOR, size_t w=60, size_t h=45);
    void drawSelectedRowMarquee(const std::string& text, uint16_t rowInPage, size_t visibleRows);
    void showValidExt(const std::vector<std::string>& exts,
                        const char* title = "Supported files");
private:
    static M5GFX* Display; 
    void drawRect(bool selected, uint8_t margin, uint16_t startY, uint16_t sizeX, uint16_t sizeY, uint16_t stepY);
    void drawSubMenuReturn(uint8_t x, uint8_t y);
    void drawSearchIcon(int x, int y, int size, uint16_t color);
    void clearMainView(uint8_t offsetY = 0);
    void clearTopBar();
    int getCenterOffset(const std::string& text, int screenWidth=240);
    std::string truncateString(const std::string& input, size_t maxLength);
    void adjustTextSizeToFit(const std::string& text, uint16_t maxWidth, float textSize);
    void horizontalSelectionWithIcons(
        const std::vector<std::string>& options,
        uint16_t selectedIndex,
        const std::vector<std::string>& icons);
    void horizontalSelectionWithoutIcons(
        const std::vector<std::string>& options,
        uint16_t selectedIndex,
        const std::string& description1,
        const std::string& description2);
    void verticalSelectionSimple(
        const std::vector<std::string>& options,
        uint16_t selectedIndex,
        size_t visibleRows = 4
    );
    void verticalSelectionWithLabelsAndShortcuts(
        const std::vector<std::string>& options,
        uint16_t selectedIndex,
        size_t visibleRows = 4,
        const std::vector<std::string>& optionLabels = {},
        const std::vector<std::string>& shortcuts = {},
        bool visibleMention=false
    );
    uint16_t colorForExt(const std::string& ext) const;
};

#endif
