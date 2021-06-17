#include "daisy_field.h"
#include "daisysp.h"
#include "granular_processor.h"

#define NUM_PARAMS 9
#define NUM_PAGES 5

#define KNOB_TOLERANCE .001f

#define DISPLAY_WIDTH 128
#define LEFT_BORDER_WIDTH 2
#define PARAM_NAME_COL_WIDTH 70
#define ROW_HEIGHT 14
#define SKINNY_ROW_HEIGHT 13
#define DEFAULT_FONT Font_7x10
#define SMALL_FONT Font_6x8
#define CHECKBOX_SIZE 8
#define CHECKBOX_MARGIN 3
#define VALUE_BAR_HEIGHT 10
#define VALUE_BAR_WIDTH 55
#define VALUE_BAR_MARGIN 3

#define PARAM_BUFFER_SIZE 8

using namespace daisysp;
using namespace daisy;

enum DISPLAY_PAGE
{
    PARAMETERS1TO3,
    PARAMETERS4TO6,
    PARAMETERS7TO9,
    BUTTONS1,
    BUTTONS2,
};

enum DISPLAY_ROW
{
    ROW_1 = 1,
    ROW_2,
    ROW_3,
    ROW_4,
};

enum
{
    PLAYBACK_QUAL_16B_ST = 0,
    PLAYBACK_QUAL_16B_MO = 1,
    PLAYBACK_QUAL_8B_ST  = 2,
    PLAYBACK_QUAL_8B_MO  = 3,
};

enum
{
    LED_PLAYBACK_QUAL_16B_ST        = 0,
    LED_PLAYBACK_QUAL_16B_MO        = 1,
    LED_PLAYBACK_QUAL_8B_ST         = 2,
    LED_PLAYBACK_QUAL_8B_MO         = 3,
    LED_SHIFT                       = 7,
    LED_BYPASS                      = 8,
    LED_SILENCE                     = 9,
    LED_FREEZE                      = 10,
    LED_PLAYBACK_MODE_SPECTRAL      = 12,
    LED_PLAYBACK_MODE_LOOPING_DELAY = 13,
    LED_PLAYBACK_MODE_STRETCH       = 14,
    LED_PLAYBACK_MODE_GRANULAR      = 15,
};

enum
{
    BUTTON_PLAYBACK_MODE_GRANULAR      = 0,
    BUTTON_PLAYBACK_MODE_STRETCH       = 1,
    BUTTON_PLAYBACK_MODE_LOOPING_DELAY = 2,
    BUTTON_PLAYBACK_MODE_SPECTRAL      = 3,
    BUTTON_FREEZE                      = 5,
    BUTTON_SILENCE                     = 6,
    BUTTON_BYPASS                      = 7,
    BUTTON_PLAYBACK_QUAL_16B_ST        = 8,
    BUTTON_PLAYBACK_QUAL_16B_MO        = 9,
    BUTTON_PLAYBACK_QUAL_8B_ST         = 10,
    BUTTON_PLAYBACK_QUAL_8B_MO         = 11,
    BUTTON_SHIFT                       = 15,
};

GranularProcessorClouds processor;
DaisyField              field;

// Pre-allocate big blocks in main memory and CCM. No malloc here.
uint8_t block_mem[118784];
uint8_t block_ccm[65536 - 128];

class ParamControl
{
  public:
    ParamControl() {}
    ~ParamControl() {}

    void Init(const char*    name,
              AnalogControl* knob,
              int            led,
              bool           shifted,
              Parameters*    params,
              int            param_num,
              DISPLAY_PAGE   display_page)
    {
        name_         = name;
        knob_         = knob;
        led_          = led;
        shifted_      = shifted;
        params_       = params;
        param_num_    = param_num;
        val_          = Changed(0.f);
        display_page_ = display_page;
    }

    const char* GetName() { return name_; }

