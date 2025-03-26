#include "daisysp.h"
#include "daisy_seed.h"
#include "daisysp-lgpl.h"

// Set max delay time to 0.75 of samplerate.
#define MAX_DELAY static_cast<size_t>(48000 * 2.5f)

#define MAX_SIZE (48000 * 60 * 5) // 5 minutes of floats at 48 khz

#define MAX_FX 5
#define MENUS 2

#define SETUP 0
#define START 1

using namespace daisysp;
using namespace daisy;
using namespace daisy::seed;

static DaisySeed hw;

/** I2C */
I2CHandle i2c;
uint8_t reverbSel[4] = {0, 1, 1, 1};
uint8_t delaySel[4] = {0, 1, 1, 2};
uint8_t chorusSel[4] = {0, 1, 1, 3};
uint8_t crushSel[4] = {0, 1, 1, 4};

static ReverbSc                                  rev;
static DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS dell;
static DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delr;
static Tone                                      tone;
static Parameter deltime, cutoffParam, crushrate;
int              mode = 0;

/** CHORUS */
Chorus ch;
float deltarget, del;;
float lfotarget, lfo;

float currentDelay, feedback, delayTarget, cutoff;

int   crushmod, crushcount;
float crushsl, crushsr, drywet;

void (*functptr[MAX_FX])(float &outl, float &outr, float inl, float inr);

/** LEDS */

GPIO led1;
GPIO led2;
GPIO led3;
GPIO led4;

/** Switches */
    enum Sw
    {
        BUTTON_1,    /** & */
        BUTTON_2,    /** & */
        BUTTON_3,
        BUTTON_4,
        BUTTON_5,
        BUTTON_6,
        BUTTON_LAST, /** &  */
    };

Switch button1,            /**< & */
        button2,               /**< & */
        button3,
        button4,
        button5,
        button6,
        *buttons[BUTTON_LAST]; /**< & */

bool eqSet = false;
bool revSet = false;
bool delSet = false;
bool chorusSet = false;
bool crushSet = false;
int eqMode = -1;
int revMode = -1;
int delMode = -1;
int chorusMode = -1;
int crushMode = -1;

int menu = 0;

bool edit = false;

size_t count = 0;

/** ENCODER */

Encoder encoder;

/** STATE */

int state = SETUP;

/** KNOBS */
enum Knob
{
    KNOB_1,    /** &  */
    KNOB_2,    /** & */
    KNOB_3,
    KNOB_4,
    KNOB_5,
    KNOB_LAST, /** & */
};

AnalogControl knob1,       /**< & */
    knob2,                 /**< & */
    knob3,
    knob4,
    knob5,
    *knobs[KNOB_LAST];

/** LOOP */

Looper looper;

bool first = true;  //first loop (sets length)
bool rec   = false; //currently recording
bool play  = false; //currently playing

int                 pos = 0;
float DSY_SDRAM_BSS buf[MAX_SIZE];
int                 mod    = MAX_SIZE;
int                 len    = 0;
float               mix = 0;
bool                res    = false;

/** TEST LED */
bool led_state = true;

//Helper functions
void Controls();

void SetupControls();

void GetReverbSample(float &outl, float &outr, float inl, float inr);

void GetDelaySample(float &outl, float &outr, float inl, float inr);

void GetCrushSample(float &outl, float &outr, float inl, float inr);

void GetChorusSample(float &outl, float &outr, float inl, float inr);

void InitKnobs();

void InitButtons();

void InitEncoder();

void ProcessAnalogControls();

void ProcessDigitalControls();

void SetupButtons();

/*void NextSamples(float&                               output,
                 AudioHandle::InterleavingInputBuffer in,
                 size_t                               i);*/

void StartCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
    SetupControls();

    memcpy(out, in, size * sizeof(float));
}

void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
    float buf1, buf2, out1, out2;

    Controls();

    //audio
    for(size_t i = 0; i < size; i += 2)
    {
        buf1 = in[i];
        buf2 = in[i + 1];

        for(size_t j = 0; j < count; j++)
        {
            (*functptr[j])(buf1, buf2, buf1, buf2); 
        }

        // left out
        out[i] = buf1;

        // right out
        out[i + 1] = buf2;
    }
}

