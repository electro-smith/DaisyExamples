// Copyright 2019 Emilie Gillet.
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
// Threshold type filter that filters out slow jumps of a value.

#ifndef STMLIB_DSP_HYSTERESIS_FILTER_H_
#define STMLIB_DSP_HYSTERESIS_FILTER_H_



namespace daisy {

class HysteresisFilter {
 public:
  HysteresisFilter() { }
  ~HysteresisFilter() { }

  void Init(float threshold) {
    value_ = 0.0f;
    threshold_ = threshold;
  }
  
  inline float Process(float value) {
    return Process(value, threshold_);
  }

  inline float Process(float value, float threshold) {
    if (threshold == 0.0f) {
      value_ = value;
    } else {
      float error = value - value_;
      if (error > threshold) {
        value_ = value - threshold;
      } else if (error < -threshold) {
        value_ = value + threshold;
      }
    }
    return value_;
  }

  inline float value() const { return value_; }

 private:
  float value_;
  float threshold_;

  DISALLOW_COPY_AND_ASSIGN(HysteresisFilter);
};

}  // namespace daisy

#endif  // STMLIB_DSP_HYSTERESIS_FILTER_H_
