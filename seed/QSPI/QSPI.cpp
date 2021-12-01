#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;

DaisySeed       hw;
daisysp::Phasor phs;

#define WAVE_LENGTH 16384
float wavform_ram[WAVE_LENGTH];
/** The DSY_QSPI_BSS attribute places your array in QSPI memory */
float DSY_QSPI_BSS qspi_buffer[WAVE_LENGTH];

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
        a          = qspi_buffer[tdx];
        b          = qspi_buffer[(tdx + 1) % WAVE_LENGTH];
        float samp = a + (b - a) * frac;

        out[0][i] = out[1][i] = samp;
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);

    phs.Init(hw.AudioSampleRate());
    phs.SetFreq(440.f);

    /** Fill 64kB wave */
    for(uint32_t i = 0; i < WAVE_LENGTH; i++)
    {
        float frac = (float)i / (float)WAVE_LENGTH;

        /** Simple saw wave gen */
        float accum = 0;
        for(int j = 1; j < 64; j++)
        {
            accum += sin(TWOPI_F * frac * j) / j;
        }
        wavform_ram[i] = accum;
    }

    size_t size = sizeof(wavform_ram[0]) * WAVE_LENGTH;
    /** Grab physical address from pointer */
    size_t address = (size_t)qspi_buffer;
    /** Erase qspi and then write that wave */
    hw.qspi.Erase(address, address + size);
    hw.qspi.Write(address, size, (uint8_t*)wavform_ram);

    /** Now compare to RAM version */
    uint32_t failcnt = 0;
    for(uint32_t i = 0; i < WAVE_LENGTH; i++)
    {
        if(qspi_buffer[i] != wavform_ram[i])
            failcnt++;
    }

    unsigned int rate = (failcnt > 0) ? 128 : 512;

    hw.StartAudio(Callback);

    while(1)
    {
        hw.SetLed(System::GetNow() & rate);
    }
}