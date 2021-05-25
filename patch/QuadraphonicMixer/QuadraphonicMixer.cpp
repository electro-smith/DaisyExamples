#include "daisysp.h"
#include "daisy_patch.h"
#include <string>
#include <limits>
#include <vector>
#include "quadMixer.h"
#include "stereoMixer.h"

using namespace daisy;
using namespace daisysp;
using namespace QuadraphonicMixer;

DaisyPatch        patch;
float             sampleRate;
constexpr uint8_t NUM_CTRL = 4;
float             ctrl[4];
constexpr uint8_t NUM_GATE = 2;
bool              gate[NUM_GATE];
constexpr uint8_t NUM_CHANNEL      = 4;
constexpr float   ONE_ONEHUNDREDTH = 0.01f;

enum State : uint8_t
{
    ParamSelect    = 0,
    Stereo         = 1,
    ReverbWet      = 2,
    ReverbFeedback = 3,
    CH1Gain        = 4,
    CH2Gain        = 5,
    CH3Gain        = 6,
    CH4Gain        = 7
};
constexpr uint8_t NUM_STATES = 8;
constexpr uint8_t NUM_PARAM  = NUM_STATES - 1;
State             currState  = State::ParamSelect;
uint8_t           currParam  = 0;

bool        stereo = true;
StereoMixer stereoMixer;
QuadMixer   quadMixer;

constexpr uint8_t      NUM_VERB = 2;
ReverbSc DSY_SDRAM_BSS verb[NUM_VERB];
float                  wet             = 0.3f;
float                  feedback        = 0.3f;
constexpr float        REVERB_LP_FREQ  = 18000.0f;
constexpr float        DRY_MINIMUM_AMP = 0.3f;

constexpr float    MIN_GAIN = 0.f;
constexpr float    MAX_GAIN = 2.f;
std::vector<float> gain{1.0f, 1.f, 1.f, 1.f};

inline float Clamp(const float value, const float min, const float max)
{
    if(value < min)
    {
        return min;
    }

    if(value > max)
    {
        return max;
    }

    return value;
}

void UpdateParamState()
{
    if(patch.encoder.FallingEdge())
    {
        switch(currState)
        {
            case State::ParamSelect:
                currState = static_cast<State>(currParam + 1);
                break;
            default: currState = State::ParamSelect; break;
        }
    }

    const int32_t currInc = patch.encoder.Increment();
    if(abs(currInc) > 0)
    {
        switch(currState)
        {
            case State::ParamSelect:
            {
                int32_t newValue = currParam + currInc;
                newValue += newValue < 0 ? NUM_PARAM : 0;
                currParam = newValue % NUM_PARAM;
            }
            break;
            case State::Stereo:
            {
                if(abs(currInc) > 0)
                {
                    stereo = !stereo;
                }
            }
            break;
            case State::ReverbWet:
            {
                wet += ONE_ONEHUNDREDTH * currInc;
                wet = Clamp(wet, 0.f, 1.f);
            }
            break;
            case State::ReverbFeedback:
            {
                feedback += ONE_ONEHUNDREDTH * currInc;
                feedback = Clamp(feedback, 0.f, 1.3f);
            }
            break;
            case State::CH1Gain:
            {
                const uint8_t gainIndex = 0;
                gain[gainIndex] += ONE_ONEHUNDREDTH * currInc;
                gain[gainIndex] = Clamp(gain[gainIndex], MIN_GAIN, MAX_GAIN);
            }
            break;
            case State::CH2Gain:
            {
                const uint8_t gainIndex = 1;
                gain[gainIndex] += ONE_ONEHUNDREDTH * currInc;
                gain[gainIndex] = Clamp(gain[gainIndex], MIN_GAIN, MAX_GAIN);
            }
            break;
            case State::CH3Gain:
            {
                const uint8_t gainIndex = 2;
                gain[gainIndex] += ONE_ONEHUNDREDTH * currInc;
                gain[gainIndex] = Clamp(gain[gainIndex], MIN_GAIN, MAX_GAIN);
            }
            break;
            case State::CH4Gain:
            {
                const uint8_t gainIndex = 3;
                gain[gainIndex] += ONE_ONEHUNDREDTH * currInc;
                gain[gainIndex] = Clamp(gain[gainIndex], MIN_GAIN, MAX_GAIN);
            }
            break;
        }
    }
}

