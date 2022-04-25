#include "daisy_field.h"
#include "daisysp.h"
#include "granular_processor.h"

//#define SHOW_KNOB_VALUES
#define TOGGLE_FREEZE_ON_HIGH //as opposed to Freezing only while the gate is open

#define AUDIO_BLOCK_SIZE 32 //DO NOT CHANGE!

#define MAIN_LOOP_DELAY 6                      //milliseconds
#define OLED_LED_UPDATE_DELAY 5                //in Frames not milliseconds
#define CV_FREEZE_UPDATE_DEBOUNCE_INTERVAL 500 //milliseconds
#define CV_FREEZE_TRIGGER_THRESHOLD 0.65f //TODO: make these last 2 parameters

#define NUM_KNOBS 8
#define NUM_PARAMS 9
#define NUM_PAGES 10

#define KNOB_CHANGE_TOLERANCE .001f
#define KNOB_CATCH_TOLERANCE .075f

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 60
#define HEADER_HEIGHT 10
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
#define MAPPED_PARAM_CONTAINER_WIDTH 18
#define MAX_SCOPE_HEIGHT 60

#define PARAM_BUFFER_SIZE 8

using namespace daisysp;
using namespace daisy;

enum DEVICE_STATE
{
    RUNNING,
    CV_MAPPING,
};

enum DISPLAY_PAGE
{
    SPLASH,
    PARAMETERS1TO3,
    PARAMETERS4TO6,
    PARAMETERS7TO9,
    BUTTONS1,
    BUTTONS2,
    CV_MAPPINGS2,
    CV_MAPPINGS3,
    CV_MAPPINGS4,
    SCOPE,
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
    LED_MAP_CV_2                    = 4,
    LED_MAP_CV_3                    = 5,
    LED_MAP_CV_4                    = 6,
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
    BUTTON_MAP_CV_2                    = 12,
    BUTTON_MAP_CV_3                    = 13,
    BUTTON_MAP_CV_4                    = 14,
    BUTTON_SHIFT                       = 15,
};

enum MAPPABLE_CVS
{
    NONE,
    CV2,
    CV3,
    CV4,
};

inline float Constrain(float var, float min, float max)
{
    if(var < min)
    {
        var = min;
    }
    else if(var > max)
    {
        var = max;
    }
    return var;
}

class ParamControl
{
  public:
    ParamControl() {}
    ~ParamControl() {}

    void Init(const char*    name,
              const char*    abbreviated_name,
              AnalogControl* knob,
              int            led,
              bool           shifted,
              bool           has_alternate,
              Parameters*    params,
              int            param_num,
              DISPLAY_PAGE   display_page,
              MAPPABLE_CVS   mapped_cv = NONE)
    {
        display_name_     = name;
        abbreviated_name_ = abbreviated_name;
        knob_             = knob;
        led_              = led;
        shifted_          = shifted;
        has_alternate_    = has_alternate;
        params_           = params;
        param_num_        = param_num;
        param_val_        = 0.5f;
        display_page_     = display_page;
        mapped_cv_        = mapped_cv;
        knob_val_         = 0.f;
        knob_val_changed_ = false;
    }

    bool HasParamChanged() { return param_val_changed_; }
    bool HasKnobChanged() { return knob_val_changed_; }
    bool HasAlternate() { return has_alternate_; }

    bool ControlChange(float newval, bool catch_val = true)
    {
        auto delta = fabsf(newval - param_val_);
        auto ret
            = delta > KNOB_CHANGE_TOLERANCE && delta < KNOB_CATCH_TOLERANCE;

        if(ret)
        {
            param_val_         = newval;
            param_val_changed_ = true;

#ifdef SHOW_KNOB_VALUES
            snprintf(val_str, PARAM_BUFFER_SIZE, "%d", int(param_val_ * 100));
#endif
        }
        else
        {
            param_val_changed_ = false;
        };
        return ret;
    }

