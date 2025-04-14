#include "Screen.h"

using namespace daisy;
using namespace daisysp;

const FontDef Screen::FONT = Font_6x8;
const std::string Screen::menuItems[] = {   "BD", "RS", "SD", "CP", 
                                    "S2", "LT", "CH", "MT",
                                    "MA", "HT", "OH", "LC", 
                                    "HC", "CY", "CB", "CL" };
#define MENU_SIZE 16

void Screen::DrawRect(uint_fast8_t x1, uint_fast8_t y1, uint_fast8_t x2, uint_fast8_t y2, bool on, bool fill) {
    if (!screenOn) { return; }

    display->DrawRect(x1, y1, x2, y2, on, fill);
}

Rectangle Screen::WriteStringAligned(
    const char*    str,
    const FontDef& font,
    Rectangle      boundingBox,
    Alignment      alignment,
    bool           on) {

        if (!screenOn) { return boundingBox; }

        return display->WriteStringAligned(str, font, boundingBox, alignment, on);
}

void Screen::DrawLine(
    uint_fast8_t x1,
    uint_fast8_t y1,
    uint_fast8_t x2,
    uint_fast8_t y2,
    bool         on) {

        if (!screenOn) { return; }

        display->DrawLine(x1, y1, x2, y2, on);
}

void Screen::DrawRect(Rectangle rect, bool on, bool fill) {
    if (!screenOn) { return; }

    display->DrawRect(rect.GetX(), rect.GetY(), rect.GetX() + rect.GetWidth(), rect.GetY() + rect.GetHeight(), on, fill);
}

void Screen::DrawRectFilled(Rectangle rect, bool border, bool fill) {
    if (!screenOn) { return; }

    DrawRect(rect, fill, true);
    DrawRect(rect, border, false);
}

void Screen::DrawButton(Rectangle rect, std::string str, bool border, bool fill, bool text) {
    if (!screenOn) { return; }

    DrawRectFilled(rect, border, fill);
    // display->WriteStringAligned(str.c_str(), FONT, rect, Alignment::centered, text);
    display->SetCursor(rect.GetX() + 2, rect.GetY() + 2);
    display->WriteString(str.c_str(), FONT, text);
}

void Screen::DrawMenu(uint8_t selected) {
    if (!screenOn) { return; }

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

void Screen::SetScreenOn(bool screenOn) { 
    this->screenOn = screenOn; 
    if (!screenOn) {
        screenCounter = 0;
    }
}

void Screen::Screensave() {
    if (screenOn) { return; }

    // horizontal wipe
    u8 x = (screenCounter / 8) % (WIDTH + 1);
    display->DrawLine(x, 0, x, HEIGHT, false);
    if (x < WIDTH - 1) {
        display->DrawLine(x + 1, 0, x + 1, HEIGHT, true);
    }
    screenCounter++;
}

void Screen::ScreensaveEvent(u8 drum) {
    if (screenOn) { return; }

    // show notes with horizontal wipe
    u8 x = (screenCounter / 8) % (WIDTH + 1);
    if (x > 6 && x < WIDTH - 2) {
        display->DrawCircle(x - 4, (15-drum) * 4 + 1, 1, true);
    }
}

void Screen::OledMessage(std::string message, int row) {
    if (!screenOn) { return; }

    char* mstr = &message[0];
    display->SetCursor(0, row * 10);
    display->WriteString(mstr, Font_6x8, true);
    display->Update();
}

void Screen::OledMessage(std::string message, int row, int column) {
    if (!screenOn) { return; }

    char* mstr = &message[0];
    display->SetCursor(column * 8, row * 10);
    display->WriteString(mstr, Font_6x8, true);
    display->Update();
}

