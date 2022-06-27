#include "daisy_patch.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;
using namespace std;

// encoder click -> record next loop
// gate_0 -> loop reset
// in1 / out1 ; loop 1
// in2 / out2 ; loop 2

DaisyPatch patch;
Parameter grain_ctrls[2];

// the fundamental unit of a grain is a block of audio
// corresponding to the block size of the audio callback
const size_t BLOCK_SIZE = 64;

// a float block represents a block of floats for one callback
using FloatBlock = float[BLOCK_SIZE];
using FloatBlockPair = FloatBlock[2];

// max loop length in blocks
const size_t MAX_LOOP_LEN = 5000;
using LooperBuffer = FloatBlock[MAX_LOOP_LEN];

// SDRAM assigned looper buffers. we loop on first two channels
LooperBuffer DSY_SDRAM_BSS global_looper_buffer[2];

// each grain has, at most, some number of blocks
const size_t MAX_GRAIN_BLOCKS = 600;  // ~2secs for block size 256
using GrainBuffer = FloatBlock[MAX_GRAIN_BLOCKS];

// and overall we maintain some max number of recording/delayed/playing
// grains. the memory for grains is stored in SDRAM, allocated once
// and split up between grains on Grain creation.
const size_t MAX_GRAINS = 400;
static GrainBuffer DSY_SDRAM_BSS global_grain_buffer[MAX_GRAINS];

// Single shared 50/50 cross fader for mixing sound on sound
CrossFade sound_on_sound_mix;

struct AverageSpread {
  // in seconds;
  float average;
  float spread;
};
AverageSpread grain_size;
AverageSpread delay_time;

string average_spread_to_str(const AverageSpread &as) {
  FixedCapStr<20> str("");
  str.AppendFloat(as.average, 1);
  str.Append(" +/- ");
  str.AppendFloat(as.spread, 1);
  return string(str);
}

const inline float random_01() {
  return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

const size_t random_average_spread(const AverageSpread &average_spread) {
  float average = average_spread.average;
  float spread = average_spread.spread;
  if (average <= 0.01 && spread <= 0.01) { return 1; }
  float min_v = average - spread;
  if (min_v < 0) { min_v = 1; }
  float max_v = average + spread;
  float diff = max_v - min_v;
  float random_value = (random_01() * diff) + min_v; // in seconds
  random_value = (random_value * 48000) / BLOCK_SIZE; // converted to blocks
  return static_cast<size_t>(random_value);
}

// number of grains queued to start
size_t start_new_grains = 0;

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

class Grain {
  public:

    enum State {
      FREE,
      RECORDING,
      DELAYED,
      PLAYING
    };

    Grain() : state_(FREE), idx_(0) {}

    void StartRecording() {
      state_ = RECORDING;
      idx_ = 0;
      grain_size_ = random_average_spread(grain_size);
      record_left_ = random_01() < 0.5;
      playback_left_ = random_01() < 0.5;
    }

    void Record(const float* input1, const float* input2) {
      const float* input = record_left_ ? input1 : input2;
      copy_n(input, BLOCK_SIZE, (*buffer_)[idx_]);

      // for first block use linear envelope to ramp up from 0.
      if (idx_ == 0) {
        for (size_t b = 0 ; b < BLOCK_SIZE; b++) {
          const float proportion = static_cast<float>(b) / (BLOCK_SIZE-1);
          (*buffer_)[idx_][b] *= proportion;
        }
      }
      // for last block use linear envelope to ramp down to 0.
      else if (idx_ == grain_size_-1) {
        for (size_t b = 0 ; b < BLOCK_SIZE; b++) {
          const float proportion = static_cast<float>(b) / (BLOCK_SIZE-1);
          (*buffer_)[idx_][b] *= 1.0 - proportion;
        }
      }

      idx_++;
      if (idx_ == grain_size_) {
        state_ = DELAYED;
        delay_time_ = random_average_spread(delay_time);
      }
    }

    void TickDownDelay() {
      delay_time_--;
      if (delay_time_<=0) {
        state_ = PLAYING;
        idx_ = 0;
      }
    }

    void Play(FloatBlockPair& stereo_out, bool* playback_left) {
      for (size_t b = 0; b < BLOCK_SIZE; b++) {
        size_t playback_channel = playback_left_ ? 0 : 1;
        stereo_out[playback_channel][b] += (*buffer_)[idx_][b];
      }
      *playback_left = playback_left_;
      idx_++;
      if (idx_ == grain_size_) {
        state_ = FREE;
      }
    }

    inline State GetState() const { return state_; }
    inline void SetBuffer(GrainBuffer* buffer) { buffer_ = buffer; }

  private:
    State state_;
    GrainBuffer* buffer_;
    size_t idx_;
    size_t grain_size_;
    size_t delay_time_;
    bool record_left_;
    bool playback_left_;
};

}

