#include <chrono>
#include <thread>

#include "daisy_patch.h"
#include "daisysp.h"
#include <string>
#include "Utility.h"
#include "BitArray.h"
#include "Screen.h"
#include "IDrum.h"
#include "Bd8.h"
#include "Sd8.h"
#include "SdNoise.h"
#include "FmDrum.h"
#include "HhSource68.h"
#include "Ch.h"
#include "Oh.h"
#include "Cy.h"
#include "Cow8.h"

using namespace daisy;
using namespace daisysp;

#define MINIMUM_NOTE 36
#define KNOB_COUNT 4

DaisyPatch hw;
Screen screen(&hw.display);
IDrum *drums[16];
uint8_t drumCount = 4;
Bd8 bd;
SdNoise rs;
Sd8 sd;
FmDrum cp;
Ch ch;
Oh oh;
Cy cy;
Cow8 cb;

HhSource68 source68;

uint8_t currentDrum = 0;
uint8_t currentKnobRow = 0;


void OledMessage(std::string message, int row) 
{
    char* mstr = &message[0];
    hw.display.SetCursor(0, row * 10);
    hw.display.WriteString(mstr, Font_6x8, true);
    hw.display.Update();
}

void OledMessage(std::string message, int row, int column) 
{
    char* mstr = &message[0];
    hw.display.SetCursor(column * 8, row * 10);
    hw.display.WriteString(mstr, Font_6x8, true);
    hw.display.Update();
}

// Display the available parameter names.
void DisplayParamMenu() {

    hw.display.DrawRect(0, 0, 127, 30, false, true);
    hw.display.DrawLine(0,10,127,10, true);

    uint8_t param;
    for (int knob = 0; knob < KNOB_COUNT; knob++) {
        for (u8 row = 0; row <= drums[currentDrum]->PARAM_COUNT / 4; row++) {
        // for (u8 row = 0; row < 2; row++) {
            Rectangle rect2(knob * 32, (row + 1) * 11, 32, 11);
            param = row * KNOB_COUNT + knob;
            std::string sc = drums[currentDrum]->GetParamName(param);
            bool selected = row == currentKnobRow;
            // hw.display.WriteStringAligned(sc.c_str(), Font_6x8, rect2, Alignment::centered, true);
            screen.DrawButton(rect2, sc, selected, selected, !selected);
            // hw.display.SetCursor(rect2.GetX(), rect2.GetY());
            // hw.display.WriteString(sc.c_str(), Font_6x8, true);
            hw.display.DrawLine(0, rect2.GetY() + 11, 127, rect2.GetY() + 11, true);
        }
    }
}

void ProcessEncoder() {
    int inc = hw.encoder.Increment();
    int newDrum = Utility::LimitInt(currentDrum + inc, 0, drumCount-1);
    if (newDrum != currentDrum) {
        drums[newDrum]->ResetParams();
        currentDrum = newDrum;
        screen.DrawMenu(currentDrum);
        DisplayParamMenu();
        hw.display.Update();        
    }
}

// Process the current knob values and update model params accordingly.
// Knob number == param number, since model params are listed in UI order.
void ProcessKnobs() {

    for (int knob = 0; knob < KNOB_COUNT; knob++) {
        float sig = hw.controls[knob].Value();
        uint8_t param = currentKnobRow * KNOB_COUNT + knob;
        // drums[currentDrum]->SetParam(param, sig, true);
        drums[currentDrum]->UpdateParam(param, sig);
        // float val = drums[currentDrum]->UpdateParam(param, sig);
        // if (param == 0) {
        //     float scaled = Utility::ScaleFloat(sig, 20, 5000, Parameter::EXPONENTIAL);
        //     OledMessage("Upd: " + std::to_string(param) + ", " + std::to_string((int)(sig*1000)) + ", " + std::to_string((int)scaled) + ", " + std::to_string((int)val) + "     ", 4);
        // }
    }
}