    bool Changed(float newval)
    {
        bool ret = fabsf(newval - val_) > KNOB_TOLERANCE;
        val_     = newval;
        if(ret)
        {
            changed_ = true;
            snprintf(val_str, PARAM_BUFFER_SIZE, "%d", int(val_ * 100));
        }
        else
        {
            changed_ = false;
        };
        return ret;
    }

    int          GetLed() const { return led_; }
    float        GetValue() { return val_; }
    char*        GetValueAsString() { return val_str; }
    int          GetParamNum() { return param_num_; }
    bool         GetShifted() { return shifted_; }
    bool         IsChanged() { return changed_; }
    DISPLAY_PAGE GetDisplayPage() { return display_page_; }

    void Process()
    {
        float val = knob_->Process();
        if(!Changed(val))
        {
            return;
        }

        switch(param_num_)
        {
            case 0: params_->position = val; break;
            case 1: params_->size = val; break;
            case 2:
                params_->pitch = powf(9.798f * (val - .5f), 2.f);
                params_->pitch *= val < .5f ? -1.f : 1.f;
                break;
            case 3: params_->density = val; break;
            case 4: params_->texture = val; break;
            case 5: params_->dry_wet = val; break;
            case 6: params_->stereo_spread = val; break;
            case 7: params_->feedback = val; break;
            case 8: params_->reverb = val; break;
        }
    }

  private:
    const char*    name_;
    AnalogControl* knob_;
    int            led_;
    bool           shifted_;
    Parameters*    params_;
    int            param_num_;
    float          val_;
    char           val_str[PARAM_BUFFER_SIZE];
    bool           changed_;
    DISPLAY_PAGE   display_page_;
};

ParamControl param_controls[NUM_PARAMS];
Parameters*  parameters;

DISPLAY_PAGE current_display_page;
bool         is_silenced, is_bypassed, is_shifted;

void Controls();
void UpdateLeds();
void UpdateOled();
void ProcessButtons();

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    Controls();

    FloatFrame input[size];
    FloatFrame output[size];

    for(size_t i = 0; i < size; i++)
    {
        input[i].l  = in[0][i] * .5f;
        input[i].r  = in[1][i] * .5f;
        output[i].l = output[i].r = 0.f;
    }

    processor.Process(input, output, size);

    for(size_t i = 0; i < size; i++)
    {
        out[0][i] = output[i].l;
        out[1][i] = output[i].r;
    }
}

void InitParams()
{
    param_controls[0].Init("Position:",
                           &field.knob[0],
                           DaisyField::LED_KNOB_1,
                           false,
                           parameters,
                           0,
                           DISPLAY_PAGE::PARAMETERS1TO3);
    param_controls[1].Init("Size:",
                           &field.knob[1],
                           DaisyField::LED_KNOB_2,
                           false,
                           parameters,
                           1,
                           DISPLAY_PAGE::PARAMETERS1TO3);
    param_controls[2].Init("Pitch:",
                           &field.knob[2],
                           DaisyField::LED_KNOB_3,
                           false,
                           parameters,
                           2,
                           DISPLAY_PAGE::PARAMETERS1TO3);
    param_controls[3].Init("Density:",
                           &field.knob[3],
                           DaisyField::LED_KNOB_4,
                           false,
                           parameters,
                           3,
                           DISPLAY_PAGE::PARAMETERS4TO6);
    param_controls[4].Init("Texture:",
                           &field.knob[4],
                           DaisyField::LED_KNOB_5,
                           false,
                           parameters,
                           4,
                           DISPLAY_PAGE::PARAMETERS4TO6);
    param_controls[5].Init("Dry/Wet:",
                           &field.knob[5],
                           DaisyField::LED_KNOB_6,
                           false,
                           parameters,
                           5,
                           DISPLAY_PAGE::PARAMETERS4TO6);
    param_controls[6].Init("Spread:",
                           &field.knob[6],
                           DaisyField::LED_KNOB_7,
                           false,
                           parameters,
                           6,
                           DISPLAY_PAGE::PARAMETERS7TO9);
    param_controls[7].Init("Feedback:",
                           &field.knob[7],
                           DaisyField::LED_KNOB_8,
                           false,
                           parameters,
                           7,
                           DISPLAY_PAGE::PARAMETERS7TO9);
    param_controls[8].Init("Revereb:",
                           &field.knob[7],
                           DaisyField::LED_KNOB_8,
                           true,
                           parameters,
                           8,
                           DISPLAY_PAGE::PARAMETERS7TO9);
}

