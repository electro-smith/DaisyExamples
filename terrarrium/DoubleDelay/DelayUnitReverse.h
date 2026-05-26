#pragma once

#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

constexpr size_t MAX_DELAY = 48000 * 5; // 5 seconds max delay

struct DelayUnit {
    DelayLine<float, MAX_DELAY>* buffer;
    float currentDelay = 0.5f;
    float delayTarget = 0.5f;
    float feedback = 0.5f;
    Tone lp_filter;

    void Init(float samplerate, DelayLine<float, MAX_DELAY>& buf)
    {
        buffer = &buf;
        buffer->Init();
        lp_filter.Init(samplerate);
        lp_filter.SetFreq(4000.0f);
    }

    float Process(float in, float delay_time)
{
    delayTarget = delay_time;

    fonepole(currentDelay, delayTarget, 0.0002f);
    buffer->SetDelay(currentDelay);

    float delayed = buffer->Read();
    float input = in + delayed * feedback;
    float filtered = lp_filter.Process(input);
    filtered = fminf(fmaxf(filtered, -1.0f), 1.0f);
    buffer->Write(filtered);

    return delayed;
}

    float Process(float in, float delay1_time, float ratio_knob)
    {
        float max_ratio = MAX_DELAY / delay1_time;
        float log_ratio = powf(2.0f, (ratio_knob - 0.5f) * 4.0f); // ~0.25x–4x
        float ratio = fminf(log_ratio, max_ratio);
        delayTarget = delay1_time * ratio;

        fonepole(currentDelay, delayTarget, 0.0002f);
        buffer->SetDelay(currentDelay);

        float delayed = buffer->Read();
        float input = in + delayed * feedback;
        float filtered = lp_filter.Process(input);
        filtered = fminf(fmaxf(filtered, -1.0f), 1.0f);
        buffer->Write(filtered);

        return delayed;
    }
};



// #pragma once

// #include "delayline_reverse.h"
// #include "daisysp.h"

// using namespace daisy;
// using namespace daisysp;

// constexpr size_t MAX_DELAY = 48000 * 5; // 5 seconds max delay

// struct DelayUnitReverse {
//     daisysp::DelayLineReverse<float, MAX_DELAY>* buffer;  
//     float currentDelay = 0.5f;    // in samples (after smoothing)
//     float delayTarget  = 0.5f;
//     float feedback     = 0.5f;
//     Tone lp_filter;
//     bool reverse       = false;
    
//     // For this reverse unit, we always use reverse playback.
//     // You could add a flag here if you want to toggle between forward and reverse.
    
//     void Init(float samplerate, daisysp::DelayLineReverse<float, MAX_DELAY>& buf)
//     {
//         buffer = &buf;
//         buffer->Reset();
//         lp_filter.Init(samplerate);
//         lp_filter.SetFreq(4000.0f);
//     }

//     // Process the audio sample using reverse delay.
//     float Process(float in)
//     {
//         // Smooth the delay time changes.
//         fonepole(currentDelay, delayTarget, 0.002f);
//         // Use SetDelay1 (as defined in DelayLineReverse) to set the delay time.
//         buffer->SetDelay1(currentDelay);
        
//         // Choose reading method based on the reverse flag.
//         float delayed = reverse ? buffer->ReadRev() : buffer->ReadFwd();
        
//         // Mix input with delayed sample using feedback.
//         float input = in + delayed * feedback;
//         float filtered = lp_filter.Process(input);
//         filtered = fminf(fmaxf(filtered, -1.0f), 1.0f);
        
//         // Write the processed sample into the buffer.
//         buffer->Write(filtered);
        
//         return delayed;
//     }
// };
