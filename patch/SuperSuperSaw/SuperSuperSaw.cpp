#include "daisy_patch.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPatch hw;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
  hw.ProcessAllControls();
  for (size_t i = 0; i < size; i++) {
    out[0][i] = in[0][i];
    out[1][i] = in[1][i];
    out[2][i] = in[2][i];
    out[3][i] = in[3][i];
  }
}

int main(void) {
  hw.Init();
  hw.SetAudioBlockSize(4); // number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  hw.StartAdc();
  hw.StartAudio(AudioCallback);
  while(1) {}
}
