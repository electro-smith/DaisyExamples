#ifndef SD8_H
#define SD8_H

#include "daisy_patch.h"
#include "daisysp.h"
#include <string>
#include "IDrum.h"
#include "Utility.h"

using namespace daisy;
using namespace daisysp;

class Sd8: public IDrum {

    public:
        // Number of settable parameters for this model.
        static const uint8_t PARAM_COUNT = 6;
        // This is the order params will appear in the UI.
        static const uint8_t PARAM_OSC_FREQUENCY = 0;
        static const uint8_t PARAM_OSC_DECAY = 1;
        static const uint8_t PARAM_NOISE_DECAY = 2;
        static const uint8_t PARAM_MIX = 3;
        static const uint8_t PARAM_OSC_ATTACK = 4;
        static const uint8_t PARAM_NOISE_ATTACK = 5;
        static const std::string paramNames[];

        /** Initialize model with default parameters.
         * \param sample_rate audio sample rate.
        */
        void Init(float sample_rate);

        /** Initialize model with specified parameters.
         * \param sample_rate audio sample rate.
         * \param frequency oscillator frequency in hertz.
        */
        void Init(float sample_rate, float oscFrequency, float oscAttack, float oscDecay, float noiseAttack, float noiseDecay, float mix);

        float Process();
        void Trigger(float velocity);

        /** Get the current value of a parameter.
         * \param param index of the desired parameter (must be <PARAM_COUNT).
        */
        float GetParam(uint8_t param);

        /** Get the current value of a parameter as a readable string.
         * \param param index of the desired parameter (must be <PARAM_COUNT).
        */
        std::string GetParamString(uint8_t param);

        /** Set the value of a parameter.
         * \param param index of the desired parameter (must be <PARAM_COUNT).
         * \param value
         * \param scaled when true, value is in the range 0-1.
        */
        float SetParam(uint8_t param, float value, bool scaled);

        std::string Name() { return "Sd8"; }
        std::string Slot() { return slot; }
        std::string GetParamName(uint8_t param) { return param < PARAM_COUNT ? paramNames[param] : ""; }

    private:
        std::string slot;
        Oscillator osc;
        AdEnv oscEnv;
        WhiteNoise noise;
        AdEnv noiseEnv;
        float velocity;
        float params[PARAM_COUNT];

};



#endif
