#ifndef HHSOURCE68_H
#define HHSOURCE68_H

#include "daisy_patch.h"
#include "daisysp.h"
#include "IDrum.h"
#include "Param.h"

using namespace daisy;
using namespace daisysp;

class HhSource68: public IDrum {

    public:
        // Number of settable parameters for this model.
        static const uint8_t PARAM_COUNT = 3;
        static const uint8_t PARAM_MORPH = 0;
        static const uint8_t PARAM_HPF = 1;
        static const uint8_t PARAM_LPF = 2;

        // how many square waves make up the sound source
        static const uint8_t OSC_COUNT = 6;

        // the morph value at which the sound imitates a 606
        static const float MORPH_606_VALUE;
        // the morph value at which the sound imitates an 808
        static const float MORPH_808_VALUE;

        static const float HPF_MAX;
        static const float HPF_MIN;
        static const float LPF_MAX;
        static const float LPF_MIN;
        static const float GAIN_MAX;

        /** Initialize model with default parameters.
         * \param sample_rate audio sample rate.
        */
        void Init(std::string slot, float sample_rate);

        /** Initialize model with specified parameters.
         * \param sample_rate audio sample rate.
         * \param frequency oscillator frequency in hertz.
        */
        void Init(std::string slot, float sample_rate, float morph);

        float Process();
        void Trigger(float velocity);
        float Signal();
        float Cowbell(bool isLow);

        float GetParam(uint8_t param);
        std::string GetParamString(uint8_t param);
        float UpdateParam(uint8_t param, float value);
        void SetParam(uint8_t param, float value);
        void ResetParams();

        std::string Name() { return "Bd8"; }
        std::string Slot() { return slot; }
        std::string GetParamName(uint8_t param) { return param < PARAM_COUNT ? paramNames[param] : ""; }

    private:
        static const float freqs606[];
        static const float freqs808[];
        static const float MIN_FREQ_FACTOR;

        std::string paramNames[PARAM_COUNT] = { "Mrph", "HPF", "LPF" };
        std::string slot;
        Param parameters[PARAM_COUNT];

        float signal = 0.0f;
        float cowSignal = 0.0f;
        float lowCowSignal = 0.0f;
        float gain = 1.0;
        Oscillator* oscs[OSC_COUNT];
        Oscillator osc0, osc1, osc2, osc3, osc4, osc5;
        Svf hpf, lpf;

        void SetMorph(float morph);

};



#endif
