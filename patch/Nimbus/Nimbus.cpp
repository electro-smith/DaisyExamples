//include "daisy_patch.h"
//#include "daisysp.h"
#include "cv_scaler.h"
//#include "codec.h"
//#include "debug_pin.h"
//#include "debug_port.h"
//#include "system.h"
//#include "version.h"
#include "granular_processor.h"
#include "meter.h"
#include "resources.h"
#include "settings.h"
#include "ui.h"

// #define PROFILE_INTERRUPT 1

using namespace clouds;
using namespace stmlib;

//using namespace daisy;
//using namespace daisysp;


GranularProcessor processor;
//Codec codec;
//DebugPort debug_port;
CvScaler cv_scaler;
Meter meter;
Settings settings;
Ui ui;
//DaisyPatch hw;

// Pre-allocate big blocks in main memory and CCM. No malloc here.
uint8_t block_mem[118784];
uint8_t block_ccm[65536 - 128] __attribute__ ((section (".ccmdata")));

int __errno;

// Default interrupt handlers.
extern "C" {

void NMI_Handler() { }
void HardFault_Handler() { while (1); }
void MemManage_Handler() { while (1); }
void BusFault_Handler() { while (1); }
void UsageFault_Handler() { while (1); }
void SVC_Handler() { }
void DebugMon_Handler() { }
void PendSV_Handler() { }

}

extern "C" {

void SysTick_Handler() {
  ui.Poll();
  if (settings.freshly_baked()) {
    if (debug_port.readable()) {
      uint8_t command = debug_port.Read();
      uint8_t response = ui.HandleFactoryTestingRequest(command);
      debug_port.Write(response);
    }
  }
}

}

void FillBuffer(Codec::Frame* input, Codec::Frame* output, size_t n) {
#ifdef PROFILE_INTERRUPT
  TIC
#endif  // PROFILE_INTERRUPT
  cv_scaler.Read(processor.mutable_parameters());
  processor.Process((ShortFrame*)input, (ShortFrame*)output, n);
  meter.Process(processor.parameters().freeze ? output : input, n);
#ifdef PROFILE_INTERRUPT
  TOC
#endif  // PROFILE_INTERRUPT
}

void AudioCallback(float **in, float **out, size_t size)
{
	//hw.ProcessAllControls();
	for (size_t i = 0; i < size; i++)
	{
		out[0][i] = in[0][i];
		out[1][i] = in[1][i];
		out[2][i] = in[2][i];
		out[3][i] = in[3][i];
	}
}

void Init() {
  System sys;
  Version version;

  sys.Init(true);
  version.Init();

  // Init granular processor.
  processor.Init(
      block_mem, sizeof(block_mem),
      block_ccm, sizeof(block_ccm));

  settings.Init();
  cv_scaler.Init(settings.mutable_calibration_data());
  //meter.Init(32000);
  //ui.Init(&settings, &cv_scaler, &processor, &meter);

  bool master = !version.revised();
  if (!codec.Init(master, 32000)) {
    ui.Panic();
  }
  if (!codec.Start(32, &FillBuffer)) {
    ui.Panic();
  }
  if (settings.freshly_baked()) {
#ifdef PROFILE_INTERRUPT
    DebugPin::Init();
#else
    debug_port.Init();
#endif  // PROFILE_INTERRUPT
  }
  sys.StartTimers();
}

int main(void) {
  Init();

	//hw.Init();
	//hw.StartAdc();
	//hw.StartAudio(AudioCallback);

  while (1) {
    //ui.DoEvents();
    processor.Prepare();
  }
}