int main(void)
{
    // setup the configuration
    I2CHandle::Config i2c_conf;
    i2c_conf.periph = I2CHandle::Config::Peripheral::I2C_1;
    i2c_conf.speed  = I2CHandle::Config::Speed::I2C_400KHZ;
    i2c_conf.mode   = I2CHandle::Config::Mode::I2C_MASTER;
    i2c_conf.pin_config.scl  = {DSY_GPIOB, 8};
    i2c_conf.pin_config.sda  = {DSY_GPIOB, 9};
    //initialise the peripheral
    i2c.Init(i2c_conf);
    // now i2c points to the corresponding peripheral and can be used.

    // initialize seed hardware and whitenoise daisysp module
    float sample_rate;
    hw.Configure();
    hw.Init();
    InitKnobs();
    InitButtons();
    InitEncoder();
    hw.SetAudioBlockSize(4);
    sample_rate = hw.AudioSampleRate();

    //chorus
    ch.Init(sample_rate);
    deltarget = del = 0.f;
    lfotarget = lfo = 0.f;

    //set parameters
    rev.Init(sample_rate);
    tone.Init(sample_rate);
    dell.Init();
    delr.Init();
    deltime.Init(knob1, sample_rate * .05, MAX_DELAY, deltime.LOGARITHMIC);
    cutoffParam.Init(knob1, 500, 20000, cutoffParam.LOGARITHMIC);
    crushrate.Init(knob2, 1, 50, crushrate.LOGARITHMIC);

    //reverb parameters
    rev.SetLpFreq(18000.0f);
    rev.SetFeedback(0.85f);

    //delay parameters
    currentDelay = delayTarget = sample_rate * 0.75f;
    dell.SetDelay(currentDelay);
    delr.SetDelay(currentDelay);

    //debugging LEDs
    led1.Init(D24, GPIO::Mode::OUTPUT);
    led2.Init(D23, GPIO::Mode::OUTPUT);
    led3.Init(D22, GPIO::Mode::OUTPUT);
    led4.Init(D21, GPIO::Mode::OUTPUT);

    led1.Write(true);
    led2.Write(true);
    led3.Write(true);

    // start callback
    hw.adc.Start();
    hw.StartAudio(StartCallback);
    while(1) {}
}

void UpdateKnobs(float &k1, float &k2, float &k3, float &k4, float &k5)
{
    if(edit)
    {
        k1 = knob1.Process();
        k2 = knob2.Process();
        k3 = knob3.Process();
        k4 = knob4.Process();
        //k5 = knob5.Process();

        if(mode == revMode)
        {
            drywet = k1;
            rev.SetFeedback(k2);
        }
        else if(mode == delMode)
        {
            delayTarget = deltime.Process();
            feedback    = k2;
        }
        else if(mode == chorusMode)
        {
            //knobs
            ch.SetLfoFreq(k1 * k1 * 20.f);
            lfotarget  = k2;
            deltarget = k3;
            ch.SetFeedback(k4);
            hw.SetLed(led_state);
        }
        else if(mode == crushMode)
        {
            cutoff = cutoffParam.Process();
            tone.SetFreq(cutoff);
            crushmod = (int)crushrate.Process();
        }
    }

    led1.Write(!((0x0004 & mode) >> 2));
    led2.Write(!((0x0002 & mode) >> 1));
    led3.Write(!(0x0001 & mode));
}

void UpdateEncoder()
{
    if(!edit)
    {
        mode = mode + encoder.Increment();
        if (mode == -1)
        {
            mode = count-1;
        }
        else 
        {
            mode = (mode % count + count) % count;
        }
    }
}

void SetupButtons()
{
    if(menu == 0)
    {
        if(button3.RisingEdge()  && !revSet)
        {
            revSet = true;
            revMode = count;
            functptr[count] = GetReverbSample;
            count++;
            //i2c.TransmitBlocking(0x41, reverbSel, 4, 1000);
        }
        if(button4.RisingEdge()  && !delSet)
        {
            delSet = true;
            delMode = count;
            functptr[count] = GetDelaySample;
            count++;
            //i2c.TransmitBlocking(0x41, delaySel, 4, 1000);
        }
        if(button5.RisingEdge()  && !chorusSet)
        {
            chorusSet = true;
            chorusMode = count;
            functptr[count] = GetChorusSample;
            count++;
            //i2c.TransmitBlocking(0x41, chorusSel, 4, 1000);
        }
    }
    else if(menu == 1)
    {
        if(button3.RisingEdge()  && !crushSet)
        {
            crushSet = true;
            crushMode = count;
            functptr[count] = GetCrushSample;
            count++;
            //i2c.TransmitBlocking(0x41, crushSel, 4, 1000);
        }
    }
    
    if(button6.RisingEdge())
    {
        led1.Write(true);
        led2.Write(true);
        led3.Write(true);
        state = START;
        hw.ChangeAudioCallback(AudioCallback);
    }
    led1.Write(!((0x0004 & count) >> 2));
    led2.Write(!((0x0002 & count) >> 1));
    led3.Write(!(0x0001 & count));
}

