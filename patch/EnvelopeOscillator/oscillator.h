#pragma once
#include <stdint.h>
#ifdef __cplusplus
namespace EnvelopeOscillator
{
namespace
{
    constexpr float C1_FREQ = 32.70f;
    constexpr float TWO_PI  = 6.28318530718f;
} // namespace

struct Oscillator
{
    void Init(float sampleRate) { _secondsPerSample = 1.f / sampleRate; }

    inline void SetFreq(float freq) { _freq = freq; }

    inline float GetFreq() { return _freq; }

    inline void SetMorph(float value) { _morph = value; }

    inline float GetMorph() { return _morph; }

    inline float Process()
    {
        const float value = CalcWave(_morph, _pos);
        _pos += _freq * _secondsPerSample;
        if(_pos >= 1.f)
        {
            _pos = 0.f;
        }

        return value;
    }

  private:
    static inline float CalcWave(float morph, float position)
    {
        return (1.f - morph) * CalcSine(position)
               + morph * CalcSquare(position);
    }

    static inline float CalcSine(float position)
    {
        return sinf(position * TWO_PI);
    }

    static inline float CalcSquare(float position)
    {
        return position < 0.5f ? 1.f : -1.f;
    }

    float _secondsPerSample{1.f / 44100.f};
    float _freq{C1_FREQ};
    float _pos{0.f};
    float _morph{0.f};
};
} // namespace EnvelopeOscillator
#endif