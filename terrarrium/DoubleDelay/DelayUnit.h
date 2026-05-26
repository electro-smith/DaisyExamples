// Version 1.0.4
#pragma once

#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

constexpr size_t MAX_DELAY = 48000 * 5; // 5 seconds max delay

inline float LocalSoftClip(float x)
{
    // return x / (1.0f + fabsf(x));

    // or 

    if(x > 1.0f)
        return (2.0f / 3.0f);
    else if(x < -1.0f)
        return (-2.0f / 3.0f);
    else
        return x - (x * x * x) / 3.0f;

}

struct DelayUnit {
    DelayLine<float, MAX_DELAY>* buffer;
    float currentDelay = 0.5f;
    float delayTarget = 0.5f;
    float feedback = 0.5f;
    Tone lp_filter;
    Svf hpf;

    void Init(float samplerate, DelayLine<float, MAX_DELAY>& buf)
    {
        buffer = &buf;
        buffer->Init();
        lp_filter.Init(samplerate);
        lp_filter.SetFreq(4000.0f);
        hpf.Init(samplerate);
        hpf.SetRes(0.7f);
        hpf.SetFreq(120.0f); // Cut off sub-bass
        hpf.SetDrive(0.0f);
    }

    float SnapToRatio(float base_delay_time, float ratio_knob)
    {
        const float ratios[] = {
            1.0f, 15.0f / 16.0f, 7.0f / 8.0f, 13.0f / 16.0f, 3.0f / 4.0f,
            11.0f / 16.0f, 5.0f / 8.0f, 9.0f / 16.0f, 1.0f / 2.0f,
            7.0f / 16.0f, 3.0f / 8.0f, 5.0f / 16.0f, 1.0f / 4.0f,
            3.0f / 16.0f, 1.0f / 8.0f, 1.0f / 16.0f
        };
        const int num_ratios = sizeof(ratios) / sizeof(ratios[0]);
        int index = static_cast<int>(ratio_knob * num_ratios);
        if(index >= num_ratios)
            index = num_ratios - 1;

        float delay_time = base_delay_time * ratios[index];
        return fminf(delay_time, MAX_DELAY);
    }

    float Process(float in, float delay_time)
    {
        delayTarget = delay_time;
        fonepole(currentDelay, delayTarget, 0.0002f);
        buffer->SetDelay(currentDelay);

        float delayed = buffer->Read();
        float input = (in + delayed * feedback) * 0.7f;
        input = LocalSoftClip(input);
        float filtered = lp_filter.Process(input);
        filtered = fminf(fmaxf(filtered, -1.0f), 1.0f);
        buffer->Write(filtered);

        return delayed;
    }
};
