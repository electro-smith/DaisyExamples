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
  hw.SetAudioBlockSize(32);
  float sample_rate = hw.AudioSampleRate();

  //init the luts
  InitResources(sample_rate);

  processor.Init(
      sample_rate, block_mem, sizeof(block_mem),
      block_ccm, sizeof(block_ccm));

  Parameters* parameters = processor.mutable_parameters();
  
  processor.set_playback_mode(PLAYBACK_MODE_GRANULAR);
  parameters->dry_wet = 1.f;

  parameters->texture = .5f;
  processor.set_low_fidelity(false);
  processor.set_num_channels(2);

	hw.StartAdc();
	hw.StartAudio(AudioCallback);
  while (1) {
    hw.ProcessAllControls();

    processor.Prepare();
    parameters->density = hw.controls[0].Process();
    parameters->size = hw.controls[1].Process();
    parameters->feedback = hw.controls[2].Process();
    parameters->position = hw.controls[3].Process();
    int32_t q = processor.quality();
    processor.set_quality(q);
  }
}
