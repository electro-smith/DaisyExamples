#include <string>

#include "daisy_patch.h"
#include "daisysp.h"
#include "Dynamics/crossfade.h"

using namespace std;
using namespace daisy;
using namespace daisysp;

DaisyPatch hw;

// keep a playback vs record buffer
const size_t BUFFER_SIZE = 60 * 48000;  // max of 60sec
using Buffer = float[BUFFER_SIZE];
Buffer DSY_SDRAM_BSS global_buffer[2];

Buffer* playback_buffer = &(global_buffer[0]);
Buffer* record_buffer = &(global_buffer[1]);

// set to true after first recording; until then
// playback input directly
bool have_recording = false;

// record head position
size_t record_head;

// actual recorded loop
size_t sample_length;

// minimum amount required for raw control to change
// to trigger updating stable_ctrl_values and
// recalulating sample start and ends
const float CTRL_TOLERANCE = 0.01;

// use Parameter for controls, rather than direct values
// since we want non linear for length.
Parameter ctrls[4];

// stable values from control
// updated if read raw value is within CTRL_TOLERANCE
float stable_ctrl_values[4];

CrossFade output_mixer;

enum State {
  WAITING,    // initial powered up
  RECORDING,  // recording sample
  PLAYING,    // looping playback
};
State state = WAITING;

class Grain {
  public:

    Grain() {}

    void SetStartEnd(float start_f, float length_f) {
      if (state!=PLAYING) return;
      start_f = fclamp(start_f, 0.0f, 1.0f);
      length_f = fclamp(length_f, 0.0f, 1.0f);

      start_ = static_cast<size_t>(start_f * sample_length);
      if (start_ >= sample_length) {
        start_ = sample_length-1;
      }
      SetEffectiveSampleStart();

      end_ = start_ + static_cast<size_t>(length_f * sample_length);
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
      const float to_return = (*playback_buffer)[playback_head_];
      playback_head_++;
      return to_return;
    }

    inline float GetStartP() const {
      return float(effective_start_) / sample_length;
    }

    inline float GetCurrentP() const {
      return float(playback_head_) / sample_length;
    }

    inline float GetEndP() {
      return float(effective_end_) / sample_length;
    }

  private:

    void SetEffectiveSampleStart() {
      // decide effective sample start by seeking forward
      // from start of grain buffer to first positive crossing
      bool last_val_lt_zero = (*playback_buffer)[start_] < 0;
      for (size_t b = start_+1; b < sample_length; b++) {
        float value = (*playback_buffer)[b];
        if (last_val_lt_zero && value > 0) {
          effective_start_ = b;
          return;
        }
        last_val_lt_zero = value < 0;
      }
      // failed to find a crossing :()
      effective_start_ = start_;
    }