    const char* GetDisplayName() { return display_name_; }
    const char* GetAbbreviatedName() { return abbreviated_name_; }
    int         GetLed() const { return led_; }
    float       GetValue() { return param_val_; }
    char*       GetValueAsString() { return val_str; }
    int         GetParamNum() { return param_num_; }
    bool        IsShifted() { return shifted_; }

    DISPLAY_PAGE GetDisplayPage() { return display_page_; }

    MAPPABLE_CVS GetMappedCV() { return mapped_cv_; }
    void         SetMappedCV(MAPPABLE_CVS mapped_cv) { mapped_cv_ = mapped_cv; }

    void Process()
    {
        float val;

        auto new_knob_val = knob_->Process();
        knob_val_changed_
            = fabsf(new_knob_val - knob_val_) > KNOB_CHANGE_TOLERANCE;
        knob_val_ = new_knob_val;

        if(mapped_cv_ == NONE)
        {
            val = new_knob_val;

            if(!ControlChange(val))
            {
                return;
            }
        }
        else
        {
            //Ignore the knob setting if parameter is mapped to a CV since param_val will be set by the CV
            //TODO: Use the knob as an offset to the incoming CV value
            val = param_val_;
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
    const char*    display_name_;
    const char*    abbreviated_name_;
    AnalogControl* knob_;
    int            led_;
    bool           shifted_;
    bool           has_alternate_;
    Parameters*    params_;
    int            param_num_;
    float          param_val_;
    bool           param_val_changed_;
    float          knob_val_;
    bool           knob_val_changed_;
    char           val_str[PARAM_BUFFER_SIZE];

    DISPLAY_PAGE display_page_;
    MAPPABLE_CVS mapped_cv_;
};

GranularProcessorClouds processor;
DaisyField              field;

// Pre-allocate big blocks in main memory and CCM. No malloc here.
uint8_t block_mem[118784];
uint8_t block_ccm[65536 - 128];

float        cpu_usage                        = 0.f;
char         cpu_usage_str[PARAM_BUFFER_SIZE] = "";
bool         param_val_changed_;
ParamControl param_controls[NUM_PARAMS];
Parameters*  parameters;
DEVICE_STATE current_device_state = RUNNING;
MAPPABLE_CVS currently_mapping_cv = NONE;
bool         can_map[4]           = {true};
int          oled_led_update_gate = 0;
DISPLAY_PAGE current_display_page;
bool is_silenced, is_bypassed, is_shifted, is_frozen_by_button, is_frozen_by_cv;
float scope_buffer[AUDIO_BLOCK_SIZE] = {0.f};

uint32_t last_freeze_cv_update;

void Controls();
void UpdateLEDs();
void UpdateOLED();
void ProcessButtons();

int Mod(int n, int m)
{
    return ((n % m) + m) % m;
}

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    uint32_t start = System::GetTick();

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
        out[0][i]       = output[i].l;
        out[1][i]       = output[i].r;
        scope_buffer[i] = (out[0][i] + out[1][i]) * .5f;
    }

    cpu_usage += 0.03f * (((System::GetTick() - start) / 200.f) - cpu_usage);
}

