/**   @brief FIR Filter example
 *    @author Alexander Petrov-Savchenko (axp@soft-amp.com)
 *    @date February 2021
 */

#include "daisysp.h"
#include "daisy_seed.h"

using namespace daisysp;
using namespace daisy;

static DaisySeed                  hw;
static Oscillator                 lfo;
static WhiteNoise                 osc;
static FIR<FIRFILTER_USER_MEMORY> flt;


static constexpr size_t flt_size    = 512;  /*< FIR filter length */
static constexpr int    upd_rate_hz = 1000; /*< FIR recalculation rate */

static float ir_front[flt_size]    = {0};   /*< Active FIR coefficients */
static float ir_back[flt_size + 1] = {0};   /*< Updating FIR coefficients */
static float wnd[flt_size / 2]     = {0};   /*< Windowing function */
static bool  ir_update_pending     = false; /*< ir_back update ready */
static int   update_count          = 0;     /*< counter of FIR updates */
static int   callback_count        = 0;     /*< counter of audio callbacks*/
static float flt_state[flt_size + 1];       /*< Impl-specific storage */

/* Blackman-Harris window function */
static void InitWindow()
{
    float scale = TWOPI_F / flt_size;

    const float a0 = 0.35875f;
    const float a1 = 0.48829f;
    const float a2 = 0.14128f;
    const float a3 = 0.01168f;

    for(size_t i = 0; i < flt_size / 2; i++)
    {
        const float wind_arg = scale * i;
        wnd[i] = a0 - a1 * cosf(wind_arg) + a2 * cosf(wind_arg * 2)
                 - a3 * cosf(wind_arg * 3);
    }
}

/* Windowed Sinc filter design */
static void UpdateFilter(float freq)
{
    /* Only proceed if the previous filter was already applied */
    if(false == ir_update_pending)
    {
        constexpr int half  = flt_size / 2;
        const float   scale = 2.0f * freq;

        /* Symmetric filter shape */
        for(int i = 0; i < half; i++)
        {
            const float sinc_arg = PI_F * (i - half);
            const float sinc     = sinf(sinc_arg * scale) / sinc_arg;
            const float filt     = sinc * wnd[i];

            ir_back[i]            = filt;
            ir_back[flt_size - i] = filt; /* may access ir_back[flt_size] !! */
        }
        ir_back[half] = scale;

        ir_update_pending = true;
    }
}

static void ApplyFilter()
{
    if(true == ir_update_pending)
    {
        for(size_t i = 0; i < flt_size; i++)
        {
            ir_front[i] = ir_back[i];
        }
        ir_update_pending = false;
    }
}

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    /* Copy generated coefficients to active buffer */
    ApplyFilter();

    for(size_t i = 0; i < size; i += 2)
    {
        const float input  = osc.Process();
        const float output = flt.Process(input);

        out[i]     = output; /*< Left */
        out[i + 1] = output; /*< Right */
    }

    callback_count++;
}

int main(void)
{
    /* initialize seed hardware and daisysp modules */
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    hw.StartLog(false);

    /* calculate callback and update rates */
    const int      callback_rate = (int)hw.AudioCallbackRate();
    const uint32_t tick_rate     = hw.system.GetPClk1Freq() * 2;
    const uint32_t update_period = tick_rate / upd_rate_hz;

    /** initialize FIR filter and set parameters 
     * Note: in this mode (user-managed memory) the coefficients
     * are NOT copied, so we may update them at runtime
     */

    flt.SetStateBuffer(flt_state, DSY_COUNTOF(flt_state));
    flt.Init(ir_front, flt_size, false);
    InitWindow();
    UpdateFilter(0.5f);

    /* set parameters for the control oscillator object */
    lfo.Init((float)upd_rate_hz);
    lfo.SetWaveform(Oscillator::WAVE_TRI);
    lfo.SetAmp(1.0f);
    lfo.SetFreq(.1f); /* 10 seconds period */

    /* set parameters for the source oscillator object */
    osc.Init();
    osc.SetAmp(0.5f);

    /* start audio callback */
    hw.StartAudio(AudioCallback);

    uint32_t next_update = hw.system.GetTick();
    while(1)
    {
        /* cut-off frequency oscillates between 0.01f and 0.31f */
        const float cutoff_freq = 0.01f + 0.15f * (1.0f + lfo.Process());
        UpdateFilter(cutoff_freq);
        update_count++;

        /* estimate average filter update rate once per second */
        if(callback_count % callback_rate == (callback_rate - 1))
        {
            const float rate
                = (float)update_count * callback_rate / callback_count;
            hw.PrintLine("Average = " FLT_FMT3 "upd/sec", FLT_VAR3(rate));
        }

        /** delay to roughly maintain the requested update rate 
         * Note: this is better than using Delay* functions, 
         * because the error doesn't accumulate 
         */
        while(next_update - hw.system.GetTick() < update_period) {}
        next_update += update_period;
    }
}
