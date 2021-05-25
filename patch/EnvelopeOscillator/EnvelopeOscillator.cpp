#include "daisysp.h"
#include "daisy_patch.h"
#include "envelope.h"
#include "oscillator.h"

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;
float      sampleRate;

constexpr uint8_t NUM_CV_IN   = 4;
constexpr uint8_t NUM_GATE_IN = 2;

constexpr float C1_FREQ = 32.70f;

float ctrl[NUM_CV_IN];
bool  gate[NUM_GATE_IN];

EnvelopeOscillator::Envelope   env;
EnvelopeOscillator::Oscillator osc;

inline float static CalcFreq(const float value)
{
    return powf(2.f, value * 5.f) * C1_FREQ; //Hz
}

void UpdateControls()
{
    patch.ProcessAnalogControls();
    patch.ProcessDigitalControls();

    for(uint8_t n = 0; n < NUM_CV_IN; n++)
    {
        ctrl[n] = patch.GetKnobValue(static_cast<DaisyPatch::Ctrl>(n));
    }

    for(uint8_t n = 0; n < NUM_GATE_IN; n++)
    {
        gate[n] = patch.gate_input[n].Trig();
    }

    osc.SetFreq(CalcFreq(ctrl[0]));
    osc.SetMorph(ctrl[1]);
    env.SetRise(ctrl[2]);
    env.SetFall(ctrl[3]);

    if(gate[0] || patch.encoder.FallingEdge())
    {
        env.Trigger();
    }
}

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    UpdateControls();

    for(size_t n = 0; n < size; n++)
    {
        out[0][n] = env.Process() * osc.Process();
        out[1][n] = 0.f;
        out[2][n] = 0.f;
        out[3][n] = 0.f;
    }

    patch.seed.dac.WriteValue(DacHandle::Channel::BOTH,
                              static_cast<uint16_t>(4095 * env.GetCurrValue()));
}

void UpdateDisplay()
{
    uint32_t minX       = 0;
    uint32_t minY       = 0;
    uint32_t lineOffset = 8;

    patch.display.Fill(false);

    std::string str  = "ENVELOPE OSCILLATOR";
    char*       cstr = &str[0];
    patch.display.SetCursor(minX, minY);
    patch.display.WriteString(cstr, Font_6x8, true);

    str = "FREQ:" + std::to_string(static_cast<uint32_t>(osc.GetFreq()));
    patch.display.SetCursor(minX, lineOffset + minY);
    patch.display.WriteString(cstr, Font_6x8, true);

    str = "MRPH:" + std::to_string(static_cast<uint8_t>(100 * osc.GetMorph()));
    patch.display.SetCursor(minX, 2 * lineOffset + minY);
    patch.display.WriteString(cstr, Font_6x8, true);

    str = "RISE:" + std::to_string(static_cast<uint32_t>(1000 * env.GetRise()));
    patch.display.SetCursor(minX, 3 * lineOffset + minY);
    patch.display.WriteString(cstr, Font_6x8, true);

    str = "FALL:" + std::to_string(static_cast<uint32_t>(1000 * env.GetFall()));
    patch.display.SetCursor(minX, 4 * lineOffset + minY);
    patch.display.WriteString(cstr, Font_6x8, true);

    str = "ENV:"
          + std::to_string(static_cast<uint8_t>(100 * env.GetCurrValue()));
    patch.display.SetCursor(minX, 5 * lineOffset + minY);
    patch.display.WriteString(cstr, Font_6x8, true);

    patch.display.Update();
}

int main(void)
{
    patch.Init();
    sampleRate = patch.AudioSampleRate();

    env.Init(sampleRate);
    osc.Init(sampleRate);

    patch.StartAdc();
    patch.StartAudio(AudioCallback);

    while(1)
    {
        UpdateDisplay();
    }
}
