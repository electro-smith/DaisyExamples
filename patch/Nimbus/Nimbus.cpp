#include "daisy_patch.h"
#include "daisysp.h"
#include "granular_processor.h"

using namespace daisysp;
using namespace daisy;

GranularProcessorClouds processor;
DaisyPatch hw;

// Pre-allocate big blocks in main memory and CCM. No malloc here.
uint8_t block_mem[118784];
uint8_t block_ccm[65536 - 128];

void AudioCallback(float **in, float **out, size_t size)
{
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

int main(void) {
	hw.Init();
  float sample_rate = hw.AudioSampleRate();

  //init the luts
  InitResources(sample_rate);

  processor.Init(
      sample_rate, block_mem, sizeof(block_mem),
      block_ccm, sizeof(block_ccm));

  Parameters* parameters = processor.mutable_parameters();
  
  processor.set_bypass(false);
  processor.set_silence(false);
  processor.set_playback_mode(PLAYBACK_MODE_GRANULAR);
  parameters->dry_wet = 1.f;
  parameters->pitch = 12.f;
  processor.set_freeze(false);
  processor.set_num_channels(2);

  parameters->granular.overlap = .5f;
  parameters->granular.window_shape = .5f;
  parameters->granular.use_deterministic_seed = true;
  parameters->granular.stereo_spread = .5f;
  parameters->texture = .5f;
  

	hw.StartAdc();
	hw.StartAudio(AudioCallback);
  while (1) {
    hw.ProcessAllControls();

    processor.Prepare();
    parameters->density = hw.controls[0].Process();
    parameters->size = hw.controls[1].Process();
    parameters->feedback = hw.controls[2].Process();
    parameters->position = hw.controls[3].Process();
  }
}
