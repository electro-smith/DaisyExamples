#include "daisy_seed.h"
#include "daisysp.h"
​
using namespace daisy;
​
DaisySeed       hw;
daisysp::Phasor phs;
​
float wavform_ram[16384];
​
void Callback(AudioHandle::InputBuffer  in,
              AudioHandle::OutputBuffer out,
              size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        float    idx  = phs.Process() * 16383.f;
        uint32_t tdx  = (uint32_t)idx;
        float    frac = idx - tdx;
        float    a, b;
        float*   qspi_buff = (float*)System::qspi_start;
        a                  = qspi_buff[tdx];
        b                  = qspi_buff[(tdx + 1) % 16834];
        float samp         = a + (b - a) * frac;
        out[0][i] = out[1][i] = samp;
    }
}
​
int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(2);
    phs.Init(hw.AudioSampleRate());
    phs.SetFreq(100.f);
​
    /** Fill 64kB sine wave */
    for(uint32_t i = 0; i < 16384; i++)
    {
        float frac     = i / 16384.f;
        wavform_ram[i] = sin(TWOPI_F * frac);
    }
​
    /** erase and then write that sin */
    size_t size = sizeof(wavform_ram[0]) * 16834;
    hw.qspi.Erase(System::qspi_start, System::qspi_start + size);
    hw.qspi.Write(System::qspi_start, size, (uint8_t*)wavform_ram);
​
    /** Now compare to RAM version */
    uint32_t failcnt = 0;
    for(uint32_t i = 0; i < 16384; i++)
    {
        float* wavform_qspi = (float*)(System::qspi_start) + i;
        if(*wavform_qspi != wavform_ram[i])
            failcnt++;
    }
​
    unsigned int rate = (failcnt > 0) ? 128 : 512;

    hw.StartAudio(Callback);
​
    while(1)
    {
        hw.SetLed(System::GetNow() & rate);
    }
}