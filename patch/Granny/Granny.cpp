#include "daisy_patch.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;
using namespace std;

DaisyPatch patch;
Parameter ctrls[4];

// the fundamental unit of a grain is a block of audio
// corresponding to the block size of the audio callback
const size_t BLOCK_SIZE = 64;

// we deal with single channel and stereo pairs of blocks
// both as floats (from the daisy) and int16 compressed versions
// stored by the grains
using FloatBlock = float[BLOCK_SIZE];
using FloatBlockPair = FloatBlock[2];
using Int16Block = int16_t[BLOCK_SIZE];
using Int16BlockPair = Int16Block[2];

// each grain has, at most, some number of blocks
const size_t MAX_GRAIN_BLOCKS = 800;  // ~2secs for block size 256
using GrainBuffer = Int16Block[MAX_GRAIN_BLOCKS];

// and overall we maintain some max number of recording/delayed/playing
// grains. the memory for grains is stored in SDRAM, allocated once
// and split up between grains on Grain creation.
const size_t MAX_GRAINS = 500;
static GrainBuffer DSY_SDRAM_BSS global_buffer[MAX_GRAINS];

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
  // TODO use daisy::Random::GetFloat(); // which defaults to (0, 1)
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

// // don't update display constantly
const int DISPLAY_UPDATE_FREQ = 100000;

// number of grains queued to start
size_t start_new_grains = 0;

// regeneration probability; 0 to 1, set by control
float regen_prob;

bool regenerate() {
  if (regen_prob < 0.02) {
    return false;
  }
  if (regen_prob > 0.98) {
    return true;
  }
  return random_01() < regen_prob;
}

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
      record_channel_ = random_01() ? 0 : 1;
      playback_channel_ = random_01() ? 0 : 1;
    }

    void Record(const Int16BlockPair& input) {
      copy_n(input[record_channel_], BLOCK_SIZE, (*buffer_)[idx_]);

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

    void Play(FloatBlockPair& stereo_out, size_t* channel) {
      for (size_t b = 0; b < BLOCK_SIZE; b++) {
        const int16_t int16_value = (*buffer_)[idx_][b];
        stereo_out[playback_channel_][b] += s162f(int16_value);
      }
      *channel = playback_channel_;
      idx_++;
      if (idx_ == grain_size_) {
        if (regenerate()) {
          state_ = DELAYED;
          delay_time_ = random_average_spread(delay_time);
        } else {
          state_ = FREE;
        }
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
    size_t record_channel_;    // {0, 1}
    size_t playback_channel_;  // {0, 1}
};



Grain grains[MAX_GRAINS];

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t _size) {

  // prepare buffer for grains to write to
  FloatBlockPair output_buffer;
  for (size_t c = 0; c < 2; c++) {
    for (size_t b = 0; b < BLOCK_SIZE; b++) {
      output_buffer[c][b] = 0.f;
    }
  }
  int num_grains_playing[2] = {0, 0};

  // buffer for converted input
  // lazily evaluated for the case of no grains playing
  Int16BlockPair in_as_int;
  bool input_converted = false;

  // process each grain
  for (auto& grain : grains) {
    switch(grain.GetState()) {
      case Grain::FREE:
        if (start_new_grains > 0) {
          grain.StartRecording();
          start_new_grains--;
        }
        break;
      case Grain::RECORDING:
        if (!input_converted) {
          for (size_t c = 0; c < 2; c++) {
            for (size_t b = 0; b < BLOCK_SIZE; b++) {
              in_as_int[c][b] = f2s16(in[c][b]);
            }
          }
          input_converted = true;
        }
        grain.Record(in_as_int);
        break;
      case Grain::DELAYED:
        grain.TickDownDelay();
        break;
      case Grain::PLAYING:
        size_t channel;
        grain.Play(output_buffer, &channel);
        num_grains_playing[channel]++;
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

  // write output; mirror stereo in1/2 to out1/2
  copy_n(in[0], BLOCK_SIZE, out[0]);
  copy_n(in[1], BLOCK_SIZE, out[1]);
  // write output of grains
  copy_n(output_buffer[0], BLOCK_SIZE, out[2]);
  copy_n(output_buffer[1], BLOCK_SIZE, out[3]);

}

void ProcessControls() {
  patch.ProcessAnalogControls();
  patch.ProcessDigitalControls();

  grain_size.average = ctrls[0].Process();
  grain_size.spread = grain_size.average / 2;
  delay_time.average = ctrls[1].Process();
  delay_time.spread = delay_time.average / 2;
  regen_prob = ctrls[2].Process();

  if (start_new_grains < MAX_GRAINS && patch.gate_input[0].Trig()) {
    start_new_grains++;
  }

}

void DisplayLines(const vector<string> &strs) {
  int line_num=0;
  for (string str : strs) {
    char* cstr = &str[0];
    patch.display.SetCursor(0, line_num*10);
    patch.display.WriteString(cstr, Font_7x10, true);
    line_num++;
  }
}

void UpdateDisplay() {
  size_t n_free = 0;
  size_t n_recording = 0;
  size_t n_delayed = 0;
  size_t n_playing = 0;
  for (auto& grain : grains) {
    switch(grain.GetState()) {
      case Grain::FREE:
        n_free++;
        break;
      case Grain::RECORDING:
        n_recording++;
        break;
      case Grain::DELAYED:
        n_delayed++;
        break;
      case Grain::PLAYING:
        n_playing++;
        break;
    }
  }

  patch.display.Fill(false);
  vector<string> strs;
  strs.push_back(" f" + to_string(n_free)
                 + " r" + to_string(n_recording)
                 + " d" + to_string(n_delayed)
                 + " p" + to_string(n_playing));
  strs.push_back("size  " + average_spread_to_str(grain_size));
  strs.push_back("delay " + average_spread_to_str(delay_time));

  FixedCapStr<20> str("");
  str.AppendFloat(regen_prob, 2);
  strs.push_back("regen " + string(str));

  DisplayLines(strs);
  patch.display.Update();
}

int main(void) {

  for (size_t g = 0; g < MAX_GRAINS; g++) {
    grains[g].SetBuffer(&global_buffer[g]);
  }

  patch.Init();

  // grain size control
  //  average; 0.1s to 3s
  //  spread;  +/- 2s
  // => max size 5s
  ctrls[0].Init(patch.controls[0], 0.1f, 3.f, Parameter::LINEAR);

  // grain playback delay control
  //  average; 0s to 5s
  //  spread; +/- 1/2 the average
  ctrls[1].Init(patch.controls[1], 0.f, 5.f, Parameter::LINEAR);

  // grain regenerate prob
  ctrls[2].Init(patch.controls[2], 0.f, 1.f, Parameter::LINEAR);

  patch.SetAudioBlockSize(BLOCK_SIZE);
  patch.StartAdc();
  patch.StartAudio(AudioCallback);

  while(true) {
    for (size_t i=0; i<DISPLAY_UPDATE_FREQ; i++) {
      ProcessControls();
    }
    UpdateDisplay();
  }

}
