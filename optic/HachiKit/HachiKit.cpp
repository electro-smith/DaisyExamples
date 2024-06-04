#include <chrono>
#include <thread>

#include "daisy_patch.h"
#include "daisysp.h"
#include "util/CpuLoadMeter.h"
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
#include "ClickSource.h"
#include "Ch.h"
#include "Oh.h"
#include "Cy.h"
#include "Cow8.h"
#include "Tom.h"

using namespace daisy;
using namespace daisysp;

#define MINIMUM_NOTE 36
#define KNOB_COUNT 4
#define CPU_SINGLE false

DaisyPatch hw;
Screen screen(&hw.display);
CpuLoadMeter meter;

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
Tom lt, mt, ht;

// Shared sound sources
HhSource68 source68;
ClickSource clickSource;

uint8_t currentDrum = 0;
uint8_t currentKnobRow = 0;
u8 maxDrum = 1;


u8 cycle = 0;
u8 cycleLength = 8;
float savedSignal = 0.0f;


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

    hw.display.DrawRect(0, 9, 127, 36, false, true);
    // hw.display.DrawLine(0,12,127,12, true);
    // hw.display.DrawLine(0,33,127,33, true);
    // hw.display.DrawLine(0,44,127,44, true);

    uint8_t param;
    for (int knob = 0; knob < KNOB_COUNT; knob++) {
        // for (u8 row = 0; row <= drums[currentDrum]->PARAM_COUNT / 4; row++) {
        for (u8 row = 0; row < 2; row++) {
            Rectangle rect2(knob * 32, (row + 1) * 12, 32, 12);
            param = row * KNOB_COUNT + knob;
            std::string sc = drums[currentDrum]->GetParamName(param);
            bool selected = row == currentKnobRow;
            // hw.display.WriteStringAligned(sc.c_str(), Font_6x8, rect2, Alignment::centered, true);
            screen.DrawButton(rect2, sc, selected, selected, !selected);
            // hw.display.SetCursor(rect2.GetX(), rect2.GetY());
            // hw.display.WriteString(sc.c_str(), Font_6x8, true);
            // hw.display.DrawLine(0, rect2.GetY() + 11, 127, rect2.GetY() + 11, true);
        }
    }

    hw.display.DrawLine(0,11,127,11, true);
    hw.display.DrawLine(0,36,127,36, true);

}

// Display the current values and parameter names of model params for 4 knobs.
// Knob number == param number, since model params are listed in UI order.
void DisplayKnobValues() {

    hw.display.DrawRect(0, 0, 127, 11, false, true);
    // hw.display.DrawLine(0,10,127,10, true);

    uint8_t param;
    for (int knob = 0; knob < KNOB_COUNT; knob++) {
        // top row: current param values from knobs
        param = currentKnobRow * KNOB_COUNT + knob;
        Rectangle rect(knob * 32, 0, 32, 8);
        std::string sc = drums[currentDrum]->GetParamString(param);
        hw.display.WriteStringAligned(sc.c_str(), Font_6x8, rect, Alignment::centered, true);
        // screen.DrawButton(rect, sc, false, true, false);

    //     for (u8 row = 0; row <= drums[currentDrum]->PARAM_COUNT / 4; row++) {
    //         Rectangle rect2(knob * 32, (row + 1) * 11, 32, 11);
    //         param = row * KNOB_COUNT + knob;
    //         sc = drums[currentDrum]->GetParamName(param);
    //         bool selected = row == currentKnobRow;
    //         // hw.display.WriteStringAligned(sc.c_str(), Font_6x8, rect2, Alignment::centered, true);
    //         screen.DrawButton(rect2, sc, selected, selected, !selected);
    //         // hw.display.SetCursor(rect2.GetX(), rect2.GetY());
    //         // hw.display.WriteString(sc.c_str(), Font_6x8, true);
    //         hw.display.DrawLine(0, rect2.GetY() + 11, 127, rect2.GetY() + 11, true);
    //     }
    }
}

void ProcessEncoder() {

    bool redraw = false;
    int inc = hw.encoder.Increment();
    int newDrum = Utility::LimitInt(currentDrum + inc, 0, drumCount-1);
    if (newDrum != currentDrum) {
        drums[newDrum]->ResetParams();
        currentDrum = newDrum;
        redraw = true;
    }

    if (hw.encoder.RisingEdge()) {
        currentKnobRow = (currentKnobRow + 1) % 2;
        redraw = true;
        drums[currentDrum]->ResetParams();
    }

    if (redraw) {
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
        drums[currentDrum]->UpdateParam(param, sig);
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
    meter.OnBlockStart();

    ProcessControls();

    for(size_t i = 0; i < size; i++) {

        // Process shared sound sources
        source68.Process();
        clickSource.Process();

        float sig = 0.0f;
        float limited = 0.0f;
        cycle = (cycle + 1) % 8;
        if (cycle < 5) {
        if (CPU_SINGLE) {
            sig = drums[currentDrum]->Process();
            limited = sig;
        } else {
            for (uint8_t i = 0; i < drumCount; i++) {
                sig += drums[i]->Process();
            }
            limited = sig / drumCount;
        }
        }

        out[0][i] = out[1][i] = limited;
        out[2][i] = out[3][i] = sig;
    }

    meter.OnBlockEnd();
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
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    samplerate = hw.AudioSampleRate();

    meter.Init(samplerate, 128, 1.0f);

    // Shared sound sources
    source68.Init("", samplerate, HhSource68::MORPH_808_VALUE);
    clickSource.Init("", samplerate, 1500, 191, 116);

    bd.Init("BD", samplerate);
    rs.Init("RS", samplerate);
    sd.Init("SD", samplerate);
    cp.Init("CP", samplerate);

    lt.Init("LT", samplerate, 80, &clickSource);
    mt.Init("MT", samplerate, 91, &clickSource);
    ht.Init("HT", samplerate, 106, &clickSource);

    ch.Init("CH", samplerate, 0.001, 0.5, &source68, HhSource68::MORPH_808_VALUE, 6000, 16000);
    oh.Init("OH", samplerate, 0.001, 0.13, 0.05, &source68, HhSource68::MORPH_808_VALUE, 6000, 16000);
    cy.Init("CY", samplerate, 0.001, 3.5, &source68, 1700, 2400);
    cb.Init("CB", samplerate, 0.001, 0.5, &source68, 1700, 2400);

    drums[0] = &bd;
    drums[1] = &rs;     // 24
    drums[2] = &sd;     // 32
    drums[3] = &cp;     // 38
    drums[4] = &lt;     // 45
    drums[5] = &mt;     // 52
    drums[6] = &ch;     // 55
    drums[7] = &ht;     // 62
    drums[8] = &oh;     // 65
    drums[9] = &cb;     // 70
    drums[10] = &cy;    // 75
    drumCount = 11;
    currentDrum = 0;

    //display
    hw.display.Fill(false);
    // OledMessage("Hachikit 0.1", 5);
    // Utility::DrawDrums(&hw.display, 5);
    screen.DrawMenu(currentDrum);
    DisplayParamMenu();
    hw.display.Update();


    // Start stuff.
    hw.SetAudioBlockSize(128);
    hw.midi.StartReceive();
    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    for(;;)
    {
        hw.midi.Listen();
        // Handle MIDI Events
        while(hw.midi.HasEvents())
        {
            HandleMidiMessage(hw.midi.PopEvent());
        }
        DisplayKnobValues();

        float avgCpu = meter.GetAvgCpuLoad();
        OledMessage("cpu:" + std::to_string((int)(avgCpu * 100)) + "%", 4);

        hw.display.Update();
    }
}
