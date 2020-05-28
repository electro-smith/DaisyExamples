// Press button one to record. First recording sets loop length. Automatically starts looping  if 1 second limit is hit.
// After first loop sound on sound recording begins. Press button one to toggle SOS recording. Hold button one to clear loop.
// Red lights indicate record enable.
// Press button 2 to pause/play loop buffer.


#include "daisysp.h"
#include "daisy_pod.h"

#define MAX_SIZE (48000 * 2) //2 seconds of floats at 48kHz

using namespace daisysp;
using namespace daisy;

static DaisyPod pod;

bool first = true; //first loop (sets length)
bool rec = false; //currently recording
bool play = false; //currently playing

int pos = 0;
float buf[MAX_SIZE];
int mod = MAX_SIZE;
int len = 0;

void Reset()
{
  for (int i = 0; i < MAX_SIZE; i++)
  {
      buf [i] = 0;
  }
  first = true;
  rec = false;
  mod = MAX_SIZE;
  pos = 0;
  play = false;
  len = 0;
}

void Controls()
{
    pod.UpdateAnalogControls();
    pod.DebounceControls();
    
    if (pod.button1.RisingEdge())
    {
        if (first && rec)
	{
	    first = false;
	    mod = len;
	    len = 0;
	}

	play = true;	
        rec = !rec;
    }

    if (pod.button2.RisingEdge())
    {
        play = ! play;
    }
    
    if (pod.button1.TimeHeldMs() >= 1000)
    {
        Reset();
    }

    if (rec)
    {
        pod.led1.Set(1,0,0);
        pod.led2.Set(1,0,0);
    }
    else
    {
	pod.ClearLeds();
    }
    pod.UpdateLeds();
}

static void AudioCallback(float *in, float *out, size_t size)
{ 
    float output;
    
    Controls();
   
    for (size_t i = 0; i < size; i += 2)
    {
       	if (rec)
	{
	    buf[pos] = buf[pos] * 0.5 + in[i] * 0.5;
	    if (first)
	    {
		len++;
	    }	  
	}

	
      	output = buf[pos];
	
	if (len >= MAX_SIZE)
	{
	    first = false;
	    mod   = MAX_SIZE;
	    len = 0;
	}

	if (play)
	{
	    pos++;
	    pos %= mod;
	}
	
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
    Reset();
    
    // start callback
    pod.StartAdc();
    pod.StartAudio(AudioCallback);

    while(1) {}
}
