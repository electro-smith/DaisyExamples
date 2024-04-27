#include "Screen.h"

using namespace daisy;
using namespace daisysp;

const FontDef Screen::FONT = Font_6x8;
const std::string Screen::menuItems[] = {   "BD", "RS", "SD", "CP", 
                                    "S2", "LT", "CH", "MT",
                                    "MA", "HT", "OH", "LC", 
                                    "HC", "CY", "CB", "CL" };
#define MENU_SIZE 16

void Screen::DrawRect(Rectangle rect, bool on, bool fill) {
    display->DrawRect(rect.GetX(), rect.GetY(), rect.GetX() + rect.GetWidth(), rect.GetY() + rect.GetHeight(), on, fill);
}

void Screen::DrawRectFilled(Rectangle rect, bool border, bool fill) {
    DrawRect(rect, fill, true);
    DrawRect(rect, border, false);
}

void Screen::DrawButton(Rectangle rect, std::string str, bool border, bool fill, bool text) {
        DrawRectFilled(rect, border, fill);
        // display->WriteStringAligned(str.c_str(), FONT, rect, Alignment::centered, text);
        display->SetCursor(rect.GetX() + 2, rect.GetY() + 2);
        display->WriteString(str.c_str(), FONT, text);
}

void Screen::DrawMenu(uint8_t selected) {

    uint8_t itemWidth = FONT.FontWidth * 2 + 3;
    uint8_t itemHeight = FONT.FontWidth + 4;
    uint8_t displayCount = std::min(WIDTH / itemWidth, MENU_SIZE);
    uint8_t highlightPos = displayCount / 2;
    // highlightPos = 4;
    uint8_t start = std::min(std::max(0, selected - highlightPos), MENU_SIZE - displayCount);

    for (uint8_t pos = start; pos < start + displayCount; pos++) {
        bool sel = pos == selected;
        uint8_t x = itemWidth * (pos - start);
        uint8_t y = HEIGHT - itemHeight;
        Rectangle rect(x, y, itemWidth, itemHeight);
        DrawButton(rect, menuItems[pos], true, sel, !sel);
    }

}