// Display the current values and parameter names of model params for 4 knobs.
// Knob number == param number, since model params are listed in UI order.
void DisplayKnobValues() {

    hw.display.DrawRect(0, 0, 127, 30, false, true);
    hw.display.DrawLine(0,10,127,10, true);

    uint8_t param;
    for (int knob = 0; knob < KNOB_COUNT; knob++) {
        // top row: current param values from knobs
        param = currentKnobRow * KNOB_COUNT + knob;
        Rectangle rect(knob * 32, 0, 32, 8);
        std::string sc = drums[currentDrum]->GetParamString(param);
        hw.display.WriteStringAligned(sc.c_str(), Font_6x8, rect, Alignment::centered, true);
        // screen.DrawButton(rect, sc, false, true, false);

        for (u8 row = 0; row <= drums[currentDrum]->PARAM_COUNT / 4; row++) {
            Rectangle rect2(knob * 32, (row + 1) * 11, 32, 11);
            param = row * KNOB_COUNT + knob;
            sc = drums[currentDrum]->GetParamName(param);
            bool selected = row == currentKnobRow;
            // hw.display.WriteStringAligned(sc.c_str(), Font_6x8, rect2, Alignment::centered, true);
            screen.DrawButton(rect2, sc, selected, selected, !selected);
            // hw.display.SetCursor(rect2.GetX(), rect2.GetY());
            // hw.display.WriteString(sc.c_str(), Font_6x8, true);
            hw.display.DrawLine(0, rect2.GetY() + 11, 127, rect2.GetY() + 11, true);
        }
    }
}

void ProcessControls()
{
    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();

    ProcessEncoder();
    ProcessKnobs();
    // ProcessGates();
}


void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{

    ProcessControls();

    for(size_t i = 0; i < size; i++)
    {

        float src = source68.Process();

        float sig = 0.0f;
        for (uint8_t i = 0; i < drumCount; i++) {
            sig += drums[i]->Process();
        }
        float limited = sig / drumCount;

        out[0][i] = out[1][i] = limited;
        // out[2][i] = out[3][i] = sig;
        out[2][i] = out[3][i] = src;

    }
}


// Typical Switch case for Message Type.
void HandleMidiMessage(MidiEvent m)
{

    switch(m.type)
    {
        case NoteOn:
        {
            // NoteOnEvent p = m.AsNoteOn();
            // This is to avoid Max/MSP Note outs for now..
            // if(m.data[1] != 0)
            // {
            //     p = m.AsNoteOn();
            //     osc.SetFreq(mtof(p.note));
            //     osc.SetAmp((p.velocity / 127.0f));
            // } else {
            //     osc.SetAmp(0.0f);
            // }
            // OledMessage("Midi: " + std::to_string(p.note) + ", " + std::to_string(p.velocity) + "     ", 3);

            NoteOnEvent p = m.AsNoteOn();
            float velocity = p.velocity / 127.0f;
            if (p.velocity > 0) {
                if (p.note >= MINIMUM_NOTE && p.note < MINIMUM_NOTE + drumCount) {
                    drums[p.note - MINIMUM_NOTE]->Trigger(velocity);
                }
            }
        }
        break;
        case NoteOff:
        {
        }
        break;
        case ControlChange:
        {
            ControlChangeEvent p = m.AsControlChange();
            switch(p.control_number)
            {
                case 1:
                    break;
                case 2:
                    break;
                default: break;
            }
            break;
        }
        default: break;
    }
}


// Main -- Init, and Midi Handling
int main(void)
{
    // Init
    float samplerate;
    hw.Init();
    samplerate = hw.AudioSampleRate();

    drums[0] = &bd;
    drums[1] = &rs;
    drums[2] = &sd;
    drums[3] = &cp;
    drumCount = 4;
    currentDrum = 0;

    for (uint8_t i = 0; i < drumCount; i++) {
        drums[i]->Init(samplerate);
    }

    drums[4] = &ch;
    drums[5] = &oh;
    drums[6] = &cy;
    drums[7] = &cb;
    drumCount = 8;
    source68.Init(samplerate, HhSource68::MORPH_808_VALUE);
    ch.Init(samplerate, 0.001, 0.5, &source68, HhSource68::MORPH_808_VALUE, 6000, 16000);
    oh.Init(samplerate, 0.001, 0.13, 0.05, &source68, HhSource68::MORPH_808_VALUE, 6000, 16000);
    cy.Init(samplerate, 0.001, 3.5, &source68, 1700, 2400);
    cb.Init(samplerate, 0.001, 0.5, &source68, 1700, 2400);

    //display
    hw.display.Fill(false);
    // OledMessage("Hachikit 0.1", 5);
    // Utility::DrawDrums(&hw.display, 5);
    screen.DrawMenu(currentDrum);
    hw.display.Update();


    // Start stuff.
    hw.SetAudioBlockSize(128);
    hw.midi.StartReceive();
    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    int c = 0;
    for(;;)
    {
        hw.midi.Listen();
        // Handle MIDI Events
        while(hw.midi.HasEvents())
        {
            HandleMidiMessage(hw.midi.PopEvent());
        }
        DisplayKnobValues();
        hw.display.Update();
        c++;
    }
}
