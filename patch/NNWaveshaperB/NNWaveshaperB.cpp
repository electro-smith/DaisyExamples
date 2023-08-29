#include <string>

#include "daisy_patch.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;
using namespace std;

DaisyPatch hw;
CpuLoadMeter cpu_load_meter;

size_t n = 0;

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {

  cpu_load_meter.OnBlockStart();

  for (size_t i = 0; i < size; i++) {
    out[0][i] = in[0][i];
    out[1][i] = in[1][i];
  }

  cpu_load_meter.OnBlockEnd();

  n++;
}

void UpdateDisplay() {
  // just serial for now
  FixedCapStr<50> str("n ");
  str.AppendInt(n);
  str.Append(" cpu ");
  str.AppendFloat(cpu_load_meter.GetAvgCpuLoad(), 5);
  hw.seed.PrintLine(str);
}


int main(void) {
  hw.Init();
  hw.SetAudioBlockSize(64); // number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  hw.StartAdc();

  hw.seed.StartLog();
  cpu_load_meter.Init(hw.AudioSampleRate(), hw.AudioBlockSize());

  hw.StartAudio(AudioCallback);

  while(1) {
    hw.ProcessAllControls();
    UpdateDisplay();
  }

}