mat::Looper loopers[2];
mat::Grain grains[MAX_GRAINS];

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {

  // first two channels in/out are looper
  for (size_t c = 0; c < 2; c++) {
    loopers[c].Process(in[c], out[c]);
  }

  // prepare buffer for grains to write to
  FloatBlockPair output_buffer;
  for (size_t c = 0; c < 2; c++) {
    for (size_t b = 0; b < BLOCK_SIZE; b++) {
      output_buffer[c][b] = 0.f;
    }
  }
  int num_grains_playing[2] = {0, 0};
  // process each grain
  for (auto& grain : grains) {
    switch(grain.GetState()) {
      case mat::Grain::FREE:
        if (start_new_grains > 0) {
          grain.StartRecording();
          start_new_grains--;
        }
        break;
      case mat::Grain::RECORDING:
        grain.Record(in[2], in[3]);
        break;
      case mat::Grain::DELAYED:
        grain.TickDownDelay();
        break;
      case mat::Grain::PLAYING:
        bool playback_left;
        grain.Play(output_buffer, &playback_left);
        num_grains_playing[playback_left ? 0 : 1]++;
        break;
    }
  }
  // normalise grain output
  for (size_t c = 0; c < 2; c++) {
    if (num_grains_playing[c] > 1) {
      const float normalisation = 1.0f / sqrt(num_grains_playing[c]);
      for (size_t b = 0; b < BLOCK_SIZE; b++) {
        output_buffer[c][b] *= normalisation;
      }
    }
  }
  // write to output
  copy_n(output_buffer[0], BLOCK_SIZE, out[2]);
  copy_n(output_buffer[1], BLOCK_SIZE, out[3]);

}

// simple state machine for looping
bool record_next_loop = false;

void UpdateControls() {
  patch.ProcessAnalogControls();
  patch.ProcessDigitalControls();

  // looper controls update
  for (auto& looper : loopers) {
    looper.ProcessControl();  // uses ctrl_1 and ctrl_2
  }
  // check for notice to record next loop;
  if (patch.encoder.RisingEdge()) {
    record_next_loop = true;
  }
  // on clock decide how to transition
  if (patch.gate_input[0].Trig()) {
    for (auto& looper : loopers) {
      looper.Trig(record_next_loop);
    }
    record_next_loop = false;
  }

  // grain controls update
  grain_size.average = grain_ctrls[0].Process();  // ctrl_3
  grain_size.spread = grain_size.average / 2;
  delay_time.average = grain_ctrls[1].Process();  // ctrl_4
  delay_time.spread = delay_time.average / 2;
  if (start_new_grains < MAX_GRAINS && patch.gate_input[1].Trig()) {
    start_new_grains++;
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

  // looper state
  for (size_t c = 0; c < 2; c++) {
    string info_str = to_string(c+1);
    info_str += " " + loopers[c].StateStr();
    info_str += " " + loopers[c].DisplayMixAmount();
    strs.push_back(info_str);
  }

  // grain state
  size_t n_free = 0;
  size_t n_recording = 0;
  size_t n_delayed = 0;
  size_t n_playing = 0;
  for (auto& grain : grains) {
    switch(grain.GetState()) {
      case mat::Grain::FREE:
        n_free++;
        break;
      case mat::Grain::RECORDING:
        n_recording++;
        break;
      case mat::Grain::DELAYED:
        n_delayed++;
        break;
      case mat::Grain::PLAYING:
        n_playing++;
        break;
    }
  }
  strs.push_back(" f" + to_string(n_free)
                 + " r" + to_string(n_recording)
                 + " d" + to_string(n_delayed)
                 + " p" + to_string(n_playing));
  strs.push_back("size  " + average_spread_to_str(grain_size));
  strs.push_back("delay " + average_spread_to_str(delay_time));

  DisplayLines(strs);
  patch.display.Update();
}

int main(void) {
  patch.Init();
  patch.SetAudioBlockSize(BLOCK_SIZE);
  patch.StartAdc();
  patch.StartAudio(AudioCallback);

  for (size_t c = 0; c < 2; c++) {
    loopers[c].Init(c);
  }

  sound_on_sound_mix.Init();
  sound_on_sound_mix.SetCurve(CROSSFADE_CPOW);
  sound_on_sound_mix.SetPos(0.5);

  for (size_t g = 0; g < MAX_GRAINS; g++) {
    grains[g].SetBuffer(&global_grain_buffer[g]);
  }

  // grain size control
  //  average; 0.1s to 3s
  //  spread;  +/- 2s
  // => max size 5s
  grain_ctrls[0].Init(patch.controls[2], 0.1f, 3.f, Parameter::LINEAR);

  // grain playback delay control
  //  average; 0s to 5s
  //  spread; +/- 1/2 the average
  grain_ctrls[1].Init(patch.controls[3], 0.f, 5.f, Parameter::LINEAR);

  while(true) {
    for (size_t i = 0; i < 10000; i++) {
      UpdateControls();
    }
    UpdateDisplay();
  }

}