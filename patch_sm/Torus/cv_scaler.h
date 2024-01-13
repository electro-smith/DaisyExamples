// Copyright 2015 Emilie Gillet.
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
// Filtering and scaling of ADC values + input calibration.

#ifndef TORUS_CV_SCALER_H_
#define TORUS_CV_SCALER_H_

#include "stmlib/stmlib.h"
#include "daisy.h"

namespace torus
{
enum Law
{
    LAW_LINEAR,
    LAW_QUADRATIC_BIPOLAR,
    LAW_QUARTIC_BIPOLAR
};

struct ChannelSettings
{
    Law   law;
    bool  remove_offset;
    float lp_coefficient;
};

enum Channels
{
    POT_FREQUENCY,
    POT_STRUCTURE,
    POT_BRIGHTNESS,
    POT_DAMPING,
    POT_POSITION,
    CV_V_OCT,
    CV_STRUCTURE,
    CV_BRIGHTNESS,
    CV_POSITION,
    CHAN_LAST
};

struct Patch;
struct PerformanceState;

class CvScaler
{
  public:
    CvScaler() {}
    ~CvScaler() {}

    void Init();
    void Read(Patch*                  patch,
              PerformanceState*       performance_state,
              daisy::VoctCalibration* calibration_data_);

    inline void MapChannel(int channel, int control)
    {
        channel_map_[channel] = control;
    }

  private:
    // This is one less than the number of channels as frequency and structure
    // share a knob.
    static const int       kNumControls = CHAN_LAST - 1;
    static ChannelSettings channel_settings_[CHAN_LAST];
    float                  adc_lp_[CHAN_LAST];
    int                    channel_map_[kNumControls];

    float transpose_;
    float fm_cv_;
    float cv_c1_;
    float cv_low_;

    int32_t chord_;
};

} // namespace torus

#endif // TORUS_CV_SCALER_H_
