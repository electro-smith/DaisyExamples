#include "daisy_patch_sm.h"
#include "daisysp.h"
using namespace daisy;
using namespace patch_sm;
using namespace daisysp;
static DaisyPatchSM hw;

int pitch1 = 0;
int pitch2 = 0;


void quantize(float adcValue, int& mm)
{
    std::vector<std::pair<float, int>> thresholdsAndValues
        = {{0.09f, 0},   {0.2f, 1},    {0.39f, 2},   {0.52f, 3},   {0.699f, 4},
           {0.873f, 5},  {0.988f, 6},  {1.1f, 7},    {1.299f, 8},  {1.399f, 9},
           {1.529f, 10}, {1.699f, 11}, {1.873f, 12}, {1.988f, 13}, {2.1f, 14},
           {2.299f, 15}, {2.399f, 16}, {2.529f, 17}, {2.699f, 18}, {2.873f, 19},
           {2.988f, 20}, {3.1f, 21},   {3.299f, 22}, {3.399f, 23}, {3.529f, 24},
           {3.699f, 25}, {3.873f, 26}, {3.988f, 27}, {4.1f, 28},   {4.299f, 29},
           {4.399f, 30}, {4.529f, 31}, {4.699f, 32}, {4.873f, 33}, {4.988f, 34},
           {5.0f, 35}};

    float fraction = (adcValue + 0.028f)*3.3f;
    
    for(const auto& pair : thresholdsAndValues)
    {
        float threshold = pair.first;
        int   value     = pair.second;
        if(fraction < threshold)
        {
            mm = value;
            return;
        }
    }
}

struct sampHoldStruct
{
    SampleHold       sampHolds;
    SampleHold::Mode mode;
    float            output;

    void Process(bool trigger, float input, SampleHold::Mode mode)
    {
        output = sampHolds.Process(trigger, input, mode);
    }
};

sampHoldStruct sampHolds[2];

const int NUM_SCALES                   = 7;
const int NUM_NOTES                    = 37;
int       scale[NUM_SCALES][NUM_NOTES] = {
    //  {0,  2,  4,  5,  7,  9,  10, 12, 14, 16, 17, 19, 21, 22,
    //  24, 26, 28, 29, 31, 33, 34, 36, 36, 36, 36, 36, 36, 36}, //  mixolydian
    {12, 12, 24, 24, 12, 12, 36, 36, 12, 12, 24, 24, 24, 36, 36, 36, 36,
     36, 48, 48, 48, 48, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36}, //  minor
    {0,  2,  3,  5,  7,  9,  10, 12, 14, 15, 17, 19, 21, 22, 24, 26, 27,
     29, 31, 33, 34, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36}, //  minor

    {0,  3,  5,  7,  10, 12, 15, 17, 18, 19, 22, 24, 27, 29, 30,
     31, 34, 36, 36, 39, 41, 42, 43, 46, 48, 48, 48, 48, 48, 48}, // penta minor

    {0,  2,  3,  5,  7,  8,  11, 12, 14, 15, 17, 19, 20, 23, 24, 26, 27,
     29, 31, 32, 35, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36}, //  Harmonic Minor
    //{ 0, 1, 4, 5, 7, 8, 10, 12, 13, 16, 17, 19, 20, 22, 24, 25, 28, 29, 31, 32, 34, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36 },  //lydian dominant     2
    {0,  1,  3,  4,  6,  7,  9,  10, 12, 13, 15, 16, 18, 19, 21, 22, 24,
     25, 27, 28, 30, 31, 32, 34, 36, 36, 36, 36, 36, 36, 36, 36, 36}, // half diminish
    // diminished H-W-H       4
    {0,  2,  4,  6,  8,  10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34,
     36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66}, // whole tone
    {
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
        13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
        26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36} //  Chromatic
};


void EnvelopeCallback(uint16_t** output, size_t size)


{
    hw.ProcessAllControls();

    float rawCV1 = hw.GetAdcValue(CV_4);
    if(rawCV1 < 0.f)
    {
        rawCV1 *= -1.f; //   helps S&H stay and range
    }


    float rawCV2 = hw.GetAdcValue(CV_8);
    if(rawCV2 < 0.f)
    {
        rawCV2 *= -1.f;
    }

    bool trigONE = hw.gate_in_1.Trig(); //
    bool trigTWO = hw.gate_in_2.Trig();

    //  SAMPLE1
    sampHolds[0].Process(trigONE, rawCV1, SampleHold::MODE_SAMPLE_HOLD);
    // HOLD1
    float SAMP1 = sampHolds[0].output;

    // QUANTIZE1
    quantize(SAMP1, pitch1);
    float note1 = (scale[0][pitch1] * .083333f * .2f);

    //  SAMPLE2
    sampHolds[1].Process(trigTWO, rawCV2, SampleHold::MODE_SAMPLE_HOLD);
    float SAMP2 = sampHolds[1].output;
    quantize(SAMP2, pitch2);
    float note2 = (scale[0][pitch2] * .083333f * .2f);


    for(size_t i = 0; i < size; i++)
    {
        output[0][i] = note1 * 4095.0f;

        output[1][i] = note2 * 4095.0f;
    }
}


int main(void)
{
    hw.Init();
    hw.StartDac(EnvelopeCallback);
    while(1) {}
}
