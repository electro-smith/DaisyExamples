#include "daisy_patch.h"
#include "daisysp.h"
#include "Dynamics/crossfade.h"

#include <string>

using namespace daisy;
using namespace daisysp;
using namespace std;

DaisyPatch patch;

// the fundamental unit of a grain is a block of audio
// corresponding to the block size of the audio callback
const size_t BLOCK_SIZE = 64;

// a float block represents a block of floats for one callback
using FloatBlock = float[BLOCK_SIZE];

// max loop length in blocks
//  tested this and current value is _at least_ x7 the 16 step of BSP
//  i.e. could lower to, say, 6000 without fear of it getting full.
//  TODO: test this!
const size_t MAX_LOOP_LEN = 60000;
using LooperBuffer = FloatBlock[MAX_LOOP_LEN];

// SDRAM assigned looper buffers
LooperBuffer DSY_SDRAM_BSS global_looper_buffer[4];

// Single shared 50/50 cross fader for mixing sound on sound
CrossFade sound_on_sound_mix;

namespace mat {
class Looper {
  public:
    enum State {
      NO_LOOP,
      RECORDING,
      PLAYING,
      FULL
    };

    Looper() : state_(NO_LOOP) {}

    void Init(const int c) {
      mix_param_.Init(patch.controls[c], 0.0f, 1.0f, Parameter::LINEAR);
      mix_crossfade_.Init();
      mix_crossfade_.SetCurve(CROSSFADE_CPOW);
      loop_buffer_ = &global_looper_buffer[c];
    }

    void ProcessControl() {
      mix_amount_ = mix_param_.Process();
      if (mix_amount_ < 0.02f) {
        mix_amount_ = 0.0f;
      } else if (mix_amount_ > 0.98f) {
        mix_amount_ = 1.0f;
      }
      mix_crossfade_.SetPos(mix_amount_);
    }

    const string DisplayMixAmount() {
      return to_string(static_cast<uint32_t>(mix_amount_*100));
    }

    const string StateStr() {
      switch (state_) {
        case NO_LOOP:
          return "N";
        case RECORDING:
          return sound_on_sound_ ? "S" : "R";
        case PLAYING:
          return "P";
        case FULL:
          return "F";
      }
      return "?";
    }

    void Trig(bool record_next_loop) {

      // reset to start of loop regardless of state change
      idx_ = 0;

      switch(state_) {

        case NO_LOOP:
          if (record_next_loop) {
            state_ = RECORDING;
            sound_on_sound_ = false;
          }
          break;

        case RECORDING:
        case FULL:
          state_ = PLAYING;
          break;

        case PLAYING:
          // if we decide to record next loop only do so depending
          // on mix amount, which also dicates whether we are replacing
          // recording or doing sound on sound mixing.
          if (record_next_loop) {
            if (mix_amount_ < 0.33) {
              state_ = RECORDING;
              sound_on_sound_ = false;
            } else if (mix_amount_ < 0.66) {
              state_ = RECORDING;
              sound_on_sound_ = true;
            }
          }
          // else ignore request to record next and keep playing
          break;
      }

    }

    void Process(const float* input, float* output) {
      switch(state_) {

        case NO_LOOP:
          copy_n(input, BLOCK_SIZE, output);
          break;

        case RECORDING:
          if (sound_on_sound_) {
            // sound on sound => mix 50/50 with live
            for (size_t i = 0; i < BLOCK_SIZE; i++) {
              float live_val = input[i];
              float loop_val = (*loop_buffer_)[idx_][i];
              float mixed = sound_on_sound_mix.Process(live_val, loop_val);
              output[i] = mixed;
              (*loop_buffer_)[idx_][i] = mixed;
            }
          } else {
            // not sound on sound => fully replace buffer
            copy_n(input, BLOCK_SIZE, output);
            copy_n(input, BLOCK_SIZE, (*loop_buffer_)[idx_]);
          }
          break;

        case PLAYING:
          // cross fade playback
          for (size_t i = 0; i < BLOCK_SIZE; i++) {
            float live_val = input[i];
            float loop_val = (*loop_buffer_)[idx_][i];
            output[i] = mix_crossfade_.Process(live_val, loop_val);
          }
	  break;

        case FULL:
          for (size_t i = 0; i < BLOCK_SIZE; i++) {
            output[i] = 0;
          }
          break;
      }

      idx_++;
      if (idx_ == MAX_LOOP_LEN) {
        state_ = FULL;
        idx_ = 0;
      }

    }

  private:
    State state_;
    bool sound_on_sound_;
    float mix_amount_;
    Parameter mix_param_;
    CrossFade mix_crossfade_;
    size_t idx_;
    LooperBuffer* loop_buffer_;
};
}

mat::Looper loopers[4];

// simple state machine for looping
bool record_next_loop = false;

void UpdateControls() {
  patch.ProcessAnalogControls();
  patch.ProcessDigitalControls();

  for (auto& looper : loopers) {
    looper.ProcessControl();
  }

  // check for notice to record next loop; either encoder click or second gate
  if (patch.encoder.RisingEdge() || patch.gate_input[1].Trig()) {
    record_next_loop = true;
  }

  // on clock decide how to transition
  if (patch.gate_input[0].Trig()) {
    for (auto& looper : loopers) {
      looper.Trig(record_next_loop);
    }
    record_next_loop = false;
  }

}

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {
  for (size_t c = 0; c < 4; c++) {
    loopers[c].Process(in[c], out[c]);
  }
}

void DisplayLines(const vector<string> &strs) {
  int line_num = 0;
  for (string str : strs) {
    char* cstr = &str[0];
    patch.display.SetCursor(0, line_num*10);
    patch.display.WriteString(cstr, Font_7x10, true);
    line_num++;
  }
}

void UpdateDisplay() {
  patch.display.Fill(false);

  vector<string> strs;
  for (size_t c = 0; c < 4; c++) {
    string info_str = to_string(c+1);
    info_str += " " + loopers[c].StateStr();
    info_str += " " + loopers[c].DisplayMixAmount();
    strs.push_back(info_str);
  }
  DisplayLines(strs);
  patch.display.Update();
}

int main(void) {
  patch.Init();
  patch.SetAudioBlockSize(BLOCK_SIZE);
  patch.StartAdc();
  patch.StartAudio(AudioCallback);

  for (size_t c = 0; c < 4; c++) {
    loopers[c].Init(c);
  }

  sound_on_sound_mix.Init();
  sound_on_sound_mix.SetCurve(CROSSFADE_CPOW);
  sound_on_sound_mix.SetPos(0.5);

  while(true) {
    UpdateControls();
    UpdateDisplay();
  }

}
