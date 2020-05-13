#include <stdio.h>
#include "daisy_patch.h"
#include "daisysp.h"

using namespace daisy;

DaisyPatch          hw;
daisysp::Oscillator osc;
daisysp::Svf        filt;

uint8_t waveforms[4] = {
    daisysp::Oscillator::WAVE_SIN,
    daisysp::Oscillator::WAVE_TRI,
    daisysp::Oscillator::WAVE_POLYBLEP_SAW,
    daisysp::Oscillator::WAVE_POLYBLEP_SQUARE,
};

uint8_t waveidx = 0;

void AudioCallback(float *in, float *out, size_t size)
{
    float sig;
    hw.UpdateAnalogControls();
    hw.DebounceControls();
    int32_t inc = hw.encoder.Increment();
    if(inc > 0)
    {
        waveidx = (waveidx + 1) % 4;
        osc.SetWaveform(waveforms[waveidx]);
    }
    else if(inc < 0)
    {
        waveidx = (waveidx + 4 - 1) % 4;
        osc.SetWaveform(waveforms[waveidx]);
    }
    for(size_t i = 0; i < size; i += 2)
    {
        sig = osc.Process();
        filt.Process(sig);
        out[i] = out[i + 1] = filt.Low();
    }
}

// Typical Switch case for Message Type.
void HandleMidiMessage(MidiEvent m)
{
    switch(m.type)
    {
        case NoteOn:
        {
            NoteOnEvent p = m.AsNoteOn();
            // This is to avoid Max/MSP Note outs for now..
            if(m.data[1] != 0)
            {
                p = m.AsNoteOn();
                osc.SetFreq(daisysp::mtof(p.note));
                osc.SetAmp((p.velocity / 127.0f) * 0.5f);
            }
        }
        break;
        case ControlChange:
        {
            ControlChangeEvent p = m.AsControlChange();
            switch(p.control_number)
            {
                case 1:
                    // CC 1 for cutoff.
                    filt.SetFreq(daisysp::mtof((float)p.value));
                    break;
                case 2:
                    // CC 2 for res.
                    filt.SetRes(((float)p.value / 127.0f));
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
    uint32_t screen_update_period, screen_update_last;
    float    samplerate;
    // Init
    hw.Init();
    samplerate           = hw.AudioSampleRate();
    screen_update_period = 17; // roughly 60Hz
    screen_update_last   = dsy_system_getnow();

    // Synthesis
    osc.Init(samplerate);
    osc.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SAW);
    osc.SetAmp(0.4f);
    filt.Init(samplerate);

    // Display Test
    hw.display.Fill(true);
    char displaybuff[32];
    sprintf(displaybuff, "Daisy");
    hw.display.SetCursor(15, (SSD1309_HEIGHT - Font_16x26.FontHeight) / 2);
    hw.display.WriteString(displaybuff, Font_16x26, false);
    hw.display.Update();
    // Start stuff.
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
        if(dsy_system_getnow() - screen_update_last > screen_update_period)
        {
            // Graph Knobs
            size_t barwidth, barspacing, barheight;
            size_t curx, cury;
            barwidth   = 15;
            barspacing = 20;
            hw.display.Fill(false);
            // Bars for all four knobs.
            for(size_t i = 0; i < DaisyPatch::CTRL_LAST; i++)
            {
                float  v;
                size_t dest;
                curx = (barspacing * i + 1) + (barwidth * i);
                cury = SSD1309_HEIGHT;
				v    = hw.GetCtrlValue(static_cast<DaisyPatch::Ctrl>(i)) + 1.0f;
				dest = (v * SSD1309_HEIGHT);
                for(size_t j = dest; j > 0; j--)
                {
                    for(size_t k = 0; k < barwidth; k++)
                    {
                        hw.display.DrawPixel(curx + k, cury - j, true);
                    }
                }
            }
            hw.display.Update();
        }
    }
}
