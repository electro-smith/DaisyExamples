#include "daisysp.h"
#include "daisy_pod.h"

const int MAX_SIZE = (int)(48000); //1 mins of floats at 48kHz

using namespace daisysp;
using namespace daisy;

static DaisyPod pod;

bool first = true; //first loop (sets length)
bool rec = false; //currently recording
int pos = 0;
float buf[MAX_SIZE];
int mod = MAX_SIZE;
int size = 0;

void Reset()
{
  for (int i = 0; i < MAX_SIZE; i++)
  {
      buf [i] = 0;
  }
  first = true;
  rec = false;
  size = 0;
  mod = MAX_SIZE;
  pos = 0;
}

static void AudioCallback(float *in, float *out, size_t size)
{ 
    float output;
  
    pod.UpdateAnalogControls();
    pod.DebounceControls();
    
    if (pod.button1.RisingEdge())
    {
        if (first && rec)
	{
	  first = false;
	  //mod = size;
	}
        rec = !rec;
    }
    
    if (pod.button1.TimeHeldMs() >= 1000)
    {
        Reset();
    }
    
    for (size_t i = 0; i < size; i += 2)
    {
	if (rec)
	{
	    buf[pos] = .5 * in[i] + .5 * buf[pos];
	    if (first)
	    {
	        size++;
	    }
	}

      	output = buf[pos];
	
	if (size >= MAX_SIZE)
	{
	    first = false;
	    rec   = false;
	    mod   = size;
	    size  = 0;
	    return;
	}

	pos++; //speed control goes here
	pos = pos % mod;
     	
	// left out
	out[i]   = output;
	
	// right out
	out[i+1] = output;
    }
}

int main(void)
{
    // initialize pod hardware and oscillator daisysp module
    
    pod.Init();
    //float sample_rate = pod.AudioSampleRate();
    Reset();
    
    // start callback
    pod.StartAdc();
    pod.StartAudio(AudioCallback);

    while(1) {}
}
