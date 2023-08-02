#include <string>

#include "daisy_patch.h"
#include "daisysp.h"
#include "Dynamics/crossfade.h"

using namespace std;
using namespace daisy;
using namespace daisysp;

DaisyPatch hw;

const size_t BUFFER_SIZE = 48000;
float DSY_SDRAM_BSS buffer[BUFFER_SIZE];

size_t sample_offset;   // where in the sample we are playing/recording
size_t sample_start;    // effective sample start; first +ve zero crossing
size_t sample_end;      // effective sample start; last +ve zero crossing
size_t sample_length;   // actual recorded sample length

enum State {
  WAITING,
  RECORDING,
  PLAYING,
};
State state = State::WAITING;

bool looping;
bool have_recording = false;

void SetSampleStart() {
  // decide effective sample start by seeking forward
  // from 0 to first positive crossing
  float last_val = buffer[0];
  for (size_t b = 1; b < BUFFER_SIZE; b++) {
    float value = buffer[b];
    if (last_val < 0 && value > 0) {
      sample_start = b;
      return;
    }
    last_val = value;
  }
  // failed to find a crossing :()
  sample_start = 0;
}

void SetSampleEnd() {
  // decide effective sample end by seeking backwards
  // from end to find "first" positive crossing
  float last_val = buffer[sample_length-1];
  for (size_t b = sample_length-2; b > 0; b--) {
    float value = buffer[b];
    if (last_val > 0 && value < 0) {
      sample_end = b;
      return;
    }
    last_val = value;
  }
  // failed to find a crossing :()
  sample_end = sample_length-1;
}

void StopRecording() {
  sample_length = sample_offset-1;
  SetSampleStart();
  SetSampleEnd();
  sample_offset = sample_start;
  state = looping ? PLAYING : WAITING;
  have_recording = true;
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
        buffer[sample_offset] = in[0][b];
        sample_offset++;
        if (sample_offset == BUFFER_SIZE) {
          StopRecording();
        }
        break;

      case PLAYING:
        // TODO: when ramping up first playing (i.e. not looping etc)
        //   we should cross fade in from whatever other signal e.g. in[0]
        out[0][b] = buffer[sample_offset];
        sample_offset++;
        if (sample_offset == sample_end) {
          state = looping ? PLAYING : WAITING;
          sample_offset = sample_start;
        }
        break;
    }

    // core wave based on effective sample length
    float core = float(sample_offset) / (sample_end-sample_start);
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
  str.Append("start  ");
  str.AppendInt(sample_start);
  strs.push_back(string(str));

  str.Clear();
  str.Append("end    ");
  str.AppendInt(sample_end);
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


