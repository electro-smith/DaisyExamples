#include "daisy_patch.h"
#include "daisysp.h"
//#include "cv_scaler.h"
//#include "codec.h"
//#include "debug_pin.h"
//#include "debug_port.h"
//#include "system.h"
//#include "version.h"
#include "granular_processor.h"
//#include "meter.h"
#include "resources.h"
//#include "settings.h"
//#include "ui.h"

// #define PROFILE_INTERRUPT 1

using namespace daisysp;
using namespace daisy;

GranularProcessorClouds processor;
//Codec codec;
//DebugPort debug_port;
//CvScaler cv_scaler;
//Meter meter;
//Settings settings;
//Ui ui;
DaisyPatch hw;

// Pre-allocate big blocks in main memory and CCM. No malloc here.
uint8_t block_mem[118784];
uint8_t block_ccm[65536 - 128] __attribute__ ((section (".ccmdata")));

//int __errno;

// Default interrupt handlers.
extern "C" {

void NMI_Handler() { }
//void HardFault_Handler() { while (1); }
void MemManage_Handler() { while (1); }
void BusFault_Handler() { while (1); }
void UsageFault_Handler() { while (1); }
void SVC_Handler() { }
void DebugMon_Handler() { }
void PendSV_Handler() { }

}

extern "C" {

// void SysTick_Handler() {
//   ui.Poll();
//   if (settings.freshly_baked()) {
//     if (debug_port.readable()) {
//       uint8_t command = debug_port.Read();
//       uint8_t response = ui.HandleFactoryTestingRequest(command);
//       debug_port.Write(response);
//     }
//   }
// }

}

void AudioCallback(float **in, float **out, size_t size)
{
  //cv_scaler.Read(processor.mutable_parameters());
  //meter.Process(processor.parameters().freeze ? output : input, n);

	hw.ProcessAllControls();

  FloatFrame input[size];
  FloatFrame output[size];

  for(size_t i = 0; i < size; i++){
    input[i].l = in[0][i];
    input[i].r = in[1][i];
    output[i].l = output[i].r = 0.f;
  }

  processor.Process(input, output, size);

	for (size_t i = 0; i < size; i++)
	{
		out[0][i] = output[i].l;
		out[1][i] = output[i].r;
		out[2][i] = out[3][i] = 0.f;
	}
}

void Init(float sample_rate) {
  //System sys;
  //Version version;

  //sys.Init(true);
  //version.Init();

  // Init granular processor.
  processor.Init(
      sample_rate, block_mem, sizeof(block_mem),
      block_ccm, sizeof(block_ccm));

  //settings.Init();
  //cv_scaler.Init(settings.mutable_calibration_data());
  //meter.Init(32000);
  //ui.Init(&settings, &cv_scaler, &processor, &meter);
}

int main(void) {

	hw.Init();

  Init(hw.AudioSampleRate());

	hw.StartAdc();
	hw.StartAudio(AudioCallback);
  while (1) {
    //ui.DoEvents();
    processor.Prepare();
  }
}
