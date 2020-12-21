// Daisy Patch FmTones
#include "daisy_field.h"
#include "daisysp.h"

//#define TEST_MEMORY 1

using namespace daisy;
using namespace daisysp;

static DaisyField hw;
static Fm2        osc;
static AdEnv      env, aenv;
static uint32_t   delta_ms;
static bool       synth_trigger;

static int32_t note_table[15]
    = {0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23, 24};
static int32_t melody_note;

// And Reverb in the ext. SRAM
static ReverbSc DSY_SDRAM_BSS verb;

void AudioCallback(float **in, float **out, size_t size)
{
    hw.ProcessAllControls();
    // Synth triggers when center of circle collides
    // with edge of display.
    if(synth_trigger)
    {
        synth_trigger = false;
        melody_note   = note_table[rand() % 15];
        env.Trigger();
        aenv.Trigger();
    }
    // Bunch of parameters in a row.
    float freq = mtof((12.f + hw.GetKnobValue(hw.KNOB_1) * 72.f) + melody_note);
    float index = hw.GetKnobValue(hw.KNOB_2);
    float curve = (hw.GetKnobValue(hw.KNOB_3) * 2.f) - 1.f;
    float atime = 0.001f + (hw.GetKnobValue(hw.KNOB_4) * 0.5f);
    float dtime = 0.001f + (hw.GetKnobValue(hw.KNOB_5) * 0.5f);
    float ratio = 2.f + (hw.GetKnobValue(hw.KNOB_6) * 12.f);
    float time  = (0.5f + (0.49f * hw.GetKnobValue(hw.KNOB_7)));
    float vfreq = (500.f + (4000.f * hw.GetKnobValue(hw.KNOB_8)));
    osc.SetFrequency(freq);
    osc.SetRatio(ratio);
    env.SetTime(ADENV_SEG_ATTACK, atime);
    env.SetTime(ADENV_SEG_DECAY, dtime);
    env.SetCurve(curve * 10.f);
    aenv.SetTime(ADENV_SEG_ATTACK, atime);
    aenv.SetTime(ADENV_SEG_DECAY, dtime * 1.5f);
    verb.SetFeedback(time);
    verb.SetLpFreq(vfreq);
    for(size_t i = 0; i < size; i++)
    {
        env.SetMax(index);
        osc.SetIndex(env.Process());
        float dryl, dryr, sendl, sendr, wetl, wetr;
        dryl = dryr = osc.Process() * aenv.Process();
        sendl       = dryl * 0.5f;
        sendr       = dryr * 0.5f;
        verb.Process(sendl, sendr, &wetl, &wetr);
        out[0][i] = (dryl + wetl) * 0.707f;
        out[1][i] = (dryr + wetr) * 0.707f;
    }
}

int main(void)
{
    uint32_t dt, ledt;
    hw.Init();
    System::Delay(500);
    hw.seed.StartLog(false);
    hw.StartAdc();
    // Init DSP
    melody_note = 0;
    osc.Init(hw.AudioSampleRate());
    env.Init(hw.AudioSampleRate());
    aenv.Init(hw.AudioSampleRate());
    aenv.SetMin(0.00001f);
    aenv.SetMax(.7f);
    env.SetTime(ADENV_SEG_ATTACK, 0.001f);
    env.SetTime(ADENV_SEG_DECAY, 0.25f);
    env.SetMin(0.02f);
    verb.Init(hw.AudioSampleRate());

    // Drawing Data
    float xspeed = 1.f;
    float yspeed = 2.f;
    int   xpos, ypos;
    xpos = SSD1309_WIDTH / 2;
    ypos = SSD1309_HEIGHT / 2;

    /////////////////////////////////////////////////////////////
    // Start Audio
    /////////////////////////////////////////////////////////////
    hw.StartAudio(AudioCallback);
    delta_ms = 0;
    ledt = dt = System::GetNow();

    while(1)
    {
        uint32_t now;
        now = System::GetNow();

        // Handle Display stuff (at 30Hz)
        if(now - ledt > 32)
        {
            bool collision = false;
            ledt           = now;

            // Update Circle location
            xpos += xspeed;
            ypos += yspeed;
            if(xpos > SSD1309_WIDTH || xpos < 0)
            {
                xspeed    = -xspeed;
                collision = true;
            }
            if(ypos > SSD1309_HEIGHT || ypos < 0)
            {
                yspeed    = -yspeed;
                collision = true;
            }
            if(collision)
            {
                synth_trigger = true;
            }
            hw.display.Fill(false);
            hw.display.DrawCircle(xpos, ypos, 16, true);
            hw.led_driver.SwapBuffersAndTransmit();
            hw.display.Update();
        }
    }
}
