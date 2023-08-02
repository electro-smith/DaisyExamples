#include <string>

#include "daisy_patch.h"
#include "daisysp.h"
#include "Dynamics/crossfade.h"

using namespace std;
using namespace daisy;
using namespace daisysp;

DaisyPatch hw;

const size_t BUFFER_SIZE = 60 * 48000;  // max of 60sec
float DSY_SDRAM_BSS buffer[BUFFER_SIZE];

// record head position
size_t record_head;

// actual recorded loop
size_t sample_length;

enum State {
  WAITING,
  RECORDING,
  PLAYING,
};
State state = State::WAITING;

class Grain {
  public:

    Grain() {}

    void SetControlIndexs(size_t a, size_t b) {
      control_a_idx_ = a;
      control_b_idx_ = b;
    }

    void UpdateSampleStartEnd() {
      start_ = static_cast<size_t>(last_ctrl_a_val_ * sample_length);
      SetEffectiveSampleStart();
      end_ = start_ + static_cast<size_t>(last_ctrl_b_val_ * sample_length);
      if (end_ >= sample_length) {
        end_ = sample_length-1;
      }
      SetEffectiveSampleEnd();
    }

    void ResetPlaybackHead() {
      playback_head_ = effective_start_;
    }

    float Playback() {
      if (playback_head_ >= effective_end_) {
        playback_head_ = effective_start_;
      }
      const float to_return = buffer[playback_head_];
      playback_head_++;
      return to_return;
    }

    void CheckControlsAndUpdateEndsIfRequired() {
      // check if ctrl2 ( sample start ) or ctrl3 ( sample length ) have changed
      bool changed = false;
      float val = hw.controls[control_a_idx_].Value();
      if (abs(last_ctrl_a_val_ - val) > 0.001) {
        changed = true;
        last_ctrl_a_val_ = val;
      }
      val = hw.controls[control_b_idx_].Value();
      if (abs(last_ctrl_b_val_ - val) > 0.001) {
        changed = true;
        last_ctrl_b_val_ = val;
      }
      // if either have changed we need to update the sample
      // start/end BUT we only do this dynamically during playback
      if (changed && state == PLAYING) {
        UpdateSampleStartEnd();
      }
    }

    inline float GetStartP() const {
      return float(effective_start_) / sample_length;
    }

    inline float GetEndP() {
      return float(effective_end_) / sample_length;
    }

  private:

    void SetEffectiveSampleStart() {
      // decide effective sample start by seeking forward
      // from start of buffer to first positive crossing
      float last_val = buffer[start_];
      for (size_t b = start_+1; b < sample_length; b++) {
        float value = buffer[b];
        if (last_val < 0 && value > 0) {
          effective_start_ = b;
          return;
        }
        last_val = value;
      }
      // failed to find a crossing :()
      effective_start_ = start_;
    }

    void SetEffectiveSampleEnd() {
      // decide effective sample end by seeking backwards
      // from the end of the buffer to find the last
      // positive crossing
      float last_val = buffer[end_-1];
      for (size_t b = end_-2; b > effective_start_; b--) {
        float value = buffer[b];
        if (last_val > 0 && value < 0) {
          effective_end_ = b;
          return;
        }
        last_val = value;
      }
      // failed to find a crossing :()
      effective_end_ = end_-1;
    }

    size_t control_a_idx_;
    size_t control_b_idx_;

    // playback head position
    size_t playback_head_;

    // starting position dictated by ctrl2.
    float last_ctrl_a_val_;
    size_t start_;

    // effective sample start; first +ve zero crossing after sample_start
    size_t effective_start_;

    // effective sample end; last +ve zero crossing before sample_end
    size_t effective_end_;

    // sample end dictated by ctrl3 (for length) + sample_start ( from ctrl2 )
    float last_ctrl_b_val_;
    size_t end_;
};

Grain grain;

void StopRecording() {
  sample_length = record_head-1;
  grain.UpdateSampleStartEnd();
  grain.ResetPlaybackHead();
  state = PLAYING;
}

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {

  for (size_t b = 0; b < size; b++) {
    switch(state) {

      case WAITING:
        out[0][b] = in[0][b];
        break;

      case RECORDING:
        out[0][b] = in[0][b];
        buffer[record_head] = in[0][b];
        record_head++;
        if (record_head == BUFFER_SIZE) {
          StopRecording();
        }
        break;

      case PLAYING:
        out[0][b] = grain.Playback();

        // // core wave based on proportion we are through effective sample
        // float core = float(sample_offset - sample_effective_start) / (sample_effective_end - sample_effective_start);
        // out[1][b] = core;
        // break;
    }

  }

}

void UpdateControls() {
  hw.ProcessAllControls();

  grain.CheckControlsAndUpdateEndsIfRequired();

  // click on encoder or trig at gate1 toggles recording
  if (hw.encoder.RisingEdge() || hw.gate_input[0].Trig()) {
    if (state == RECORDING) {
      StopRecording();
    } else {
      state = RECORDING;
      record_head = 0;
    }
  }

}

void DisplayLines(const vector<string> &strs) {
  int line_num = 0;
  for (string str : strs) {
    char* cstr = &str[0];
    hw.display.SetCursor(0, line_num*10);
    hw.display.WriteString(cstr, Font_7x10, true);
    line_num++;
  }
}

void UpdateDisplay() {
  hw.display.Fill(false);
  vector<string> strs;

  FixedCapStr<18> str("");

  str.Append("state ");
  switch (state) {
    case WAITING:
      str.Append("WAITING ");
      break;
    case RECORDING:
      str.Append("RECORDING ");
      break;
    case PLAYING:
      str.Append("PLAYING ");
      break;
  }
  strs.push_back(string(str));

//  for (size_t g=0; g<2; g++) {
    str.Clear();
    str.Append("g");
//    str.AppendInt(g);
    str.Append(" ");
    str.AppendFloat(grain.GetStartP(), 3);
    str.Append(" ");
    str.AppendFloat(grain.GetEndP(), 3);
    strs.push_back(string(str));
//  }

  DisplayLines(strs);
  hw.display.Update();
}

int main(void) {
  hw.Init();

  hw.SetAudioBlockSize(64); // number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  grain.SetControlIndexs(0, 1);

  while(true) {
    for (size_t i = 0; i < 100; i++) {
      UpdateControls();
    }
    UpdateDisplay();
  }

}
