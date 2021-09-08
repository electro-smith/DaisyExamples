#pragma once
#include <stdint.h>
#ifdef __cplusplus

namespace QuadraphonicMixer
{
namespace
{
    constexpr uint8_t STEREO_MIXER_NUM_CHANNEL = 4;
    constexpr uint8_t STEREO_MIXER_NUM_OUTPUT  = 2;
} // namespace

struct StereoMixer
{
    struct Input
    {
        float Values[STEREO_MIXER_NUM_CHANNEL];
    };

    struct Output
    {
        float Values[STEREO_MIXER_NUM_OUTPUT];
    };

    StereoMixer() {}

    void Process(const Input& in, Output& out)
    {
        for(uint8_t n = 0; n < STEREO_MIXER_NUM_OUTPUT; n++)
        {
            out.Values[n] = ProcessChannel(in.Values[0], 0, n)
                            + ProcessChannel(in.Values[1], 1, n)
                            + ProcessChannel(in.Values[2], 2, n)
                            + ProcessChannel(in.Values[3], 3, n);
        }
    }

    inline void SetPan(const uint8_t index, const float value)
    {
        _amp[index][0] = 1.f - value;
        _amp[index][1] = value;
    }

    inline float GetAngle(const uint8_t index) const { return _angle[index]; }

    inline float GetAmp(const uint8_t channelIndex,
                        const uint8_t outIndex) const
    {
        return _amp[channelIndex][outIndex];
    }

  private:
    inline float ProcessChannel(const float   value,
                                const uint8_t channelIndex,
                                const uint8_t outIndex)
    {
        return _amp[channelIndex][outIndex] * value;
    }

    float _angle[STEREO_MIXER_NUM_CHANNEL]{0.f, 0.f, 0.f, 0.f};
    float _amp[STEREO_MIXER_NUM_CHANNEL][STEREO_MIXER_NUM_OUTPUT]{0.f,
                                                                  0.f,
                                                                  0.f,
                                                                  0.f};
};
} // namespace QuadraphonicMixer
#endif