void InitParams()
{
    param_controls[0].Init("Position:",
                           "Po",
                           &field.knob[0],
                           DaisyField::LED_KNOB_1,
                           false,
                           false,
                           parameters,
                           0,
                           DISPLAY_PAGE::PARAMETERS1TO3);

    param_controls[1].Init("Size:",
                           "Sz",
                           &field.knob[1],
                           DaisyField::LED_KNOB_2,
                           false,
                           false,
                           parameters,
                           1,
                           DISPLAY_PAGE::PARAMETERS1TO3);

    param_controls[2].Init("Pitch:",
                           "Pi",
                           &field.knob[2],
                           DaisyField::LED_KNOB_3,
                           false,
                           false,
                           parameters,
                           2,
                           DISPLAY_PAGE::PARAMETERS1TO3);

    param_controls[3].Init("Density:",
                           "Dn",
                           &field.knob[3],
                           DaisyField::LED_KNOB_4,
                           false,
                           false,
                           parameters,
                           3,
                           DISPLAY_PAGE::PARAMETERS4TO6);

    param_controls[4].Init("Texture:",
                           "Tx",
                           &field.knob[4],
                           DaisyField::LED_KNOB_5,
                           false,
                           false,
                           parameters,
                           4,
                           DISPLAY_PAGE::PARAMETERS4TO6);

    param_controls[5].Init("Dry/Wet:",
                           "DW",
                           &field.knob[5],
                           DaisyField::LED_KNOB_6,
                           false,
                           false,
                           parameters,
                           5,
                           DISPLAY_PAGE::PARAMETERS4TO6);

    param_controls[6].Init("Spread:",
                           "Sp",
                           &field.knob[6],
                           DaisyField::LED_KNOB_7,
                           false,
                           false,
                           parameters,
                           6,
                           DISPLAY_PAGE::PARAMETERS7TO9);

    param_controls[7].Init("Feedback:",
                           "Fb",
                           &field.knob[7],
                           DaisyField::LED_KNOB_8,
                           false,
                           true,
                           parameters,
                           7,
                           DISPLAY_PAGE::PARAMETERS7TO9);

    param_controls[8].Init("Reverb:",
                           "Rv",
                           &field.knob[7],
                           DaisyField::LED_KNOB_8,
                           true,
                           true,
                           parameters,
                           8,
                           DISPLAY_PAGE::PARAMETERS7TO9);
}

int main(void)
{
    field.Init();
    field.SetAudioBlockSize(AUDIO_BLOCK_SIZE);
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

    //Process all params once to set inital state
    for(int i = NUM_PARAMS - 1; i >= 0; i--)
    {
        param_controls[i].Process();
    }

    current_display_page = SPLASH;
    UpdateOLED();

    //Delay for a second to show the splash screen
    System::Delay(1000);

    field.StartAdc();
    field.StartAudio(AudioCallback);

    while(1)
    {
        processor.Prepare();

        //Since we dont need to update the UI on every execution of the loop only update every nth iteration
        oled_led_update_gate
            = Mod(oled_led_update_gate + 1, OLED_LED_UPDATE_DELAY);
        if(oled_led_update_gate == OLED_LED_UPDATE_DELAY - 1)
        {
            UpdateLEDs();
            UpdateOLED();
        }

        //And we probably dont need to call Prepare so often so we can sleep a bit
        System::Delay(MAIN_LOOP_DELAY);
    }
}

