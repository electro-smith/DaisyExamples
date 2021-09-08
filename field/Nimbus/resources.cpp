// Copyright 2014 Emilie Gillet.
//
// Author: Emilie Gillet (emilie.o.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantiafl portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// Resources definitions.
//
// Automatically generated with:
// make resources


#include "resources.h"
#include <stdint.h>
#include "daisysp.h"

using namespace daisysp;

int16_t  lut_ulaw[LUT_ULAW_SIZE];
float    lut_pitch_ratio_high[LUT_PITCH_RATIO_HIGH_SIZE];
float    lut_pitch_ratio_low[LUT_PITCH_RATIO_LOW_SIZE];
uint16_t atan_lut[ATAN_LUT_SIZE];

float lut_sin[LUT_SIN_SIZE];
float lut_window[LUT_WINDOW_SIZE];
float lut_xfade_in[LUT_XFADE_IN_SIZE];
float lut_xfade_out[LUT_XFADE_OUT_SIZE];
float lut_sine_window_4096[LUT_SINE_WINDOW_4096_SIZE];
float lut_grain_size[LUT_GRAIN_SIZE_SIZE];

//helper for sine window
void sum_window(float* window, int steps, float* output)
{
    int n      = LUT_SINE_WINDOW_4096_SIZE;
    int start  = 0;
    int stride = n / steps;

    for(int i = 0; i < LUT_SINE_WINDOW_4096_SIZE; i++)
    {
        output[i] = 0.f;
    }

    for(int i = 0; i < steps; i++)
    {
        for(int j = start; j < start + stride; j++)
        {
            output[j - start] += powf(window[j], 2.f);
            output[j] = output[j - start];
        }

        start += stride;
    }
}

//helper for ulaw encoding
inline short MuLaw2Lin(uint8_t u_val)
{
    int16_t t;
    u_val = ~u_val;
    t     = ((u_val & 0xf) << 3) + 0x84;
    t <<= ((unsigned)u_val & 0x70) >> 4;
    return ((u_val & 0x80) ? (0x84 - t) : (t - 0x84));
}

//init all the luts
void InitResources(float sample_rate)
{
    // lut_ulaw
    for(int i = 0; i < LUT_ULAW_SIZE; i++)
    {
        lut_ulaw[i] = MuLaw2Lin(i);
    }

    // lut_pitch_ratio_high + lut_pitch_ratio_low
    for(int i = 0; i < LUT_PITCH_RATIO_HIGH_SIZE; i++)
    {
        float ratio             = (float)i - 128.f;
        ratio                   = powf(2.f, ratio / 12.f);
        float semitone          = powf(2.f, (float)i / 256.0 / 12.0);
        lut_pitch_ratio_high[i] = ratio;
        lut_pitch_ratio_low[i]  = semitone;
    }

    // atan_lut
    for(size_t i = 0; i < ATAN_LUT_SIZE; ++i)
    {
        atan_lut[i] = 65536.0 / (2 * PI_F) * asinf(i / 512.0f);
    }

    // lut_sin
    for(int i = 0; i < LUT_SIN_SIZE; i++)
    {
        float t    = (float)i / 1024.f * TWOPI_F;
        lut_sin[i] = sin(t);
    }

    // lut_window
    for(int i = 0; i < LUT_WINDOW_SIZE; i++)
    {
        float t       = (float)i / (float)LUT_WINDOW_SIZE;
        lut_window[i] = 1.f - ((cos(t * PI_F) + 1.f) * .5f);
    }

    // lut_xfade_in + lut_xfade_out
    for(int i = 0; i < LUT_XFADE_IN_SIZE; i++)
    {
        float t = i / (float)(LUT_XFADE_IN_SIZE - 1);
        t       = 1.04 * t - 0.02f;
        t       = fclamp(t, 0.f, 1.f);
        t *= PI_F / 2.f;

        float two_neg_half = powf(2, -.5f);
        lut_xfade_in[i]    = sin(t) * two_neg_half;
        lut_xfade_out[i]   = cos(t) * two_neg_half;
    }

    // lut_sine_window_4096
    for(int i = 0; i < LUT_SINE_WINDOW_4096_SIZE; i++)
    {
        float t = (float)i / (float)LUT_SINE_WINDOW_4096_SIZE;
        //float sine = sin(PI_F * t);
        //float raised = (0.5f * cos(PI_F * t * 2.f) + 0.5f) * sqrt(4.f / 3.f);

        lut_sine_window_4096[i] = powf(1.f - powf((2.f * t - 1.f), 2.f), 1.25f);
    }

    float compensation[LUT_SINE_WINDOW_4096_SIZE];
    sum_window(lut_sine_window_4096, 2, compensation);

    for(int i = 0; i < LUT_SINE_WINDOW_4096_SIZE; i++)
    {
        compensation[i] = powf(compensation[i], .5f);
        lut_sine_window_4096[i] /= compensation[i];
    }

    // lut_grain_size
    for(int i = 0; i < LUT_GRAIN_SIZE_SIZE; i++)
    {
        float size        = ((float)i / (float)LUT_GRAIN_SIZE_SIZE) * 4.f;
        lut_grain_size[i] = floor(1024.f * powf(2, size));
    }
}
const float src_filter_1x_2_45[] = {
    -6.928606892e-04, -5.894682972e-03, 4.393903915e-04,  5.352009980e-03,
    1.833575577e-03,  -7.103853054e-03, -5.275577768e-03, 7.999060050e-03,
    1.029879712e-02,  -7.191125897e-03, -1.675763381e-02, 3.628265970e-03,
    2.423749384e-02,  4.020326715e-03,  -3.208822586e-02, -1.775516900e-02,
    3.947412082e-02,  4.200610725e-02,  -4.553678524e-02, -9.270618476e-02,
    4.952442102e-02,  3.157869177e-01,  4.528032253e-01,  3.157869177e-01,
    4.952442102e-02,  -9.270618476e-02, -4.553678524e-02, 4.200610725e-02,
    3.947412082e-02,  -1.775516900e-02, -3.208822586e-02, 4.020326715e-03,
    2.423749384e-02,  3.628265970e-03,  -1.675763381e-02, -7.191125897e-03,
    1.029879712e-02,  7.999060050e-03,  -5.275577768e-03, -7.103853054e-03,
    1.833575577e-03,  5.352009980e-03,  4.393903915e-04,  -5.894682972e-03,
    -6.928606892e-04,
};