void UpdateButtons()
{
    if(button3.RisingEdge() && !edit)
    {
        edit = true;
    }
    if(button6.RisingEdge() && edit)
    {
        edit = false;
    }
    if(button6.TimeHeldMs() >= 1000)
    {
        hw.ChangeAudioCallback(StartCallback);
        count = 0;
    }
}

void SetupControls()
{
    ProcessDigitalControls();
    if((menu == 0) && (encoder.Increment() == 1))
    {
        menu = 1;
    }
    if((menu == 1) && (encoder.Increment() == -1))
    {
        menu = 0;
    }
    SetupButtons();
}

void Controls()
{
    float k1, k2, k3, k4, k5;
    //delayTarget = feedback = drywet = 0;

    ProcessAnalogControls();
    ProcessDigitalControls();

    UpdateKnobs(k1, k2, k3, k4, k5);

    UpdateEncoder();

    UpdateButtons();
}

void GetReverbSample(float &outl, float &outr, float inl, float inr)
{
    rev.Process(inl, inr, &outl, &outr);
    outl = drywet * outl + (1 - drywet) * inl;
    outr = drywet * outr + (1 - drywet) * inr;
}

void GetDelaySample(float &outl, float &outr, float inl, float inr)
{
    fonepole(currentDelay, delayTarget, .00007f);
    delr.SetDelay(currentDelay);
    dell.SetDelay(currentDelay);
    outl = dell.Read();
    outr = delr.Read();

    dell.Write((feedback * outl) + inl);
    outl = (feedback * outl) + ((1.0f - feedback) * inl);

    delr.Write((feedback * outr) + inr);
    outr = (feedback * outr) + ((1.0f - feedback) * inr);
}

void GetCrushSample(float &outl, float &outr, float inl, float inr)
{
    crushcount++;
    crushcount %= crushmod;
    if(crushcount == 0)
    {
        crushsr = inr;
        crushsl = inl;
    }
    outl = tone.Process(crushsl);
    outr = tone.Process(crushsr);
}

void GetChorusSample(float &outl, float &outr, float inl, float inr)
{
    fonepole(del, deltarget, .0001f); //smooth these at audio rate
    fonepole(lfo, lfotarget, .0001f);

    ch.SetLfoDepth(lfo);
    ch.SetDelay(del);

    ch.Process(inl);

    outl = ch.GetLeft();
    outr = ch.GetRight();
}

void InitKnobs()
{
    // Configure the ADC channels using the desired pin
    AdcChannelConfig knob_init[KNOB_LAST];
    knob_init[KNOB_1].InitSingle(hw.GetPin(20));
    knob_init[KNOB_2].InitSingle(hw.GetPin(19));
    knob_init[KNOB_3].InitSingle(hw.GetPin(18));
    knob_init[KNOB_4].InitSingle(hw.GetPin(17));
    knob_init[KNOB_5].InitSingle(hw.GetPin(16));
    // Initialize with the knob init struct w/ 2 members
    // Set Oversampling to 32x
    hw.adc.Init(knob_init, KNOB_LAST);
    // Make an array of pointers to the knobs.
    knobs[KNOB_1] = &knob1;
    knobs[KNOB_2] = &knob2;
    knobs[KNOB_3] = &knob3;
    knobs[KNOB_4] = &knob4;
    knobs[KNOB_5] = &knob5;
    for(int i = 0; i < KNOB_LAST; i++)
    {
        knobs[i]->Init(hw.adc.GetPtr(i), hw.AudioCallbackRate());
    }
}

void InitButtons()
{
    // button1
    button1.Init(hw.GetPin(28));
    // button2
    button2.Init(hw.GetPin(27));
    // button3
    button3.Init(hw.GetPin(7));
    // button4
    button4.Init(hw.GetPin(8));
    // button5
    button5.Init(hw.GetPin(9));
    // button6
    button6.Init(hw.GetPin(10));

    buttons[BUTTON_1] = &button1;
    buttons[BUTTON_2] = &button2;
    buttons[BUTTON_3] = &button3;
    buttons[BUTTON_4] = &button4;
    buttons[BUTTON_5] = &button5;
    buttons[BUTTON_6] = &button6;
}

void InitEncoder()
{
    encoder.Init(hw.GetPin(25), hw.GetPin(26), hw.GetPin(13));
}

void ProcessAnalogControls()
{
    knob1.Process();
    knob2.Process();
    knob3.Process();
    knob4.Process();
    knob5.Process();
}

void ProcessDigitalControls()
{
    encoder.Debounce();
    //button1.Debounce();
    //button2.Debounce();
    button3.Debounce();
    button4.Debounce();
    button5.Debounce();
    button6.Debounce();
}