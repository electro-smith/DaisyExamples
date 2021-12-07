#pragma once

#include "daisy.h"
#include "CustomPages.h"
#include "daisy_patch.h"

#define PIN_OLED_DC 9
#define PIN_OLED_RESET 30

namespace daisy
{
void ClearDisplay(const daisy::UiCanvasDescriptor& canvas)
{
    display_.Fill(false);
}

void FlushDisplay(const daisy::UiCanvasDescriptor& canvas)
{
    display_.Update();
}

class TorusPatchUI : public UI
{
  public:
    ~TorusPatchUI(){};

    void InitDisplay(DaisyPatch* hw)
    {
        //there should be a way to set OledDisplay::driver_, instead we just reinit
        OledDisplay<SSD130x4WireSpi128x64Driver>::Config display_config;
        display_config.driver_config.transport_config.pin_config.dc
            = hw->seed.GetPin(PIN_OLED_DC);
        display_config.driver_config.transport_config.pin_config.reset
            = hw->seed.GetPin(PIN_OLED_RESET);

        display_.Init(display_config);

        desc_.handle_        = &hw->display;
        desc_.clearFunction_ = ClearDisplay;
        desc_.flushFunction_ = FlushDisplay;
    }

    void DoEvents()
    {
        // menu_one_.Draw(desc_);
        Rectangle rect;
        menu_one_.Draw(display_, false, false, rect, false);
    }

    void GenerateEvents() {}

  private:
    UiCanvasDescriptor                                    desc_;
    MenuOne                                               menu_one_;
    OledDisplay<OledDisplay<SSD130x4WireSpi128x64Driver>> display_;
};

} // namespace daisy