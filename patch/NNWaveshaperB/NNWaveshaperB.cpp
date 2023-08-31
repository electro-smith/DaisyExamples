#include <string>

#include "arm_math.h"
//#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

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

  // see CMSIS/DSP/Examples/ARM/arm_matrix_example/arm_matrix_example_f32.c
  // https://github.com/electro-smith/libDaisy/blob/ae9b45e2927aafba5f261f2ff36e3f41ae74d019/Drivers/CMSIS/DSP/Examples/ARM/arm_matrix_example/arm_matrix_example_f32.c

  // from ..../Drivers/CMSIS/DSP/Examples/ARM/arm_matrix_example/arm_matrix_example_f32.c

  // float_t A_f32[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };  // (4, 2)
  // float_t B_f32[6] = { 0, 1, 2, 3, 4, 5 };        // (2, 3)
  // float_t AB_f32[12];                             // (4, 3)

  // arm_matrix_instance_f32 A;
  // arm_mat_init_f32(&A, 4, 2, (float32_t *)A_f32);

  // arm_matrix_instance_f32 B;
  // arm_mat_init_f32(&B, 2, 3, (float32_t *)B_f32);

  // arm_matrix_instance_f32 AB;
  // arm_mat_init_f32(&AB, 4, 3, (float32_t *)AB_f32);

  // arm_status status;
  // status = arm_mat_mult_f32(&A, &B, &AB);

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

