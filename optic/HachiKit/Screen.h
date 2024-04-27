#ifndef SCREEN_H
#define SCREEN_H

#include "daisy_patch.h"
#include "daisysp.h"
#include <string>

using namespace daisy;
using namespace daisysp;

class Screen {

    public:
        static const uint8_t HEIGHT = 63;
        static const uint8_t WIDTH = 127;
        static const FontDef FONT;
        static const std::string menuItems[];

        void DrawRect(Rectangle rect, bool on, bool fill);
        void DrawRectFilled(Rectangle rect, bool border, bool fill);
        void DrawButton(Rectangle rect, std::string str, bool border, bool text, bool fill);

        Screen(OledDisplay<SSD130x4WireSpi128x64Driver> *display) {
            this->display = display;
        }

        void DrawMenu(uint8_t selected);
        

    private:
        OledDisplay<SSD130x4WireSpi128x64Driver> *display;

};


#endif