int main(void)
{
    field.Init();
    field.SetAudioBlockSize(32); 
    float sample_rate = field.AudioSampleRate();

    //init the luts
    InitResources(sample_rate);

    processor.Init(sample_rate,
                   block_mem,
                   sizeof(block_mem),
                   block_ccm,
                   sizeof(block_ccm));

    parameters = processor.mutable_parameters();

    processor.set_playback_mode(PLAYBACK_MODE_GRANULAR);
    processor.set_quality(0);

    InitParams();

    //Process all params once to set intial state
    for(int i = 0; i < NUM_PARAMS; i++)
    {
        param_controls[i].Process();
    }

    current_display_page = PARAMETERS1TO3;

    field.StartAdc();
    field.StartAudio(AudioCallback);

    while(1)
    {
        processor.Prepare();
        UpdateOled();
        UpdateLeds();
    }
}

void RenderParamsPage(DISPLAY_PAGE page)
{
    int offset = 0;
    for(int i = 0; i < NUM_PARAMS; i++)
    {
        if(param_controls[i].GetDisplayPage() == page)
        {
            field.display.SetCursor(LEFT_BORDER_WIDTH,
                                    (offset + 1) * ROW_HEIGHT);
            field.display.WriteString(
                param_controls[i].GetName(), DEFAULT_FONT, true);

            field.display.DrawRect(PARAM_NAME_COL_WIDTH,
                                   (offset + 1) * ROW_HEIGHT,
                                   PARAM_NAME_COL_WIDTH + VALUE_BAR_WIDTH,
                                   ((offset + 1) * ROW_HEIGHT)
                                       + VALUE_BAR_HEIGHT,
                                   true);

            field.display.DrawRect(
                PARAM_NAME_COL_WIDTH,
                (offset + 1) * ROW_HEIGHT,
                PARAM_NAME_COL_WIDTH
                    + int(param_controls[i].GetValue() * VALUE_BAR_WIDTH),
                ((offset + 1) * ROW_HEIGHT) + VALUE_BAR_HEIGHT,
                true,
                true);

#ifdef SHOW_KNOB_VALUES
            // Show the knob vaules overlayed on the value bars
            field.display.SetCursor(PARAM_NAME_COL_WIDTH + 20,
                                    ((offset + 1) * ROW_HEIGHT) + 2);
            field.display.WriteString(
                param_controls[i].GetValueAsString(), Font_6x8, true);
#endif
            ++offset;
        }
    }
}