void UpdateControls()
{
    patch.ProcessAnalogControls();
    patch.ProcessDigitalControls();

    UpdateParamState();

    for(uint8_t n = 0; n < NUM_CTRL; n++)
    {
        ctrl[n] = patch.GetKnobValue(static_cast<DaisyPatch::Ctrl>(n));
        stereoMixer.SetPan(n, ctrl[n]);
        quadMixer.SetAngle(n, ctrl[n]);
    }

    for(uint8_t n = 0; n < NUM_GATE; n++)
    {
        gate[n] = patch.gate_input[n].Trig();
    }

    for(uint8_t n = 0; n < NUM_VERB; n++)
    {
        verb[n].SetFeedback(feedback);
    }
}

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    UpdateControls();

    float mixedValues[4];
    float verbValues[4];
    for(size_t n = 0; n < size; n++)
    {
        if(stereo)
        {
            const StereoMixer::Input inStereo{gain[0] * in[0][n],
                                              gain[1] * in[1][n],
                                              gain[2] * in[2][n],
                                              gain[3] * in[3][n]};

            StereoMixer::Output outStereo{0, 0};
            stereoMixer.Process(inStereo, outStereo);

            mixedValues[0] = outStereo.Values[0];
            mixedValues[1] = outStereo.Values[1];
            mixedValues[2] = 0.f;
            mixedValues[3] = 0.f;
        }
        else
        {
            const QuadMixer::Data inQuad{gain[0] * in[0][n],
                                         gain[1] * in[1][n],
                                         gain[2] * in[2][n],
                                         gain[3] * in[3][n]};

            QuadMixer::Data outQuad{0, 0, 0, 0};
            quadMixer.Process(inQuad, outQuad);

            mixedValues[0] = outQuad.Values[0];
            mixedValues[1] = outQuad.Values[1];
            mixedValues[2] = outQuad.Values[2];
            mixedValues[3] = outQuad.Values[3];
        }

        verb[0].Process(
            mixedValues[0], mixedValues[1], &verbValues[0], &verbValues[1]);
        verb[1].Process(
            mixedValues[2], mixedValues[3], &verbValues[2], &verbValues[3]);

        for(uint8_t m = 0; m < NUM_CHANNEL; m++)
        {
            out[m][n] = wet * verbValues[m]
                        + (DRY_MINIMUM_AMP + 1.f - wet) * mixedValues[m];
        }
    }
}

void UpdateDisplay()
{
    uint32_t minX       = 0;
    uint32_t minY       = 0;
    uint32_t xOffset    = 9 * 7;
    uint32_t lineOffset = 8;

    patch.display.Fill(false);

    bool highlightState[NUM_STATES]{false};
    highlightState[static_cast<uint8_t>(currState)] = true;
    if(highlightState[static_cast<uint8_t>(State::ParamSelect)])
    {
        highlightState[1 + currParam] = true;
    }

    std::string str  = "QUADRAPHONIC MIXER";
    char*       cstr = &str[0];
    patch.display.SetCursor(minX, minY);
    patch.display.WriteString(cstr, Font_6x8, true);

    str = stereo ? "STEREO" : "QUAD";
    patch.display.SetCursor(minX, minY + lineOffset);
    patch.display.WriteString(
        cstr, Font_6x8, !highlightState[static_cast<uint8_t>(State::Stereo)]);

    str = "VERB:" + std::to_string(static_cast<uint32_t>(100 * wet));
    patch.display.SetCursor(minX, minY + 2 * lineOffset);
    patch.display.WriteString(
        cstr,
        Font_6x8,
        !highlightState[static_cast<uint8_t>(State::ReverbWet)]);

    str = "FDBK:" + std::to_string(static_cast<uint32_t>(100 * feedback));
    patch.display.SetCursor(minX, minY + 3 * lineOffset);
    patch.display.WriteString(
        cstr,
        Font_6x8,
        !highlightState[static_cast<uint8_t>(State::ReverbFeedback)]);

    str = "CH1:" + std::to_string(static_cast<uint32_t>(100 * ctrl[0]));
    patch.display.SetCursor(minX, minY + 4 * lineOffset);
    patch.display.WriteString(cstr, Font_6x8, true);

    str = "GAIN:" + std::to_string(static_cast<uint32_t>(100 * gain[0]));
    patch.display.SetCursor(minX + xOffset, minY + 4 * lineOffset);
    patch.display.WriteString(
        cstr, Font_6x8, !highlightState[static_cast<uint8_t>(State::CH1Gain)]);

    str = "CH2:" + std::to_string(static_cast<uint32_t>(100 * ctrl[1]));
    patch.display.SetCursor(minX, minY + 5 * lineOffset);
    patch.display.WriteString(cstr, Font_6x8, true);

    str = "GAIN:" + std::to_string(static_cast<uint32_t>(100 * gain[1]));
    patch.display.SetCursor(minX + xOffset, minY + 5 * lineOffset);
    patch.display.WriteString(
        cstr, Font_6x8, !highlightState[static_cast<uint8_t>(State::CH2Gain)]);

    str = "CH3:" + std::to_string(static_cast<uint32_t>(100 * ctrl[2]));
    patch.display.SetCursor(minX, minY + 6 * lineOffset);
    patch.display.WriteString(cstr, Font_6x8, true);

    str = "GAIN:" + std::to_string(static_cast<uint32_t>(100 * gain[2]));
    patch.display.SetCursor(minX + xOffset, minY + 6 * lineOffset);
    patch.display.WriteString(
        cstr, Font_6x8, !highlightState[static_cast<uint8_t>(State::CH3Gain)]);

    str = "CH4:" + std::to_string(static_cast<uint32_t>(100 * ctrl[3]));
    patch.display.SetCursor(minX, minY + 7 * lineOffset);
    patch.display.WriteString(cstr, Font_6x8, true);

    str = "GAIN:" + std::to_string(static_cast<uint32_t>(100 * gain[3]));
    patch.display.SetCursor(minX + xOffset, minY + 7 * lineOffset);
    patch.display.WriteString(
        cstr, Font_6x8, !highlightState[static_cast<uint8_t>(State::CH4Gain)]);

    patch.display.Update();
}

int main(void)
{
    patch.Init();
    sampleRate = patch.AudioSampleRate();

    for(uint8_t n = 0; n < 2; n++)
    {
        verb[n].Init(sampleRate);
        verb[n].SetFeedback(feedback);
        verb[n].SetLpFreq(REVERB_LP_FREQ);
    }

    patch.StartAdc();
    patch.StartAudio(AudioCallback);

    while(1)
    {
        UpdateDisplay();
    }
}