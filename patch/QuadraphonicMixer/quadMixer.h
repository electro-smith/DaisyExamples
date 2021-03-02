#pragma once
#include <stdint.h>
#ifdef __cplusplus

namespace QuadraphonicMixer
{
namespace
{
    constexpr uint8_t QUAD_MIXER_NUM_CHANNEL = 4;
    constexpr float   ANGLE_REGION_SIZE      = 1.4f;
    constexpr float   TWO_PI                 = 6.28318531f;
} // namespace

struct QuadMixer
{
    struct Data
    {
        float Values[QUAD_MIXER_NUM_CHANNEL];
    };

    QuadMixer() {}

    void Process(const Data& in, Data& out)
    {
        for(uint8_t n = 0; n < QUAD_MIXER_NUM_CHANNEL; n++)
        {
            out.Values[n] = ProcessChannel(in.Values[0], 0, n)
                            + ProcessChannel(in.Values[1], 1, n)
                            + ProcessChannel(in.Values[2], 2, n)
                            + ProcessChannel(in.Values[3], 3, n);
        }
    }

    void SetAngle(const uint8_t index, const float angle)
    {
        _angle[index] = TWO_PI * angle;

        const float x1 = cosf(_angle[index]);
        const float y1 = sinf(_angle[index]);
        for(uint8_t n = 0; n < QUAD_MIXER_NUM_CHANNEL; n++)
        {
            const float x2   = cosf(_regionAngles[n]);
            const float y2   = sinf(_regionAngles[n]);
            const float dX   = abs(x1 - x2);
            const float dY   = abs(y1 - y2);
            const float dist = sqrt(dX * dX + dY * dY);

            if(dist < ANGLE_REGION_SIZE)
            {
                _amp[index][n] = 1.f - (dist / ANGLE_REGION_SIZE);
            }
            else
            {
                _amp[index][n] = 0.f;
            }
        }
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

    float _angle[QUAD_MIXER_NUM_CHANNEL]{0.f, 0.f, 0.f, 0.f};
    float _regionAngles[QUAD_MIXER_NUM_CHANNEL]{0.f,
                                                TWO_PI * 0.25f,
                                                TWO_PI * 0.5f,
                                                TWO_PI * 0.75f};
    float _amp[QUAD_MIXER_NUM_CHANNEL][QUAD_MIXER_NUM_CHANNEL]{};
};
} // namespace QuadraphonicMixer
#endif