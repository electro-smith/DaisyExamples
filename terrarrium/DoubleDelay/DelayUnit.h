#pragma once

#include "daisysp.h"
#include "ExtendedDelayLine.h"  // Include our wrapper

using namespace daisy;
using namespace daisysp;

constexpr size_t MAX_DELAY = 48000 * 5; // 5 seconds max delay

struct DelayUnit {
    ExtendedDelayLine<float, MAX_DELAY>* buffer;  // Use ExtendedDelayLine now
    float currentDelay = 0.5f;
    float delayTarget  = 0.5f;
    float feedback     = 0.5f;
    Tone lp_filter;
    bool reverse       = false;  // Flag for reverse playback

    void Init(float samplerate, ExtendedDelayLine<float, MAX_DELAY>& buf)
    {
        buffer = &buf;
        buffer->Reset();
        lp_filter.Init(samplerate);
        lp_filter.SetFreq(4000.0f);
    }

    float Process(float in, float /*delay_samples*/)
    {
        // Smooth delay time changes.
        fonepole(currentDelay, delayTarget, 0.0002f);
        buffer->SetDelay(currentDelay);

        // Choose forward or reverse read.
        float delayed = reverse ? buffer->ReverseRead() : buffer->Read();

        // Mix input with delayed signal using feedback.
        float input = in + delayed * feedback;
        float filtered = lp_filter.Process(input);
        filtered = fminf(fmaxf(filtered, -1.0f), 1.0f);
        buffer->Write(filtered);

        return delayed;
    }
};
