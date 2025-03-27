// Copyright 2015 Emilie Gillet.
//
// Author: Emilie Gillet (emilie.o.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// See http://creativecommons.org/licenses/MIT/ for more information.

#include "dsp/part.h"
#include "dsp/strummer.h"
#include "dsp/string_synth_part.h"
#include "cv_scaler.h"
#include "storage.h"
#include "calibrate.h"
#include "daisy_patch_sm.h"

using namespace daisy;
using namespace patch_sm;
using namespace torus;

enum SettingsState
{
    NONE,
    CALIBRATE,
    NORMALIZE,
};

DaisyPatchSM hw;

uint16_t reverb_buffer[32768];

Calibrate                   calibration;
CvScaler                    cv_scaler;
Part                        part;
StringSynthPart             string_synth;
Strummer                    strummer;
Switch                      mode_button, shift_switch;
PersistentStorage<Settings> storage(hw.qspi);
Settings                    default_settings{
    0.f, // offset
    1.f, // scale
    // On first boot the strum is normalized to accept a strum in to make it
    // simpler to get sound out.
    true,  // strum_in
    false, // note_in
    false, // exciter_in
    false, // easter_egg_on
    0,     // model_mode
    0,     // poly_mode
    0,     // egg_mode
};
SettingsState settings_state{NONE};
bool          trigger_save;
bool          shift;
bool          toggle_easter_egg_mode;

// control items
int         kNumControls = 5;
const char* controlListValues[]
    = {"Frequency", "Structure", "Brightness", "Damping", "Position"};
MappedStringListValue controlListValueOne(controlListValues, kNumControls, 1);
MappedStringListValue controlListValueTwo(controlListValues, kNumControls, 2);
MappedStringListValue controlListValueThree(controlListValues, kNumControls, 3);
MappedStringListValue controlListValueFour(controlListValues, kNumControls, 4);

// poly/model items
int                   kNumPolyModes    = 3;
const char*           polyListValues[] = {"One", "Two", "Four"};
MappedStringListValue polyListValue(polyListValues, kNumPolyModes, 0);
int                   kNumModels        = 6;
const char*           modelListValues[] = {"Modal",
                                           "Symp Str",
                                           "Inhrm Str",
                                           "Fm Voice",
                                           "Westn Chrd",
                                           "Str & Verb"};
MappedStringListValue modelListValue(modelListValues, kNumModels, 0);
int                   kNumEggModes = 6;
const char*           eggListValues[]
    = {"Formant", "Chorus", "Reverb", "Formant2", "Ensemble", "Reverb2"};
MappedStringListValue eggListValue(eggListValues, kNumEggModes, 0);

int old_poly = 0;

float IndexToBrightness(int index, int total)
{
    return static_cast<float>(index + 1) / static_cast<float>(total);
}

