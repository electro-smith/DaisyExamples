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
// all copies or substantial portions of the Software.
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
// Main processing class.

#include "granular_processor.h"
#include <stdint.h>
#include <cstring>

//#include "debug_pin.h"

//#include "parameter_interpolator.h"
#include "buffer_allocator.h"
#include "resources.h"

//using namespace daisysp;
//using namespace daisy;
using namespace std;

void GranularProcessorClouds::Init(float  sample_rate,
                                   void*  large_buffer,
                                   size_t large_buffer_size,
                                   void*  small_buffer,
                                   size_t small_buffer_size)
{
    sample_rate_    = sample_rate;
    buffer_[0]      = large_buffer;
    buffer_[1]      = small_buffer;
    buffer_size_[0] = large_buffer_size;
    buffer_size_[1] = small_buffer_size;

    num_channels_ = 2;
    low_fidelity_ = false;
    bypass_       = false;

    src_down_.Init();
    src_up_.Init();

    ResetFilters();

    previous_playback_mode_ = PLAYBACK_MODE_LAST;
    reset_buffers_          = true;
    dry_wet_                = 0.0f;
}

void GranularProcessorClouds::ResetFilters()
{
    for(int32_t i = 0; i < 2; ++i)
    {
        fb_filter_[i].Init(sample_rate_);
        lp_filter_[i].Init(sample_rate_);
        hp_filter_[i].Init(sample_rate_);
    }
}

void GranularProcessorClouds::ProcessGranular(FloatFrame* input,
                                              FloatFrame* output,
                                              size_t      size)
{
    // At the exception of the spectral mode, all modes require the incoming
    // audio signal to be written to the recording buffer.
    if(playback_mode_ != PLAYBACK_MODE_SPECTRAL)
    {
        const float* input_samples = &input[0].l;
        for(int32_t i = 0; i < num_channels_; ++i)
        {
            if(resolution() == 8)
            {
                buffer_8_[i].WriteFade(
                    &input_samples[i], size, 2, !parameters_.freeze);
            }
            else
            {
                buffer_16_[i].WriteFade(
                    &input_samples[i], size, 2, !parameters_.freeze);
            }
        }
    }

    switch(playback_mode_)
    {
        case PLAYBACK_MODE_GRANULAR:
            // In Granular mode, DENSITY is a meta parameter.
            parameters_.granular.use_deterministic_seed
                = parameters_.density < 0.5f;
            if(parameters_.density >= 0.53f)
            {
                parameters_.granular.overlap
                    = (parameters_.density - 0.53f) * 2.12f;
            }
            else if(parameters_.density <= 0.47f)
            {
                parameters_.granular.overlap
                    = (0.47f - parameters_.density) * 2.12f;
            }
            else
            {
                parameters_.granular.overlap = 0.0f;
            }
            // And TEXTURE too.
            parameters_.granular.window_shape
                = parameters_.texture < 0.75f ? parameters_.texture * 1.333f
                                              : 1.0f;

            if(resolution() == 8)
            {
                player_.Play(buffer_8_, parameters_, &output[0].l, size);
            }
            else
            {
                player_.Play(buffer_16_, parameters_, &output[0].l, size);
            }
            break;

        case PLAYBACK_MODE_STRETCH:
            if(resolution() == 8)
            {
                ws_player_.Play(buffer_8_, parameters_, &output[0].l, size);
            }
            else
            {
                ws_player_.Play(buffer_16_, parameters_, &output[0].l, size);
            }
            break;

        case PLAYBACK_MODE_LOOPING_DELAY:
            if(resolution() == 8)
            {
                looper_.Play(buffer_8_, parameters_, &output[0].l, size);
            }
            else
            {
                looper_.Play(buffer_16_, parameters_, &output[0].l, size);
            }
            break;

        case PLAYBACK_MODE_SPECTRAL:
        {
            parameters_.spectral.quantization = parameters_.texture;
            parameters_.spectral.refresh_rate
                = 0.01f + 0.99f * parameters_.density;
            float warp                = parameters_.size - 0.5f;
            parameters_.spectral.warp = 4.0f * warp * warp * warp + 0.5f;

            float randomization = parameters_.density - 0.5f;
            randomization *= randomization * 4.2f;
            randomization -= 0.05f;
            CONSTRAIN(randomization, 0.0f, 1.0f);
            parameters_.spectral.phase_randomization = randomization;
            phase_vocoder_.Process(parameters_, input, output, size);

            if(num_channels_ == 1)
            {
                for(size_t i = 0; i < size; ++i)
                {
                    output[i].r = output[i].l;
                }
            }
        }
        break;

        default: break;
    }
}