void RenderParams(DISPLAY_PAGE page)
{
    int offset = 0;
    for(int i = 0; i < NUM_PARAMS; i++)
    {
        if(param_controls[i].GetDisplayPage() == page)
        {
            field.display.SetCursor(LEFT_BORDER_WIDTH,
                                    (offset + 1) * ROW_HEIGHT);
            field.display.WriteString(
                param_controls[i].GetDisplayName(), DEFAULT_FONT, true);

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

inline void RenderSplash()
{
    field.display.SetCursor(17, 20);
    field.display.WriteString("NIMBUS", Font_16x26, true);
}

inline void RenderCpuUsage()
{
    snprintf(cpu_usage_str,
             cpu_usage,
             "%02d%%",
             int(0.0001f * cpu_usage * (field.seed.AudioSampleRate())
                 / field.seed.AudioBlockSize()));
    field.display.SetCursor(80, 0);
    field.display.WriteString("CPU:", SMALL_FONT, true);
    field.display.SetCursor(105, 0);
    field.display.WriteString(cpu_usage_str, SMALL_FONT, true);
}

inline void RenderCVMappings(MAPPABLE_CVS cv)
{
    if(cv == NONE)
    {
        return;
    }

    int x_offset = 0;
    int y_offset = 1;
    field.display.SetCursor(LEFT_BORDER_WIDTH, ROW_HEIGHT);
    for(int i = 0; i < NUM_PARAMS; i++)
    {
        if(param_controls[i].GetMappedCV() == cv)
        {
            field.display.SetCursor(
                LEFT_BORDER_WIDTH + (x_offset * MAPPED_PARAM_CONTAINER_WIDTH)
                    + 3,
                (y_offset * ROW_HEIGHT) + 3);
            field.display.WriteString(
                param_controls[i].GetAbbreviatedName(), SMALL_FONT, true);
            field.display.DrawRect(
                LEFT_BORDER_WIDTH + (x_offset * MAPPED_PARAM_CONTAINER_WIDTH),
                (y_offset * ROW_HEIGHT),
                LEFT_BORDER_WIDTH + (x_offset * MAPPED_PARAM_CONTAINER_WIDTH)
                    + 16,
                (y_offset * ROW_HEIGHT) + 12,
                true);
            x_offset++;
            if(x_offset * MAPPED_PARAM_CONTAINER_WIDTH > DISPLAY_WIDTH / 2)
            {
                x_offset = 0;
                y_offset++;
            }
        }
    }
}

inline void RenderButtons1()
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

inline void RenderButtons2()
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

inline void RenderScope()
{
    int prev_x = 0;
    int prev_y = (DISPLAY_HEIGHT + HEADER_HEIGHT) / 2;
    for(size_t i = 0; i < AUDIO_BLOCK_SIZE; i++)
    {
        int y = std::min(std::max((DISPLAY_HEIGHT + HEADER_HEIGHT) / 2
                                      - int(scope_buffer[i] * MAX_SCOPE_HEIGHT),
                                  0),
                         DISPLAY_HEIGHT);
        int x = i * DISPLAY_WIDTH / AUDIO_BLOCK_SIZE;
        if(i != 0)
        {
            field.display.DrawLine(prev_x, prev_y, x, y, true);
        }
        prev_x = x;
        prev_y = y;
    }
}

void UpdateOLED()
{
    field.display.Fill(false);

    if(current_display_page != SPLASH)
    {
        field.display.DrawLine(
            0, HEADER_HEIGHT, DISPLAY_WIDTH, HEADER_HEIGHT, true);
        field.display.SetCursor(0, 0);
    }

    switch(current_display_page)
    {
        case SPLASH: RenderSplash(); break;

        case PARAMETERS1TO3:
            field.display.WriteString("Paramters Page 1", DEFAULT_FONT, true);
            RenderParams(PARAMETERS1TO3);
            break;

        case PARAMETERS4TO6:
            field.display.WriteString("Paramters Page 2", DEFAULT_FONT, true);
            RenderParams(PARAMETERS4TO6);
            break;

        case PARAMETERS7TO9:
            field.display.WriteString("Paramters Page 3", DEFAULT_FONT, true);
            RenderParams(PARAMETERS7TO9);
            break;

        case BUTTONS1:
            field.display.WriteString("Buttons Page 1", DEFAULT_FONT, true);
            RenderButtons1();
            break;

        case BUTTONS2:
            field.display.WriteString("Buttons Page 2", DEFAULT_FONT, true);
            RenderButtons2();
            break;

        case CV_MAPPINGS2:
            field.display.WriteString("CV2 Mappings", DEFAULT_FONT, true);
            RenderCVMappings(CV2);
            break;

        case CV_MAPPINGS3:
            field.display.WriteString("CV3 Mappings", DEFAULT_FONT, true);
            RenderCVMappings(CV3);
            break;

        case CV_MAPPINGS4:
            field.display.WriteString("CV4 Mappings", DEFAULT_FONT, true);
            RenderCVMappings(CV4);
            break;

        case SCOPE:
            field.display.WriteString("Scope", DEFAULT_FONT, true);
            RenderCpuUsage();
            RenderScope();
            break;

        default: break;
    }

    field.display.Update();
}

void UpdateLEDs()
{
    for(int i = 0; i < 8; i++)
    {
        if(param_controls[i].GetMappedCV() == NONE)
        {
            field.led_driver.SetLed(param_controls[i].GetLed(),
                                    param_controls[i].GetValue());
        }
        else
        {
            //LED is dark if the parameter is mapped to CV
            field.led_driver.SetLed(param_controls[i].GetLed(), 0.f);
        }
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

    field.led_driver.SetLed(LED_MAP_CV_2,
                            current_device_state == CV_MAPPING
                                    && currently_mapping_cv == CV2
                                ? 1.f
                                : 0.f);
    field.led_driver.SetLed(LED_MAP_CV_3,
                            current_device_state == CV_MAPPING
                                    && currently_mapping_cv == CV3
                                ? 1.f
                                : 0.f);
    field.led_driver.SetLed(LED_MAP_CV_4,
                            current_device_state == CV_MAPPING
                                    && currently_mapping_cv == CV4
                                ? 1.f
                                : 0.f);

    field.led_driver.SetLed(LED_FREEZE, processor.frozen() ? 1.f : 0.5f);
    field.led_driver.SetLed(LED_SILENCE, is_silenced ? 1.f : 0.5f);
    field.led_driver.SetLed(LED_BYPASS, is_bypassed ? 1.f : 0.5f);
    field.led_driver.SetLed(LED_SHIFT, is_shifted ? 1.f : 0.5f);

    field.led_driver.SwapBuffersAndTransmit();
}

void ProcessParam(ParamControl& pc, bool auto_page_change)
{
    pc.Process();

    switch(current_device_state)
    {
        case RUNNING:
            if(pc.HasKnobChanged())
            {
                //If control is mapped to CV then don't change the page when its value is changed
                if(pc.GetMappedCV() == NONE && auto_page_change)
                {
                    current_display_page = pc.GetDisplayPage();
                }
            }
            break;

        case CV_MAPPING:
            if(can_map[currently_mapping_cv] == true)
            {
                //The knob has changed from the last iteration so we are mapping
                if(pc.HasKnobChanged())
                {
                    //If control has not been mapped yet or mapped to another CV then map it to the new CV
                    if(pc.GetMappedCV() != currently_mapping_cv)
                    {
                        pc.SetMappedCV(currently_mapping_cv);
                        //Only allow one mapping action per key press
                        can_map[currently_mapping_cv] = false;
                    }
                    else
                    {
                        //If the control is already mapped and the knob has been moved then unmap it
                        pc.SetMappedCV(NONE);
                        //Only allow one mapping action per key press
                        can_map[currently_mapping_cv] = false;
                    }
                }
            }
            break;

        default: break;
    }
}

void ProcessParams(bool auto_page_change = true)
{
    for(int i = NUM_PARAMS - 1; i >= 0; i--)
    {
        if((is_shifted
            && (param_controls[i].IsShifted()
                || !param_controls[i].HasAlternate()))
           || (!is_shifted && !param_controls[i].IsShifted()))
        {
            ProcessParam(param_controls[i], auto_page_change);
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
        is_frozen_by_button = !is_frozen_by_button;
        processor.ToggleFreeze();
        //current_display_page = BUTTONS2;
    }

    if(field.KeyboardRisingEdge(BUTTON_SILENCE))
    {
        is_silenced = !is_silenced;
        processor.set_silence(is_silenced);
        //current_display_page = BUTTONS2;
    }

    if(field.KeyboardRisingEdge(BUTTON_BYPASS))
    {
        is_bypassed = !is_bypassed;
        processor.set_bypass(is_bypassed);
        //current_display_page = BUTTONS2;
    }

    if(field.KeyboardRisingEdge(BUTTON_SHIFT))
    {
        is_shifted = !is_shifted;
        //current_display_page = BUTTONS2;
    }

    if(field.sw[0].RisingEdge())
    {
        current_display_page
            = DISPLAY_PAGE(Mod(current_display_page - 1, NUM_PAGES));
    }

    if(field.sw[1].RisingEdge())
    {
        current_display_page
            = DISPLAY_PAGE(Mod(current_display_page + 1, NUM_PAGES));
    }

    if(field.KeyboardRisingEdge(BUTTON_MAP_CV_2))
    {
        current_device_state = CV_MAPPING;
        currently_mapping_cv = CV2;
        can_map[CV2]         = true;
        current_display_page = CV_MAPPINGS2;
    }

    if(field.KeyboardFallingEdge(BUTTON_MAP_CV_2))
    {
        current_device_state = RUNNING;
        currently_mapping_cv = NONE;
    }

    if(field.KeyboardRisingEdge(BUTTON_MAP_CV_3))
    {
        current_device_state = CV_MAPPING;
        currently_mapping_cv = CV3;
        can_map[CV3]         = true;
        current_display_page = CV_MAPPINGS3;
    }

    if(field.KeyboardFallingEdge(BUTTON_MAP_CV_3))
    {
        current_device_state = RUNNING;
        currently_mapping_cv = NONE;
    }

    if(field.KeyboardRisingEdge(BUTTON_MAP_CV_4))
    {
        current_device_state = CV_MAPPING;
        currently_mapping_cv = CV4;
        can_map[CV4]         = true;
        current_display_page = CV_MAPPINGS4;
    }

    if(field.KeyboardFallingEdge(BUTTON_MAP_CV_4))
    {
        current_device_state = RUNNING;
        currently_mapping_cv = NONE;
    }

    processor.set_silence(is_silenced);
    processor.set_bypass(is_bypassed);
}

void ProcessGatesTriggersCv()
{
    //Using CV1 in as a gate to freeze and unfreeze the processor
    //0.7f should map to 3.5 volts for HIGH state
    //Debounced
    if(!is_frozen_by_button)
    {
#ifdef TOGGLE_FREEZE_ON_HIGH
        //Has the debounce interval elapsed? If not then we just disregard this value
        if(System::GetNow()
           > last_freeze_cv_update + CV_FREEZE_UPDATE_DEBOUNCE_INTERVAL)
        {
            auto  cv1               = field.GetCv(field.CV_1);
            float new_freeze_cv_val = cv1->Process();
            if(new_freeze_cv_val > CV_FREEZE_TRIGGER_THRESHOLD)
            {
                //Toggle freeze for the processor if the gate is held high
                is_frozen_by_cv    = !is_frozen_by_cv;
                parameters->freeze = is_frozen_by_cv;
            }
            last_freeze_cv_update = System::GetNow();
        }
#else
        auto  cv1               = field.GetCv(field.CV_1);
        float new_freeze_cv_val = cv1->Process();
        //Only freeze the processor while the gate is held high
        parameters->freeze = is_frozen_by_cv
            = (new_freeze_cv_val > GATE_TRIGGER_THRESHOLD);
#endif
    }
    else
    {
        is_frozen_by_cv = false;
    }

    parameters->trigger = field.gate_in.Trig();
    parameters->gate    = field.gate_in.State();

    //Send CV to all the mapped parameters
    for(int i = 0; i < NUM_PARAMS; i++)
    {
        if(param_controls[i].GetMappedCV() != NONE)
        {
            float cv_value = field.GetCvValue(param_controls[i].GetMappedCV());
            float clamped_cv_value = Constrain(cv_value, 0.0f, 1.0f);
            param_controls[i].ControlChange(clamped_cv_value);
            param_controls[i].Process();
        }
    }
}

void Controls()
{
    field.ProcessAllControls();
    ProcessParams();
    ProcessGatesTriggersCv();
    ProcessButtons();
}