#include <string>

#include "daisy_patch.h"
#include "daisysp.h"
#include "Dynamics/crossfade.h"

using namespace std;
using namespace daisy;
using namespace daisysp;

DaisyPatch hw;

// the fundamental unit of a grain is a block of audio
// corresponding to the block size of the audio callback
const size_t BLOCK_SIZE = 64;

// a float block represents a block of floats for one callback
using FloatBlock = float[BLOCK_SIZE];

// max sample length
const size_t MAX_SAMPLE_LEN = 5000;
using SampleBuffer = FloatBlock[MAX_SAMPLE_LEN];

// SDRAM assigned sample buffer
SampleBuffer DSY_SDRAM_BSS buffer;

size_t sample_offset;
size_t sample_length;

enum State {
  WAITING,
  RECORDING,
  PLAYING,
};
State state = State::WAITING;

bool looping;

bool have_recording = false;

CrossFade fader;

void StopRecording() {
  sample_length = sample_offset-1;
  sample_offset = 0;
  state = looping ? PLAYING : WAITING;
  have_recording = true;

  // rewrite last block as linear interp to sample start
  float from_val = buffer[sample_length-1][0];
  float to_val = buffer[0][0];
  for (size_t b = 0; b < BLOCK_SIZE; b++) {
    fader.SetPos(float(b) / BLOCK_SIZE);
    buffer[sample_length-1][b] = fader.Process(from_val, to_val);
  }

}


void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {

  switch(state) {

    case WAITING:
      for (size_t b = 0; b < BLOCK_SIZE; b++) {
        out[0][b] = in[0][b];
      }
      break;

    case RECORDING:
      for (size_t b = 0; b < BLOCK_SIZE; b++) {
        out[0][b] = in[0][b];
        buffer[sample_offset][b] = in[0][b];
      }
      sample_offset++;
      if (sample_offset == MAX_SAMPLE_LEN) {
        StopRecording();
      }
      break;

    case PLAYING:
      for (size_t b = 0; b < BLOCK_SIZE; b++) {
        out[0][b] = buffer[sample_offset][b];
      }
      sample_offset++;
      if (sample_offset == sample_length) {
        state = looping ? PLAYING : WAITING;
        sample_offset = 0;
      }
      break;
  }

  // core wave based on sample length
  float core = float(sample_offset) / sample_length;
  for (size_t b = 0; b < BLOCK_SIZE; b++) {
    out[1][b] = core;
  }

}


void UpdateControls() {
  hw.ProcessAllControls();

  // ctrl1 decides whether we loop sample or not
  bool new_looping = hw.controls[0].Value() > 0.5;
  if (!looping && new_looping && state==WAITING && have_recording) {
    state = PLAYING;
  }
  looping = new_looping;

  // click on encoder
  if (hw.encoder.RisingEdge()) {
    if (state == RECORDING) {
      StopRecording();
    } else {
      state = RECORDING;
      sample_offset = 0;
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

  str.Clear();
  str.Append("looping ");
  str.Append(looping ? "T" : "F");
  strs.push_back(string(str));

  str.Clear();
  str.Append("offset ");
  str.AppendInt(sample_offset);
  strs.push_back(string(str));

  str.Clear();
  str.Append("length ");
  str.AppendInt(sample_length);
  strs.push_back(string(str));

  DisplayLines(strs);
  hw.display.Update();
}

int main(void) {
  hw.Init();

  hw.SetAudioBlockSize(BLOCK_SIZE); // number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  fader.Init();
  fader.SetCurve(CROSSFADE_CPOW);

  while(true) {
    for (size_t i = 0; i < 100; i++) {
      UpdateControls();
    }
    UpdateDisplay();
  }

}