inline void RenderButtonPage1()
{
    field.display.SetCursor(LEFT_BORDER_WIDTH, ROW_HEIGHT);
    field.display.WriteString("Quality:", SMALL_FONT, true);
    field.display.SetCursor(LEFT_BORDER_WIDTH, ROW_2 * SKINNY_ROW_HEIGHT);
    field.display.DrawRect(0,
                           (ROW_2 * SKINNY_ROW_HEIGHT) - VALUE_BAR_MARGIN,
                           DISPLAY_WIDTH,
                           (ROW_2 * SKINNY_ROW_HEIGHT) + VALUE_BAR_HEIGHT,
                           true,
                           true);

    auto currentPlaybackQuality = processor.quality();

    switch(currentPlaybackQuality)
    {
        case PLAYBACK_QUAL_16B_ST:
            field.display.WriteString("16 bit Stereo", SMALL_FONT, false);
            break;
        case PLAYBACK_QUAL_16B_MO:
            field.display.WriteString("16 bit Mono", SMALL_FONT, false);
            break;
        case PLAYBACK_QUAL_8B_ST:
            field.display.WriteString("8bit u-law Stereo", SMALL_FONT, false);
            break;
        case PLAYBACK_QUAL_8B_MO:
            field.display.WriteString("8bit u-law Mono", SMALL_FONT, false);
            break;
        default: break;
    }

    field.display.SetCursor(LEFT_BORDER_WIDTH, ROW_3 * SKINNY_ROW_HEIGHT);
    field.display.WriteString("Playback Mode:", SMALL_FONT, true);
    field.display.SetCursor(LEFT_BORDER_WIDTH, ROW_4 * SKINNY_ROW_HEIGHT);
    field.display.DrawRect(0,
                           (ROW_4 * SKINNY_ROW_HEIGHT) - VALUE_BAR_MARGIN,
                           DISPLAY_WIDTH,
                           (ROW_4 * SKINNY_ROW_HEIGHT) + VALUE_BAR_HEIGHT,
                           true,
                           true);

    auto currentPlaybackMode = processor.playback_mode();
    switch(currentPlaybackMode)
    {
        case PLAYBACK_MODE_GRANULAR:
            field.display.WriteString("Granular", SMALL_FONT, false);
            break;
        case PLAYBACK_MODE_STRETCH:
            field.display.WriteString("Shift and Stretch", SMALL_FONT, false);
            break;
        case PLAYBACK_MODE_LOOPING_DELAY:
            field.display.WriteString("Looping Delay", SMALL_FONT, false);
            break;
        case PLAYBACK_MODE_SPECTRAL:
            field.display.WriteString("Spectral Processor", SMALL_FONT, false);
            break;
        default: break;
    }
}

inline void RenderButtonPage2()
{
    field.display.SetCursor(LEFT_BORDER_WIDTH, ROW_HEIGHT);

    field.display.WriteString("Shift:", SMALL_FONT, true);
    field.display.DrawRect(DISPLAY_WIDTH - CHECKBOX_SIZE - CHECKBOX_MARGIN,
                           (ROW_1 * SKINNY_ROW_HEIGHT) - 1,
                           DISPLAY_WIDTH - CHECKBOX_MARGIN,
                           (ROW_1 * SKINNY_ROW_HEIGHT) + CHECKBOX_SIZE,
                           true,
                           is_shifted);

    field.display.SetCursor(LEFT_BORDER_WIDTH, 2 * SKINNY_ROW_HEIGHT);

    field.display.WriteString("Freeze:", SMALL_FONT, true);
    field.display.DrawRect(DISPLAY_WIDTH - CHECKBOX_SIZE - CHECKBOX_MARGIN,
                           (ROW_2 * SKINNY_ROW_HEIGHT) - 1,
                           DISPLAY_WIDTH - CHECKBOX_MARGIN,
                           (ROW_2 * SKINNY_ROW_HEIGHT) + CHECKBOX_SIZE,
                           true,
                           processor.frozen());

    field.display.SetCursor(LEFT_BORDER_WIDTH, ROW_3 * SKINNY_ROW_HEIGHT);

    field.display.WriteString("Silent:", SMALL_FONT, true);
    field.display.DrawRect(DISPLAY_WIDTH - CHECKBOX_SIZE - CHECKBOX_MARGIN,
                           (ROW_3 * SKINNY_ROW_HEIGHT) - 1,
                           DISPLAY_WIDTH - CHECKBOX_MARGIN,
                           (ROW_3 * SKINNY_ROW_HEIGHT) + CHECKBOX_SIZE,
                           true,
                           is_silenced);

    field.display.SetCursor(LEFT_BORDER_WIDTH, ROW_4 * SKINNY_ROW_HEIGHT);
    field.display.WriteString("Bypass:", SMALL_FONT, true);

    field.display.DrawRect(DISPLAY_WIDTH - CHECKBOX_SIZE - CHECKBOX_MARGIN,
                           (ROW_4 * SKINNY_ROW_HEIGHT) - 1,
                           DISPLAY_WIDTH - CHECKBOX_MARGIN,
                           (ROW_4 * SKINNY_ROW_HEIGHT) + CHECKBOX_SIZE,
                           true,
                           is_bypassed);
}


