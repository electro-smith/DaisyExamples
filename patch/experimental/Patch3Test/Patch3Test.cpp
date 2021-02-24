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

void AudioCallback(float **in, float **out, size_t size)
{
    float sig;
    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();
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
    if(hw.encoder.FallingEdge()
       || hw.gate_input[DaisyPatch::GateInput::GATE_IN_1].Trig())
    {
        waveidx = 0;
        osc.SetWaveform(waveforms[waveidx]);
    }
    if(hw.gate_input[DaisyPatch::GateInput::GATE_IN_2].Trig())
    {
        waveidx = 3;
        osc.SetWaveform(waveforms[waveidx]);
    }
    for(size_t i = 0; i < size; i++)
    {
        sig = osc.Process();
        filt.Process(sig);
        out[0][i] = filt.Low();
        out[1][i] = filt.High();
        out[2][i] = filt.Band();
        out[3][i] = filt.Notch();
    }
    dsy_gpio_toggle(&hw.gate_output);
    hw.seed.dac.WriteValue(DacHandle::Channel::ONE,
                           (hw.GetKnobValue(DaisyPatch::Ctrl::CTRL_1) + 1.f)
                               * 4095.0f);
    hw.seed.dac.WriteValue(DacHandle::Channel::TWO,
                           (hw.GetKnobValue(DaisyPatch::Ctrl::CTRL_2) + 1.f)
                               * 4095.0f);
}

void BypassTest(float **in, float **out, size_t size)
{
    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();
    for(size_t chn = 0; chn < 4; chn++)
    {
        for(size_t i = 0; i < size; i += 2)
        {
            out[chn][i] = in[chn][i];
        }
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
    float samplerate;
    // Init
    hw.Init();
    samplerate = hw.AudioSampleRate();

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
        hw.DisplayControls();
    }
}