void GranularProcessorClouds::Process(FloatFrame* input,
                                      FloatFrame* output,
                                      size_t      size)
{
    // TIC
    if(bypass_)
    {
        std::copy(&input[0], &input[size], &output[0]);
        return;
    }

    if(silence_ || reset_buffers_ || previous_playback_mode_ != playback_mode_)
    {
        float* output_samples = &output[0].l;
        std::fill(&output_samples[0], &output_samples[size << 1], 0);
        return;
    }

    for(size_t i = 0; i < size; ++i)
    {
        in_[i].l = input[i].l;
        in_[i].r = input[i].r;
    }
    if(num_channels_ == 1)
    {
        for(size_t i = 0; i < size; ++i)
        {
            in_[i].l = (in_[i].l + in_[i].r) * 0.5f;
            in_[i].r = in_[i].l;
        }
    }

    // Apply feedback, with high-pass filtering to prevent build-ups at very
    // low frequencies (causing large DC swings).
    ONE_POLE(freeze_lp_, parameters_.freeze ? 1.0f : 0.0f, 0.0005f)
    float feedback = parameters_.feedback;
    float cutoff   = (20.0f + 100.0f * feedback * feedback);

    fb_filter_[0].SetFreq(cutoff);
    fb_filter_[0].SetRes(1.f);

    fb_filter_[1].SetFreq(cutoff);
    fb_filter_[1].SetRes(1.f);

    for(size_t i = 0; i < size; i++)
    {
        fb_filter_[0].Process(fb_[i].l);
        fb_[i].l = fb_filter_[0].High();

        fb_filter_[1].Process(fb_[i].r);
        fb_[i].r = fb_filter_[1].High();
    }

    float fb_gain = feedback * (1.0f - freeze_lp_);
    for(size_t i = 0; i < size; ++i)
    {
        in_[i].l
            += fb_gain
               * (SoftLimit(fb_gain * 1.4f * fb_[i].l + in_[i].l) - in_[i].l);
        in_[i].r
            += fb_gain
               * (SoftLimit(fb_gain * 1.4f * fb_[i].r + in_[i].r) - in_[i].r);
    }

    if(low_fidelity_)
    {
        size_t downsampled_size = size / kDownsamplingFactor;
        src_down_.Process(in_, in_downsampled_, size);
        ProcessGranular(in_downsampled_, out_downsampled_, downsampled_size);
        src_up_.Process(out_downsampled_, out_, downsampled_size);
    }
    else
    {
        ProcessGranular(in_, out_, size);
    }

    // Diffusion and pitch-shifting post-processings.
    if(playback_mode_ != PLAYBACK_MODE_SPECTRAL)
    {
        float texture = parameters_.texture;
        float diffusion
            = playback_mode_ == PLAYBACK_MODE_GRANULAR
                  ? texture > 0.75f ? (texture - 0.75f) * 4.0f : 0.0f
                  : parameters_.density;
        diffuser_.set_amount(diffusion);
        diffuser_.Process(out_, size);
    }

    if(playback_mode_ == PLAYBACK_MODE_LOOPING_DELAY
       && (!parameters_.freeze || looper_.synchronized()))
    {
        pitch_shifter_.set_ratio(SemitonesToRatio(parameters_.pitch));
        pitch_shifter_.set_size(parameters_.size);
        pitch_shifter_.Process(out_, size);
    }

    // Apply filters.
    if(playback_mode_ == PLAYBACK_MODE_LOOPING_DELAY
       || playback_mode_ == PLAYBACK_MODE_STRETCH)
    {
        float cutoff    = parameters_.texture;
        float lp_cutoff = 0.5f
                          * SemitonesToRatio(
                              (cutoff < 0.5f ? cutoff - 0.5f : 0.0f) * 216.0f);
        float hp_cutoff = 0.25f
                          * SemitonesToRatio(
                              (cutoff < 0.5f ? -0.5f : cutoff - 1.0f) * 216.0f);
        CONSTRAIN(lp_cutoff, 0.0f, 0.499f);
        CONSTRAIN(hp_cutoff, 0.0f, 0.499f);
        float lpq = 1.0f + 3.0f * (1.0f - feedback) * (0.5f - lp_cutoff);
        lpq *= .25f;

        lp_filter_[0].SetFreq(lp_cutoff * sample_rate_);
        lp_filter_[0].SetRes(lpq);
        lp_filter_[1].SetFreq(lp_cutoff * sample_rate_);
        lp_filter_[1].SetRes(lpq);

        hp_filter_[0].SetFreq(hp_cutoff * sample_rate_);
        hp_filter_[0].SetRes(lpq);
        hp_filter_[1].SetFreq(hp_cutoff * sample_rate_);
        hp_filter_[1].SetRes(lpq);


        for(size_t i = 0; i < size; i++)
        {
            lp_filter_[0].Process(out_[i].l);
            out_[i].l = lp_filter_[0].Low();

            lp_filter_[1].Process(out_[i].r);
            out_[i].r = lp_filter_[1].Low();

            hp_filter_[0].Process(out_[i].l);
            out_[i].l = hp_filter_[0].High();

            hp_filter_[1].Process(out_[i].r);
            out_[i].r = hp_filter_[1].High();
        }
    }

    // This is what is fed back. Reverb is not fed back.
    std::copy(&out_[0], &out_[size], &fb_[0]);

    // Apply reverb.
    float reverb_amount = parameters_.reverb * 0.95f;
    reverb_amount += feedback * (2.0f - feedback) * freeze_lp_;
    CONSTRAIN(reverb_amount, 0.0f, 1.0f);

    reverb_.set_amount(reverb_amount * 0.54f);
    reverb_.set_diffusion(0.7f);
    reverb_.set_time(0.35f + 0.63f * reverb_amount);
    reverb_.set_input_gain(0.2f);
    reverb_.set_lp(0.6f + 0.37f * feedback);
    reverb_.Process(out_, size);

    const float           post_gain = 1.2f;
    ParameterInterpolator dry_wet_mod(&dry_wet_, parameters_.dry_wet, size);
    for(size_t i = 0; i < size; ++i)
    {
        float dry_wet  = dry_wet_mod.Next();
        float fade_in  = Interpolate(lut_xfade_in, dry_wet, 16.0f);
        float fade_out = Interpolate(lut_xfade_out, dry_wet, 16.0f);
        float l        = input[i].l * fade_out;
        float r        = input[i].r * fade_out;
        l += out_[i].l * post_gain * fade_in;
        r += out_[i].r * post_gain * fade_in;
        output[i].l = l;
        output[i].r = r;
    }
}