void UpdateOled()
{
    field.display.Fill(false);
    field.display.DrawLine(0, 10, DISPLAY_WIDTH, 10, true);

    field.display.SetCursor(0, 0);
    switch(current_display_page)
    {
        case PARAMETERS1TO3:
            field.display.WriteString("Paramters Page 1", DEFAULT_FONT, true);
            RenderParamsPage(PARAMETERS1TO3);
            break;

        case PARAMETERS4TO6:
            field.display.WriteString("Paramters Page 2", DEFAULT_FONT, true);
            RenderParamsPage(PARAMETERS4TO6);
            break;

        case PARAMETERS7TO9:
            field.display.WriteString("Paramters Page 3", DEFAULT_FONT, true);
            RenderParamsPage(PARAMETERS7TO9);
            break;

        case BUTTONS1:
            field.display.WriteString("Buttons Page 1", DEFAULT_FONT, true);
            RenderButtonPage1();
            break;

        case BUTTONS2:
            field.display.WriteString("Buttons Page 2", DEFAULT_FONT, true);
            RenderButtonPage2();
            break;
        default: break;
    }

    field.display.Update();
}

void UpdateLeds()
{
    for(int i = 0; i < 8; i++)
    {
        field.led_driver.SetLed(param_controls[i].GetLed(),
                                param_controls[i].GetValue());
    }

    auto currentPlaybackMode = processor.playback_mode();

    field.led_driver.SetLed(
        LED_PLAYBACK_MODE_GRANULAR,
        currentPlaybackMode == PLAYBACK_MODE_GRANULAR ? 1.f : 0.5f);
    field.led_driver.SetLed(
        LED_PLAYBACK_MODE_STRETCH,
        currentPlaybackMode == PLAYBACK_MODE_STRETCH ? 1.f : 0.5f);
    field.led_driver.SetLed(
        LED_PLAYBACK_MODE_LOOPING_DELAY,
        currentPlaybackMode == PLAYBACK_MODE_LOOPING_DELAY ? 1.f : 0.5f);
    field.led_driver.SetLed(
        LED_PLAYBACK_MODE_SPECTRAL,
        currentPlaybackMode == PLAYBACK_MODE_SPECTRAL ? 1.f : 0.5f);

    auto currentPlaybackQuality = processor.quality();

    field.led_driver.SetLed(
        LED_PLAYBACK_QUAL_16B_ST,
        currentPlaybackQuality == PLAYBACK_QUAL_16B_ST ? 1.f : 0.5f);
    field.led_driver.SetLed(
        LED_PLAYBACK_QUAL_16B_MO,
        currentPlaybackQuality == PLAYBACK_QUAL_16B_MO ? 1.f : 0.5f);
    field.led_driver.SetLed(
        LED_PLAYBACK_QUAL_8B_ST,
        currentPlaybackQuality == PLAYBACK_QUAL_8B_ST ? 1.f : 0.5f);
    field.led_driver.SetLed(
        LED_PLAYBACK_QUAL_8B_MO,
        currentPlaybackQuality == PLAYBACK_QUAL_8B_MO ? 1.f : 0.5f);

    field.led_driver.SetLed(LED_FREEZE, processor.frozen() ? 1.f : 0.5f);
    field.led_driver.SetLed(LED_SILENCE, is_silenced ? 1.f : 0.5f);
    field.led_driver.SetLed(LED_BYPASS, is_bypassed ? 1.f : 0.5f);
    field.led_driver.SetLed(LED_SHIFT, is_shifted ? 1.f : 0.5f);

    field.led_driver.SwapBuffersAndTransmit();
}

int mod(int n, int m)
{
    return ((n % m) + m) % m;
}

