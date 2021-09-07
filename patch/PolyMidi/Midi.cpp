#include <list>
#include <vector>

#include "daisy_patch.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

using std::vector;

DaisyPatch hw;
Oscillator osc;
Svf        filt;

struct Voice {
    int note;
    int velocity;
    int assignmentIdx;
    Oscillator osc;
};

template<size_t size>
class PolyOsc {

    public:
        PolyOsc() {}
        ~PolyOsc() {}

        void Init(int samplerate)
        {
            sr_ = samplerate;
            currAss_ = 0;
            for (size_t i = 0; i < size; i++)
            {
                voices[i].assignmentIdx = 0;
                voices[i].note = -1;
                voices[i].velocity = 0;
                voices[i].osc.Init(sr_);
                voices[i].osc.SetWaveform(voices[i].osc.WAVE_SAW);
                voices[i].osc.SetFreq(440);
                voices[i].osc.SetAmp(0);
            }
        }

        // void SetNumVoices(unsigned int num)
        // {
        //     int difference =  (int) num - voices.size();
        //     if (difference > 0)
        //     {
        //         for (int i = 0; i < difference; i++)
        //         {
        //             Voice v;
        //             v.assignmentIdx = 0;
        //             v.note = -1;
        //             v.velocity = 0;
        //             v.osc.Init(sr_);
        //             v.osc.SetWaveform(v.osc.WAVE_SAW);
        //             v.osc.SetAmp(0);
        //             voices.push_back(v);
        //         }
        //     }
        //     else if (difference < 0)
        //     {
        //         while (voices.size() > num)
        //             voices.pop_back();
        //     }
        // }

        bool saturated()
        {
            for (size_t i = 0; i < size; i++)
            {
                if (voices[i].note == -1)
                    return false;
            }
            return true;
        }

        void NoteOn(uint8_t note_num, uint8_t vel)
        {
            if (vel == 0) NoteOff(note_num);
            else
            {
                if (saturated())
                {
                    // TODO -- this could eventually fail if a lot of voices are played
                    int lowest = 1000000;
                    int lowIdx = 0;
                    for (size_t i = 0; i < size; i++)
                    {
                        if (voices[i].assignmentIdx < lowest)
                        {
                            lowest = voices[i].assignmentIdx;
                            lowIdx = i;
                        }
                    }
                    voices[lowIdx].assignmentIdx = ++currAss_;
                    voices[lowIdx].note = note_num;
                    voices[lowIdx].velocity = vel;
                    voices[lowIdx].osc.SetFreq(mtof(note_num));
                    voices[lowIdx].osc.SetAmp(((float) vel / 127.0f));
                }
                else
                {
                    for (size_t i = 0; i < size; i++)
                    {
                        if (voices[i].note == -1)
                        {
                            voices[i].assignmentIdx = ++currAss_;
                            voices[i].note = note_num;
                            voices[i].velocity = vel;
                            voices[i].osc.SetFreq(mtof(note_num));
                            voices[i].osc.SetAmp(((float) vel / 127.0f));
                            break;
                        }
                    }
                }
            }
        }

        void NoteOff(uint8_t note_num)
        {
            for (size_t i = 0; i < size; i++)
            {
                if (voices[i].note == note_num)
                {
                    voices[i].note = -1;
                    voices[i].velocity = 0;
                    voices[i].osc.SetAmp(0);
                }
            }
        }

        void SetWave(int wave)
        {
            for (size_t i = 0; i < size; i++)
            {
                voices[i].osc.SetWaveform((const uint8_t) wave);
            }
        }

        float Process()
        {
            float accum = 0;
            for (size_t i = 0; i < size; i++)
            {
                accum += voices[i].osc.Process();
            }
            return accum;
        }

    private:
        int currAss_;
        int sr_;
        Voice voices[size];
};

PolyOsc<16> poly;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    hw.ProcessAnalogControls();
    int wave = hw.controls[0].GetRawFloat() * 4;

    switch (wave) {
        case 0:
            // osc.SetWaveform(osc.WAVE_SIN);
            poly.SetWave(Oscillator::WAVE_SIN);
            break;
        case 1:
            poly.SetWave(Oscillator::WAVE_TRI);
            break;
        case 2:
            poly.SetWave(Oscillator::WAVE_SQUARE);
            break;
        default:
        case 3:
            poly.SetWave(Oscillator::WAVE_RAMP);
            break;
    }

    float sig;
    for(size_t i = 0; i < size; i++)
    {
        sig = poly.Process();
        sig *= 0.25;
        // filt.Process(sig);
        for(size_t chn = 0; chn < 4; chn++)
        {
            out[chn][i] = sig;
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
            poly.NoteOn(p.note, p.velocity);
        }
        break;
        case NoteOff:
        {
                NoteOffEvent p = m.AsNoteOff();
                poly.NoteOff(p.note);
        }
        break;
        case ControlChange:
        {
            ControlChangeEvent p = m.AsControlChange();
            switch(p.control_number)
            {
                case 1:
                    // CC 1 for cutoff.
                    filt.SetFreq(mtof((float)p.value));
                    break;
                case 2:
                    // CC 2 for res.
                    filt.SetRes(((float)p.value / 127.0f));
                    break;
                default: break;
            }
            break;
        }
        case PitchBend:
        {

        }
        break;
        default: break;
    }
}

int main(void)
{
	// Init
    float samplerate;
    hw.Init();
    samplerate = hw.AudioSampleRate();

    // Synthesis
    poly.Init(samplerate);

    hw.display.Fill(false);
    hw.display.Update();

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
    }
}