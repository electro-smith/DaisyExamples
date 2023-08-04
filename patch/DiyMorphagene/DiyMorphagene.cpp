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

// minimum amount required for raw control to change
// to trigger updating stable_ctrl_values and
// recalulating sample start and ends
const float CTRL_TOLERANCE = 0.01;

// stable values from control
// updated if read raw value is within CTRL_TOLERANCE
float stable_ctrl_values[4];

enum State {
  WAITING,    // initial powered up
  RECORDING,  // recording sample
  PLAYING,    // looping playback
};
State state = WAITING;

class Grain {
  public:

    Grain() {}

    void SetStartEnd(const float ctrl_a_value, const float ctrl_b_value) {
      if (state!=PLAYING) return;
      start_ = static_cast<size_t>(ctrl_a_value * sample_length);
      SetEffectiveSampleStart();
      end_ = start_ + static_cast<size_t>(ctrl_b_value * sample_length);
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
        ResetPlaybackHead();
      }
      const float to_return = buffer[playback_head_];
      playback_head_++;
      return to_return;
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

    // playback head position
    size_t playback_head_;

    // starting position dictated by ctrl2.
    size_t start_;

    // effective sample start; first +ve zero crossing after sample_start
    size_t effective_start_;

    // effective sample end; last +ve zero crossing before sample_end
    size_t effective_end_;

    // sample end dictated by ctrl3 (for length) + sample_start ( from ctrl2 )
    size_t end_;
};

Grain grains[4];

bool CheckCtrlValues() {
  bool at_least_one_changed = false;
  for (size_t c=0; c<4; c++) {
    float val = hw.controls[c].Value();
    if (abs(stable_ctrl_values[c] - val) > CTRL_TOLERANCE) {
      if (val < 0.01) {
        val = 0;
      } else if (val > 0.98) {
        val = 1.0;
      }
      stable_ctrl_values[c] = val;
      at_least_one_changed = true;
    }
  }
  return at_least_one_changed;
}

void UpdateAllSampleStartEnds() {
  const float start_base = stable_ctrl_values[0];
  const float start_spread = stable_ctrl_values[1];
  const float length_base = stable_ctrl_values[2];
  const float length_spread = stable_ctrl_values[3];

  for (size_t i=0; i<4; i++) {
    const float p = float(i) / 3;  // [0, 1/3, 2/3, 1]
    grains[i].SetStartEnd(start_base + (p * start_spread),
                          length_base + (p * length_spread));
  }

}

void StopRecording() {
  // snap shot new sample length
  sample_length = record_head-1;
  // reset all grains back to PLAYING
  state = PLAYING;
  UpdateAllSampleStartEnds();
  for (auto& grain : grains) {
    grain.ResetPlaybackHead();
  }
}

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {

  for (size_t b = 0; b < size; b++) {
    switch(state) {

      case WAITING:
        for (size_t c = 0; c < 4; c++) {
          out[c][b] = in[c][b];
        }
        break;

      case RECORDING:
        for (size_t c = 0; c < 4; c++) {
          out[c][b] = in[c][b];
        }
        buffer[record_head] = in[0][b];
        record_head++;
        if (record_head == BUFFER_SIZE) {
          StopRecording();
        }
        break;

      case PLAYING:
        for (size_t c = 0; c < 4; c++) {
          out[c][b] = grains[c].Playback();
        }
        break;

        // DEBUG
        // // core wave based on proportion we are through effective sample
        // float core = float(sample_offset - sample_effective_start) / (sample_effective_end - sample_effective_start);
        // out[1][b] = core;
        // break;
    }

  }

}

void UpdateControls() {
  hw.ProcessAllControls();

  // if any control has changed update sample start / end for
  // all grains. this could be optimised to only trigger any update
  // for the applicable grains based on mode, but it works fine as is.
  const bool at_least_one_control_changed = CheckCtrlValues();
  if (at_least_one_control_changed) {
    UpdateAllSampleStartEnds();
  }

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

  // for (size_t c=0; c<4; c++) {
  //   str.Clear();
  //   str.AppendFloat(stable_ctrl_values[c], 3);
  //   strs.push_back(string(str));
  // }

  for (size_t g=0; g<4; g++) {
    auto& grain = grains[g];
    str.Clear();
    str.Append("g");
    str.AppendInt(g);
    str.Append(" ");
    str.AppendFloat(grain.GetStartP(), 3);
    str.Append(" ");
    str.AppendFloat(grain.GetEndP(), 3);
    strs.push_back(string(str));
  }

  DisplayLines(strs);
  hw.display.Update();
}

int main(void) {
  hw.Init();

  hw.SetAudioBlockSize(64); // number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while(true) {
    for (size_t i = 0; i < 100; i++) {
      UpdateControls();
    }
    UpdateDisplay();
  }

}