void ProcessControls(Patch* patch, PerformanceState* state)
{
    Settings& settings_data = storage.GetSettings();
    hw.ProcessAllControls();
    mode_button.Debounce();
    shift_switch.Debounce();
    shift = shift_switch.Pressed();
    float led_brightness{0.f};

    if(shift)
    {
        // In shift mode the first knob controls the frequency.
        controlListValueOne.SetIndex(0);

        if(mode_button.TimeHeldMs() >= 5000)
        {
            // Light up the LED to indicate that releasing the button will enter
            // the calibration state.
            led_brightness = 1.f;
            if(settings_state != CALIBRATE)
            {
                // Holding the mode button while in shift mode enters the calibration
                // state.
                settings_state = CALIBRATE;
                calibration.Start();
            }
        }
        else if(mode_button.TimeHeldMs() >= 3000 && settings_state != NORMALIZE)
        {
            // Holding the mode button while in shift mode enters the normalizing
            // state.
            settings_state = NORMALIZE;
        }
        else if(settings_state == CALIBRATE)
        {
            float cv      = hw.GetAdcValue(CV_5);
            bool  pressed = mode_button.RisingEdge();
            if(calibration.ProcessCalibration(cv, pressed))
            {
                calibration.cal.GetData(settings_data.scale,
                                        settings_data.offset);
                trigger_save   = true;
                settings_state = NONE;
            }
            led_brightness = calibration.GetBrightness();
        }
        else if(settings_state == NORMALIZE)
        {
            // Turning the knobs to the right sets normalize on, left turns
            // them off.
            settings_data.strum_in   = hw.GetAdcValue(0) > .5f;
            settings_data.note_in    = hw.GetAdcValue(1) > .5f;
            settings_data.exciter_in = hw.GetAdcValue(2) > .5f;
            led_brightness = (System::GetNow() & 255) > 127 ? 1.f : 0.f;

            if(mode_button.RisingEdge())
            {
                // Pressing the mode button again exits normalize.
                settings_state = NONE;
                trigger_save   = true;
            }
        }
        else if(mode_button.RisingEdge())
        {
            // In shift mode the mode button rotates through the poly modes.
            if(polyListValue.GetIndex() + 1 >= kNumPolyModes)
            {
                polyListValue.SetIndex(0);
            }
            else
            {
                polyListValue.Step(1, false);
            }
            settings_data.poly_mode = polyListValue.GetIndex();
            trigger_save            = true;
        }

        if(settings_state == NONE)
        {
            // Set the brightness in relation to the position as it rotates
            // through the modes.
            led_brightness
                = IndexToBrightness(polyListValue.GetIndex(), kNumPolyModes);
        }
    }
    else
    {
        // In normal mode the first knob controls the structure.
        controlListValueOne.SetIndex(1);

        if(mode_button.TimeHeldMs() >= 3000)
        {
            // Check that the mode hasn't already been toggled so that it
            // doesn't flip the state on every loop.
            if(!toggle_easter_egg_mode)
            {
                // Holding the mode button in normal mode will toggle the easter egg.
                settings_data.easter_egg_on = !settings_data.easter_egg_on;
                toggle_easter_egg_mode      = true;
                trigger_save                = true;
            }
        }
        else if(mode_button.RisingEdge())
        {
            if(settings_data.easter_egg_on)
            {
                // If easter egg is on then the mode button rotates through the
                // easter egg modes.
                if(eggListValue.GetIndex() + 1 >= kNumEggModes)
                {
                    eggListValue.SetIndex(0);
                }
                else
                {
                    eggListValue.Step(1, false);
                }
                settings_data.egg_mode = eggListValue.GetIndex();
                trigger_save           = true;
            }
            else
            {
                // In normal mode the mode button rotates through the models.
                if(modelListValue.GetIndex() + 1 >= kNumModels)
                {
                    modelListValue.SetIndex(0);
                }
                else
                {
                    modelListValue.Step(1, false);
                }
                settings_data.model_mode = modelListValue.GetIndex();
                trigger_save             = true;
            }
        }

        if(mode_button.FallingEdge())
        {
            // Clear the easter egg setting state when the button is released.
            toggle_easter_egg_mode = false;
        }

        if(toggle_easter_egg_mode)
        {
            // Flash the LED to indicate that the easter egg mode will toggle.
            led_brightness = (System::GetNow() & 255) > 127 ? 1.f : 0.f;
        }
        else if(settings_data.easter_egg_on)
        {
            // Set the brightness in relation to the position as it rotates
            // through the easter egg modes.
            led_brightness
                = IndexToBrightness(eggListValue.GetIndex(), kNumEggModes);
        }
        else
        {
            // Set the brightness in relation to the position as it rotates
            // through the models.
            led_brightness
                = IndexToBrightness(modelListValue.GetIndex(), kNumModels);
        }
    }
    hw.WriteCvOut(CV_OUT_2, led_brightness * 5.f);

    // control settings
    cv_scaler.MapChannel(0, controlListValueOne.GetIndex());
    cv_scaler.MapChannel(1, controlListValueTwo.GetIndex());
    cv_scaler.MapChannel(2, controlListValueThree.GetIndex());
    cv_scaler.MapChannel(3, controlListValueFour.GetIndex());
    cv_scaler.MapChannel(4, CV_V_OCT);
    cv_scaler.MapChannel(5, CV_STRUCTURE);
    cv_scaler.MapChannel(6, CV_BRIGHTNESS);
    cv_scaler.MapChannel(7, CV_POSITION);

    //polyphony setting
    int poly = polyListValue.GetIndex();
    if(old_poly != poly)
    {
        part.set_polyphony(0x01 << poly);
        string_synth.set_polyphony(0x01 << poly);
    }
    old_poly = poly;

    //model settings
    part.set_model((torus::ResonatorModel)modelListValue.GetIndex());
    string_synth.set_fx((torus::FxType)eggListValue.GetIndex());

    // normalization settings
    state->internal_note    = !settings_data.note_in;
    state->internal_exciter = !settings_data.exciter_in;
    state->internal_strum   = !settings_data.strum_in;

    //strum
    state->strum = hw.gate_in_1.Trig();
}

