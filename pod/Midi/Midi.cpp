#include "daisy_pod.h"
#include "daisysp.h"
#include <stdio.h>
#include <stddef.h>
#include <string.h>

namespace dsp = daisysp;

daisy::DaisyPod    hw;
daisy::MidiHandler midi;
dsp::Oscillator    osc;
dsp::Svf           filt;

void AudioCallback(daisy::AudioHandle::InterleavingInputBuffer  in,
                   daisy::AudioHandle::InterleavingOutputBuffer out,
                   size_t                                       size)
{
    float sig;
    for(size_t i = 0; i < size; i += 2)
    {
        sig = osc.Process();
        filt.Process(sig);
        out[i] = out[i + 1] = filt.Low();
    }
}

// Typical Switch case for Message Type.
void HandleMidiMessage(daisy::MidiEvent m)
{
    switch(m.type)
    {
        case daisy::NoteOn:
        {
            daisy::NoteOnEvent p = m.AsNoteOn();
            char               buff[512];
            sprintf(buff,
                    "Note Received:\t%d\t%d\t%d\r\n",
                    m.channel,
                    m.data[0],
                    m.data[1]);
            hw.seed.usb_handle.TransmitInternal((uint8_t *)buff, strlen(buff));
            // This is to avoid Max/MSP Note outs for now..
            if(m.data[1] != 0)
            {
                p = m.AsNoteOn();
                osc.SetFreq(dsp::mtof(p.note));
                osc.SetAmp((p.velocity / 127.0f));
            }
        }
        break;
        case daisy::ControlChange:
        {
            daisy::ControlChangeEvent p = m.AsControlChange();
            switch(p.control_number)
            {
                case 1:
                    // CC 1 for cutoff.
                    filt.SetFreq(dsp::mtof((float)p.value));
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
    // Init
    float samplerate;
    hw.Init();
    midi.Init(daisy::MidiHandler::MidiInputMode::INPUT_MODE_UART1,
              daisy::MidiHandler::MidiOutputMode::OUTPUT_MODE_UART1);
    hw.seed.usb_handle.Init(daisy::UsbHandle::FS_INTERNAL);
    daisy::System::Delay(250);

    // Synthesis
    samplerate = hw.AudioSampleRate();
    osc.Init(samplerate);
    osc.SetWaveform(dsp::Oscillator::WAVE_POLYBLEP_SAW);
    filt.Init(samplerate);

    // Start stuff.
    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    midi.StartReceive();
    for(;;)
    {
        midi.Listen();
        // Handle MIDI Events
        while(midi.HasEvents())
        {
            HandleMidiMessage(midi.PopEvent());
        }
    }
}