#include "daisysp.h"
#include "daisy_pod.h"

const int MAX_SIZE = (int)(48000); //1 mins of floats at 48kHz

using namespace daisysp;
using namespace daisy;

static DaisyPod pod;

bool first = true; //first loop (sets length)
bool rec = false; //currently recording
bool play = false;
float pos = 0;
float buf[MAX_SIZE];
int mod = MAX_SIZE;
int len = 0;
int lastpos = 0;

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

static void AudioCallback(float *in, float *out, size_t size)
{ 
    float output;
  
    pod.UpdateAnalogControls();
    pod.DebounceControls();

    float speed = pod.knob1.Process() * 5 + 1;
    
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
    
    for (size_t i = 0; i < size; i += 2)
    {
        float posrem = pos - (int) pos;
	int posflr = (int) floor(pos);
	int posceil = (int) ceil(pos);
	
	if (rec)
	{
	  buf[(int)floor(pos)] = ((1 - posrem) * buf[posflr] + posrem * buf[posceil]) * 0.5 + in[i] * 0.5;
	  if (first)
	    {
		len++;
	    }	  
	}

	
      	output = (1 - posrem) * buf[(int)floor(pos)] + posrem * buf[(int)ceil(pos)]; //linear interpolation
	
	if (len >= MAX_SIZE)
	{
	    first = false;
	    rec   = false;
	    mod   = MAX_SIZE;
	    len = 0;
	}

	if (play)
	{
	    pos += speed;
	    pos = fmod(pos, mod);
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
    //float sample_rate = pod.AudioSampleRate();
    Reset();
    
    // start callback
    pod.StartAdc();
    pod.StartAudio(AudioCallback);

    while(1) {}
}