void ProcessParams()
{
    if(!is_shifted)
    {
        for(int i = 0; i < 8; i++)
        {
            param_controls[i].Process();
            if(param_controls[i].IsChanged())
            {
                current_display_page = param_controls[i].GetDisplayPage();
            }
        }
    }
    else
    {
        //TODO: Smells funny, but works... sort of
        param_controls[8].Process();
        if(param_controls[8].IsChanged())
        {
            current_display_page = param_controls[8].GetDisplayPage();
        }
    }
}

void ProcessButtons()
{
    if(field.KeyboardRisingEdge(BUTTON_PLAYBACK_QUAL_16B_ST))
    {
        processor.set_quality(PLAYBACK_QUAL_16B_ST);
        current_display_page = BUTTONS1;
    }

    if(field.KeyboardRisingEdge(BUTTON_PLAYBACK_QUAL_16B_MO))
    {
        processor.set_quality(PLAYBACK_QUAL_16B_MO);
        current_display_page = BUTTONS1;
    }

    if(field.KeyboardRisingEdge(BUTTON_PLAYBACK_QUAL_8B_ST))
    {
        processor.set_quality(PLAYBACK_QUAL_8B_ST);
        current_display_page = BUTTONS1;
    }

    if(field.KeyboardRisingEdge(BUTTON_PLAYBACK_QUAL_8B_MO))
    {
        processor.set_quality(PLAYBACK_QUAL_8B_MO);
        current_display_page = BUTTONS1;
    }

    if(field.KeyboardRisingEdge(BUTTON_PLAYBACK_MODE_GRANULAR))
    {
        processor.set_playback_mode(PLAYBACK_MODE_GRANULAR);
        current_display_page = BUTTONS1;
    }

    if(field.KeyboardRisingEdge(BUTTON_PLAYBACK_MODE_STRETCH))
    {
        processor.set_playback_mode(PLAYBACK_MODE_STRETCH);
        current_display_page = BUTTONS1;
    }

    if(field.KeyboardRisingEdge(BUTTON_PLAYBACK_MODE_LOOPING_DELAY))
    {
        processor.set_playback_mode(PLAYBACK_MODE_LOOPING_DELAY);
        current_display_page = BUTTONS1;
    }

    if(field.KeyboardRisingEdge(BUTTON_PLAYBACK_MODE_SPECTRAL))
    {
        processor.set_playback_mode(PLAYBACK_MODE_SPECTRAL);
        current_display_page = BUTTONS1;
    }

    if(field.KeyboardRisingEdge(BUTTON_FREEZE))
    {
        processor.ToggleFreeze();
        current_display_page = BUTTONS2;
    }

    if(field.KeyboardRisingEdge(BUTTON_SILENCE))
    {
        is_silenced = !is_silenced;
        processor.set_silence(is_silenced);
        current_display_page = BUTTONS2;
    }

    if(field.KeyboardRisingEdge(BUTTON_BYPASS))
    {
        is_bypassed = !is_bypassed;
        processor.set_bypass(is_bypassed);
        current_display_page = BUTTONS2;
    }

    if(field.KeyboardRisingEdge(BUTTON_SHIFT))
    {
        is_shifted           = !is_shifted;
        current_display_page = BUTTONS2;
    }

    if(field.sw[0].RisingEdge())
    {
        current_display_page
            = DISPLAY_PAGE(mod(current_display_page - 1, NUM_PAGES));
    }

    if(field.sw[1].RisingEdge())
    {
        current_display_page
            = DISPLAY_PAGE(mod(current_display_page + 1, NUM_PAGES));
    }

    processor.set_silence(is_silenced);
    processor.set_bypass(is_bypassed);
}

void ProcessGatesAndTriggers()
{
    //Using CV1 in as a gate for Freeze
    //0.7f should map to 3.5 volts for HIGH state
    parameters->freeze = field.GetCvValue(field.CV_1) > 0.7f; 
    parameters->trigger = field.gate_in.Trig();
    parameters->gate    = field.gate_in.State();
}


void Controls()
{
    field.ProcessAllControls();
    ProcessParams();
    ProcessButtons();
    ProcessGatesAndTriggers();
}
