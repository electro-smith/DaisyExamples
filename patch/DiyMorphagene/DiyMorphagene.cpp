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

enum CtrlMode {
  TWO_GRAINS,  // 2 grains, 2 start ctrls, 2 len ctrls   S1 L1 S2 L2
  S1_L3,       // 3 grains, 1 start ctrl, 3 len ctrls    S* L1 L2 L3
  S3_L1,     // 3 grains, 3 start ctrls, 1 len ctrl    S1 s2 S3 L*
};
const CtrlMode LAST_CTRL_MODE = S3_L1;
CtrlMode ctrl_mode = TWO_GRAINS;

enum State {
  WAITING,    // initial powered up
  RECORDING,  // recording sample
  PLAYING,    // looping playback
};
State state = WAITING;

class Grain {
  public:

    Grain() {}

    void SetControlIndexs(size_t a, size_t b) {
      control_a_idx_ = a;
      control_b_idx_ = b;
      UpdateSampleStartEnd();
    }

    void UpdateSampleStartEnd() {
      if (state!=PLAYING) return;
      const float ctrl_a_value = stable_ctrl_values[control_a_idx_];
      const float ctrl_b_value = stable_ctrl_values[control_b_idx_];
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
        playback_head_ = effective_start_;
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

    // indexes of the two controls for this grain
    size_t control_a_idx_;
    size_t control_b_idx_;

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

// we need three grains, though for ctrl_mode==TWO_GRAINS we won't use
// the third.
Grain grains[3];

bool CheckCtrlValues() {
  bool at_least_one_changed = false;
  for (size_t c=0; c<4; c++) {
    const float val = hw.controls[c].Value();
    if (abs(stable_ctrl_values[c] - val) > CTRL_TOLERANCE) {
      stable_ctrl_values[c] = val;
      at_least_one_changed = true;
    }
  }
  return at_least_one_changed;
}

void StopRecording() {
  sample_length = record_head-1;
  state = PLAYING;
  for (auto& grain : grains) {
    grain.UpdateSampleStartEnd();
    grain.ResetPlaybackHead();
  }
}

void ProcessModeChange() {
  switch (ctrl_mode) {
    case TWO_GRAINS:
      grains[0].SetControlIndexs(0, 1);
      grains[1].SetControlIndexs(2, 3);
      // ignore how grain[2] is configured
      break;
    case S1_L3:
      grains[0].SetControlIndexs(0, 1);
      grains[1].SetControlIndexs(0, 2);
      grains[2].SetControlIndexs(0, 3);
      break;
    case S3_L1:
      grains[0].SetControlIndexs(0, 3);
      grains[1].SetControlIndexs(1, 3);
      grains[2].SetControlIndexs(2, 3);
      break;
  }
}

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {

  for (size_t b = 0; b < size; b++) {
    switch(state) {

      case WAITING:
        out[0][b] = in[0][b];
        out[1][b] = in[0][b];
        out[2][b] = in[0][b];
        break;

      case RECORDING:
        out[0][b] = in[0][b];
        out[1][b] = in[0][b];
        out[2][b] = in[0][b];
        buffer[record_head] = in[0][b];
        record_head++;
        if (record_head == BUFFER_SIZE) {
          StopRecording();
        }
        break;

      case PLAYING:
        out[0][b] = grains[0].Playback();
        out[1][b] = grains[1].Playback();
        out[2][b] = (ctrl_mode==TWO_GRAINS) ? 0 : grains[2].Playback();
        // // core wave based on proportion we are through effective sample
        // float core = float(sample_offset - sample_effective_start) / (sample_effective_end - sample_effective_start);
        // out[1][b] = core;
        // break;
    }

  }

}

size_t n = 0;

void UpdateControls() {
  hw.ProcessAllControls();

  // if any control has changed update sample start / end for
  // all grains
  const bool at_least_one_control_changed = CheckCtrlValues();
  if (at_least_one_control_changed && state==PLAYING) {
    for (auto& grain : grains) {
      grain.UpdateSampleStartEnd();
    }
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

  // turning encoder cycles through control modes
  const int enc_increment = hw.encoder.Increment();
  bool ctrl_mode_changed = false;
  if (enc_increment > 0) {
    if (ctrl_mode == LAST_CTRL_MODE) {
      ctrl_mode = static_cast<CtrlMode>(0);
    } else {
      // clumsy :/
      int ctrl_mode_i = static_cast<int>(ctrl_mode);
      ctrl_mode_i++;
      ctrl_mode = static_cast<CtrlMode>(ctrl_mode_i);
    }
    ctrl_mode_changed = true;
  } else if (enc_increment < 0) {
    int ctrl_mode_i = static_cast<int>(ctrl_mode);
    if (ctrl_mode_i == 0) {
      ctrl_mode = LAST_CTRL_MODE;
    } else {
      ctrl_mode_i--;
      ctrl_mode = static_cast<CtrlMode>(ctrl_mode_i);
    }
    ctrl_mode_changed = true;
  }
  if (ctrl_mode_changed) {
    ProcessModeChange();
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

  switch(ctrl_mode) {
    case TWO_GRAINS:
      strs.push_back("S1 L1 S2 L2");
      break;
    case S1_L3:
      strs.push_back("S* L1 L2 L3");
      break;
    case S3_L1:
      strs.push_back("S1 S2 S3 L*");
      break;
  }

  str.Clear();
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

  for (size_t g=0; g<3; g++) {
    if (g==2 && ctrl_mode == TWO_GRAINS) {
      strs.push_back("");
    } else {
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

  grains[2].SetControlIndexs(0, 1);  // ensure grain[2] always in valid state
  ctrl_mode = TWO_GRAINS;
  ProcessModeChange();

  while(true) {
    for (size_t i = 0; i < 100; i++) {
      UpdateControls();
    }
    UpdateDisplay();
  }

}