void GranularProcessorClouds::Prepare()
{
    bool playback_mode_changed = previous_playback_mode_ != playback_mode_;
    bool benign_change = previous_playback_mode_ != PLAYBACK_MODE_SPECTRAL
                         && playback_mode_ != PLAYBACK_MODE_SPECTRAL
                         && previous_playback_mode_ != PLAYBACK_MODE_LAST;

    if(!reset_buffers_ && playback_mode_changed && benign_change)
    {
        ResetFilters();
        pitch_shifter_.Clear();
        previous_playback_mode_ = playback_mode_;
    }

    if((playback_mode_changed && !benign_change) || reset_buffers_)
    {
        parameters_.freeze = false;
    }

    if(reset_buffers_ || (playback_mode_changed && !benign_change))
    {
        void*  buffer[2];
        size_t buffer_size[2];
        void*  workspace;
        size_t workspace_size;
        if(num_channels_ == 1)
        {
            // Large buffer: 120k of sample memory.
            // small buffer: fully allocated to FX workspace.
            buffer[0]      = buffer_[0];
            buffer_size[0] = buffer_size_[0];
            buffer[1]      = NULL;
            buffer_size[1] = 0;
            workspace      = buffer_[1];
            workspace_size = buffer_size_[1];
        }
        else
        {
            // Large buffer: 64k of sample memory + FX workspace.
            // small buffer: 64k of sample memory.
            buffer_size[0] = buffer_size[1] = buffer_size_[1];
            buffer[0]                       = buffer_[0];
            buffer[1]                       = buffer_[1];

            workspace_size = buffer_size_[0] - buffer_size_[1];
            workspace      = static_cast<uint8_t*>(buffer[0]) + buffer_size[0];
        }
        float sr = sample_rate();

        BufferAllocator allocator(workspace, workspace_size);
        diffuser_.Init(allocator.Allocate<float>(2048));
        reverb_.Init(allocator.Allocate<uint16_t>(16384));

        size_t    correlator_block_size = (kMaxWSOLASize / 32) + 2;
        uint32_t* correlator_data
            = allocator.Allocate<uint32_t>(correlator_block_size * 3);
        correlator_.Init(&correlator_data[0],
                         &correlator_data[correlator_block_size]);
        pitch_shifter_.Init((uint16_t*)correlator_data);

        if(playback_mode_ == PLAYBACK_MODE_SPECTRAL)
        {
            phase_vocoder_.Init(buffer,
                                buffer_size,
                                lut_sine_window_4096,
                                4096,
                                num_channels_,
                                resolution(),
                                sr);
        }
        else
        {
            for(int32_t i = 0; i < num_channels_; ++i)
            {
                if(resolution() == 8)
                {
                    buffer_8_[i].Init(
                        buffer[i], (buffer_size[i]), tail_buffer_[i]);
                }
                else
                {
                    buffer_16_[i].Init(
                        buffer[i], ((buffer_size[i]) >> 1), tail_buffer_[i]);
                }
            }
            int32_t num_grains
                = (num_channels_ == 1 ? 40 : 32) * (low_fidelity_ ? 23 : 16)
                  >> 4;
            player_.Init(num_channels_, num_grains);
            ws_player_.Init(&correlator_, num_channels_);
            looper_.Init(num_channels_);
        }
        reset_buffers_          = false;
        previous_playback_mode_ = playback_mode_;
    }

    if(playback_mode_ == PLAYBACK_MODE_SPECTRAL)
    {
        phase_vocoder_.Buffer();
    }
    else if(playback_mode_ == PLAYBACK_MODE_STRETCH)
    {
        if(resolution() == 8)
        {
            ws_player_.LoadCorrelator(buffer_8_);
        }
        else
        {
            ws_player_.LoadCorrelator(buffer_16_);
        }
        correlator_.EvaluateSomeCandidates();
    }
}