float input[kMaxBlockSize];
float output[kMaxBlockSize];
float aux[kMaxBlockSize];

const float kNoiseGateThreshold = 0.00003f;
float       in_level            = 0.0f;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    Settings&        settings_data = storage.GetSettings();
    PerformanceState performance_state;
    Patch            patch;

    ProcessControls(&patch, &performance_state);
    cv_scaler.Read(&patch, &performance_state, &calibration.cal);

    if(settings_data.easter_egg_on)
    {
        for(size_t i = 0; i < size; ++i)
        {
            input[i] = in[0][i];
        }
        strummer.Process(NULL, size, &performance_state);
        string_synth.Process(
            performance_state, patch, input, output, aux, size);
    }
    else
    {
        // Apply noise gate.
        for(size_t i = 0; i < size; i++)
        {
            float in_sample = in[0][i];
            float error, gain;
            error = in_sample * in_sample - in_level;
            in_level += error * (error > 0.0f ? 0.1f : 0.0001f);
            gain     = in_level <= kNoiseGateThreshold
                           ? (1.0f / kNoiseGateThreshold) * in_level
                           : 1.0f;
            input[i] = gain * in_sample;
        }

        strummer.Process(input, size, &performance_state);
        part.Process(performance_state, patch, input, output, aux, size);
    }

    for(size_t i = 0; i < size; i++)
    {
        out[0][i] = output[i];
        out[1][i] = aux[i];
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(kMaxBlockSize);
    float samplerate = hw.AudioSampleRate();
    float blocksize  = hw.AudioBlockSize();
    InitResources();

    mode_button.Init(hw.B7,
                     samplerate,
                     Switch::TYPE_MOMENTARY,
                     Switch::POLARITY_INVERTED,
                     Switch::PULL_UP);
    shift_switch.Init(hw.B8,
                      samplerate,
                      Switch::TYPE_TOGGLE,
                      Switch::POLARITY_NORMAL,
                      Switch::PULL_UP);

    strummer.Init(0.01f, samplerate / blocksize);
    part.Init(reverb_buffer);
    string_synth.Init(reverb_buffer);

    storage.Init(default_settings);
    if(storage.GetState() == PersistentStorage<Settings>::State::FACTORY)
    {
        // Update the user values with the defaults.
        storage.RestoreDefaults();
    }
    /** Restore settings from previous power cycle */
    Settings& settings_data = storage.GetSettings();
    calibration.cal.SetData(settings_data.scale, settings_data.offset);
    polyListValue.SetIndex(settings_data.poly_mode);
    eggListValue.SetIndex(settings_data.egg_mode);
    modelListValue.SetIndex(settings_data.model_mode);
    cv_scaler.Init();

    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    while(1)
    {
        // Without this pause the save events get missed for some unknown reason.
        hw.Delay(1);
        if(trigger_save)
        {
            storage.Save();
            trigger_save = false;
        }
    }
}