    void SetEffectiveSampleEnd() {
      // decide effective sample end by seeking backwards
      // from the end of grain buffer to find the last
      // positive crossing
      bool last_val_gt_zero = (*playback_buffer)[end_-1] > 0;
      for (size_t b = end_-2; b > effective_start_; b--) {
        float value = (*playback_buffer)[b];
        if (last_val_gt_zero && value < 0) {
          effective_end_ = b;
          return;
        }
        last_val_gt_zero = value > 0;
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
const size_t NUM_GRAINS = 16; // MUST be a power of two based on how folding over output is done...
Grain grains[NUM_GRAINS];

bool CheckCtrlValues() {
  bool at_least_one_changed = false;
  for (size_t c=0; c<4; c++) {
    // TODO: .Debounce() instead of CTRL_TOL?
    float val = ctrls[c].Process();
    if (abs(stable_ctrl_values[c] - val) > CTRL_TOLERANCE) {
      // snap everything EXCEPT sample length to (0, 1)
      if (c != 2) {
        if (val < 0.01) {
          val = 0;
        } else if (val > 0.98) {
          val = 1.0;
        }
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

  for (size_t i=0; i<NUM_GRAINS; i++) {
    const float p = float(i) / (NUM_GRAINS-1);  // [0, 1/3, 2/3, 1]
    grains[i].SetStartEnd(start_base + (p * start_spread),
                          length_base + (p * length_spread));
  }

}

void StopRecording() {
  // snap shot new sample length
  sample_length = record_head-1;
  // swap playback and recording buffer
  Buffer* tmp = playback_buffer;
  playback_buffer = record_buffer;
  record_buffer = tmp;
  // we have a recording now!
  have_recording = true;
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

  // TODO(mat) move b to inner loop

  for (size_t b = 0; b < size; b++) {

    if (have_recording) {
      // collect array of all grains
      float g[NUM_GRAINS];
      for (size_t i = 0; i < NUM_GRAINS; i++) {
        g[i] = grains[i].Playback();
      }

      // for some stability have out0 be the first grain
      out[0][b] = g[0];

      // fold together until it's two single values in g[0] and g[1]
      /*  this is sparta mode
      size_t size = NUM_GRAINS;
      while (size > 2) {
        size_t in1 = 0;
        size_t in2 = size/2;
        const size_t new_size = size/2;
        while (in1 < new_size) {
          g[in1] = output_mixer.Process(g[in1], g[in2]);
          in1++;
          in2++;
        }
        size = new_size;
      }
      */
      size_t size = NUM_GRAINS;
      while (size > 2) {
        size_t in = 0;
        while (in < size) {
          g[in/2] = output_mixer.Process(g[in], g[in+1]);
          in += 2;
        }
        size /= 2;
      }

      // have out2 and out3 be the last two folded values
      out[1][b] = g[0];
      out[2][b] = g[1];

      // and out4 be the very final
      out[3][b] = output_mixer.Process(g[0], g[1]);  // final mix
    } else {
      // just monitor input directly
      for (size_t c = 0; c < 4; c++) {
        out[c][b] = in[c][b];
      }
    }

    // only record in explicitly in that state
    if (state==RECORDING) {
      (*record_buffer)[record_head] = in[0][b];
      record_head++;
      if (record_head == BUFFER_SIZE) {
        StopRecording();
      }
    }

    // DEBUG for mordax sync
    // // core wave based on proportion we are through effective sample
    // float core = float(sample_offset - sample_effective_start) / (sample_effective_end - sample_effective_start);
    // out[1][b] = core;
    // break;

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

  // trig to gate 2 restarts all playbacks
  if (hw.gate_input[1].Trig() && state == PLAYING) {
    for (auto& grain : grains) {
      grain.ResetPlaybackHead();
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


  if (state == WAITING) {
    strs.push_back("WAITING");

  } else if (state == RECORDING) {
    strs.push_back("RECORDING");
    float proportion_buffer_filled = static_cast<float>(record_head) / BUFFER_SIZE;
    FixedCapStr<18> str("");
    str.AppendFloat(proportion_buffer_filled, 4);
    strs.push_back(string(str));

  } else { // PLAYING
    strs.push_back("PLAYING");
    FixedCapStr<18> str("");
    // show params for 4 of the NUM_GRAINS grains
    for (size_t line=0; line<4; line++) {
      size_t g = (size_t)((NUM_GRAINS-1) * ((float)line/3));  // ( 0, 1/3, 2/3,.. NUM_GRAINS-1)
      auto& grain = grains[g];
      str.Clear();
      str.AppendFloat(grain.GetStartP(), 3);
      str.Append(" ");
      str.AppendFloat(grain.GetCurrentP(), 3);
      str.Append(" ");
      str.AppendFloat(grain.GetEndP(), 3);
      strs.push_back(string(str));
    }

  }

  // for (size_t c=0; c<4; c++) {
  //   str.Clear();
  //   str.AppendFloat(stable_ctrl_values[c], 3);
  //   strs.push_back(string(str));
  // }


  DisplayLines(strs);
  hw.display.Update();
}

int main(void) {
  hw.Init();

  hw.SetAudioBlockSize(64); // number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  // sample start
  ctrls[0].Init(hw.controls[0], 0.f, 1.0f, Parameter::LINEAR);
  // sample start spread
  ctrls[1].Init(hw.controls[1], 0.f, 1.0f, Parameter::LINEAR);
  // sample length
  ctrls[2].Init(hw.controls[2], 0.f, 1.0f, Parameter::EXPONENTIAL);
  // sample length spread
  ctrls[3].Init(hw.controls[3], 0.f, 1.0f, Parameter::LINEAR);


  output_mixer.Init();
  output_mixer.SetCurve(CROSSFADE_CPOW);
  output_mixer.SetPos(0.5);

  while(true) {
    for (size_t i = 0; i < 100; i++) {
      UpdateControls();
    }
    UpdateDisplay();
  }

}

