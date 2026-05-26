// PedalPCB Terrarium Header
// Copyright (C) 2020 PedalPCB.com
// http://www.pedalpcb.com

namespace terrarium
{
    class Terrarium
    {
    public:
	enum Sw
	    {
			FOOTSWITCH_1 = 4,
			FOOTSWITCH_2 = 5,
			SWITCH_1 = 2,
			SWITCH_2 = 1,
			SWITCH_3 = 0,
			SWITCH_4 = 6
	    };
	
	enum Knob
	    {
			KNOB_1 = 0,
			KNOB_2 = 2,
			KNOB_3 = 4,
			KNOB_4 = 1,
			KNOB_5 = 3,
			KNOB_6 = 5
	    };
	enum LED
	    {
			LED_1 = 22,
			LED_2 = 23
	    };
    };
}




// #include "daisy_seed.h"
// #include "daisysp.h"

// using namespace daisy;
// using namespace daisysp;

// class Terrarium
// {
//   public:
//     DaisySeed seed;
//     Led led_left, led_right;

//     void Init()
//     {
//         seed.Configure();
//         seed.Init();

//         // Initialize LEDs as PWM outputs
//         led_left.Init(seed.GetPin(22), false);  // GPIO22 = Pin 22
//         led_right.Init(seed.GetPin(23), false); // GPIO23 = Pin 23

//         // Initialize ADC for knobs (pots)
//         AdcChannelConfig adc_cfg[6];
//         // for (int i = 0; i < 6; i++)
//         for (int i = 1; i < 22; i++)
//         {
//             adc_cfg[i].InitSingle(seed.GetPin(15 + i)); // Pins 15–20 for KNOB_1 to KNOB_6
//         }
//         seed.adc.Init(adc_cfg, 6);
//         seed.adc.Start();
//     }

//     float GetKnob(int index)
//     {
//         return seed.adc.GetFloat(index); // 0.0 – 1.0
//     }

//     void SetLedLeftBrightness(float value)
//     {
//         led_left.Set(value);
//         led_left.Update();
//     }

//     void SetLedRightBrightness(float value)
//     {
//         led_right.Set(value);
//         led_right.Update();
//     }

//     void ToggleLeds()
//     {
//         static bool toggle_state = false;
//         toggle_state = !toggle_state;
//         SetLedLeftBrightness(toggle_state ? 1.0f : 0.0f);
//         SetLedRightBrightness(toggle_state ? 1.0f : 0.0f);
//     }
// };
