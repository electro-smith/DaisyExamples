#pragma once
#include <stdint.h>
#include "utils.h"
#include <vector>
#ifdef __cplusplus
namespace EnvelopeOscillator
{
struct Envelope
{
    void Init(float sampleRate) { _secondsPerSample = 1.f / sampleRate; }

    void Trigger()
    {
        _state = State::Rising;
        if(_value > 0.f)
        {
            _dt = _value * _riseLength;
        }
        else
        {
            _dt = 0.f;
        }
    }

    void SetRise(const float value)
    {
        // value expected to be from 0.f to 1.f
        _riseLength = Utils::GetValue(riseFallLengths, value);
    }

    inline float GetRise() { return _riseLength; }

    void SetFall(const float value)
    {
        // value expected to be from 0.f to 1.f
        _fallLength = Utils::GetValue(riseFallLengths, value);
    }

    inline float GetFall() { return _fallLength; }

    inline float Process()
    {
        switch(_state)
        {
            case State::Rising:
                _dt += _secondsPerSample;
                _value = _dt / _riseLength;
                if(_dt >= _riseLength)
                {
                    _state = State::Falling;
                    _dt    = 0.f;
                }
                break;
            case State::Falling:
                _dt += _secondsPerSample;
                _value = 1.f - (_dt / _fallLength);
                if(_dt >= _fallLength)
                {
                    _state = State::None;
                    _dt    = 0.f;
                }
                break;
            case State::None:
            {
                _value = 0.f;
            }
            break;
        }

        return _value;
    }

    inline float GetCurrValue() { return _value; }

  private:
    enum State
    {
        Rising,
        Falling,
        None
    };
    State _state{State::None};
    float _secondsPerSample{1.f / 44100.f};
    float _dt{0.f};
    float _riseLength{0.1f};
    float _fallLength{0.1f};
    float _value{0.f};
    const std::vector<float>
        riseFallLengths{0.05f, 0.1f, 0.2f, 0.3f, 0.9f, 1.3f, 3.f};
};
} // namespace EnvelopeOscillator
#endif