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

char paramNames[10][15];

int mymod(int a, int b){
  return (b + (a%b)) % b;
}

class ParamControl{
  public:
    ParamControl() {}
    ~ParamControl() {}

    void Init(AnalogControl* control, Parameters* params){
      params_ = params;
      control_ = control;
      param_num_ = 0;
      param_max_ = 9;
    }

    void incParamNum(int inc){
      param_num_ += inc;
      param_num_ = mymod(param_num_, param_max_);
    }

    char* getName(){
      return paramNames[param_num_];
    }

    void Process(){
      float val = control_->Process();
      switch(param_num_){
        case 0:
          params_->position = val;
          break;
        case 1:
          params_->size = val;
          break;
        case 2:
          params_->pitch = val;
          break;
        case 3:
          params_->density = val;
          break;
        case 4:
          params_->texture = val;
          break;
        case 5:
          params_->dry_wet = val;
          break;
        case 6:
          params_->stereo_spread = val;
          break;
        case 7:
          params_->feedback = val;
          break;
        case 8:
          params_->reverb = val;
          break;
          

      }
    }

  private:
    AnalogControl* control_;
    Parameters* params_;
    int param_num_;
    int param_max_;
};

ParamControl paramControls[4];

int menupos;
bool selected;

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

//set up the param names for the oled code
void InitStrings(){
  sprintf(paramNames[0], "position");
  sprintf(paramNames[1], "size");
  sprintf(paramNames[2], "pitch");
  sprintf(paramNames[3], "density");
  sprintf(paramNames[4], "texture");
  sprintf(paramNames[5], "dry_wet");
  sprintf(paramNames[6], "stereo_spread");
  sprintf(paramNames[7], "feedback");
  sprintf(paramNames[8], "reverb");
}

int main(void) {
	hw.Init();
  hw.SetAudioBlockSize(32); // clouds won't work with blocks bigger than 32
  float sample_rate = hw.AudioSampleRate();

  //init the luts
  InitResources(sample_rate);

  processor.Init(
      sample_rate, block_mem, sizeof(block_mem),
      block_ccm, sizeof(block_ccm));
  processor.set_playback_mode(PLAYBACK_MODE_GRANULAR);

  Parameters* parameters = processor.mutable_parameters();  

  InitStrings();

  for(int i = 0; i < 4; i++){
    paramControls[i].Init(&hw.controls[i], parameters);
  }

	hw.StartAdc();
	hw.StartAudio(AudioCallback);
  while (1) {
    processor.Prepare();
    
    //controls and menu
    hw.ProcessAllControls();
    for(int i = 0; i < 4; i++){
      paramControls[i].Process();
    }


    //encoder press
    selected ^= hw.encoder.RisingEdge();

    //encoder turn
    if(selected){
      paramControls[menupos].incParamNum(hw.encoder.Increment());
    }
    else{
      menupos += hw.encoder.Increment();
      menupos = mymod(menupos, 4);
    }

    //oled
    hw.display.Fill(false);

    for(int i = 0; i < 4; i++){
      hw.display.SetCursor(0,i*13);
      hw.display.WriteString(paramControls[i].getName(), Font_7x10, true);
    };

    hw.display.Update();
  }
}
