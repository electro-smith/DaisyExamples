#ifndef SD8_H
#define SD8_H

#include "daisy_patch.h"
#include "daisysp.h"
#include <string>
#include "IDrum.h"
#include "Utility.h"
#include "Param.h"

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

        void Init(std::string slot, float sample_rate);
        void Init(std::string slot, float sample_rate, float oscFrequency, float oscAttack, float oscDecay, float noiseAttack, float noiseDecay, float mix);
        float Process();
        void Trigger(float velocity);

        float GetParam(uint8_t param);
        float UpdateParam(uint8_t param, float value);
        void SetParam(uint8_t param, float value);
        void ResetParams();
        std::string GetParamString(uint8_t param);

        std::string Name() { return "Sd8"; }
        std::string Slot() { return slot; }
        std::string GetParamName(uint8_t param) { return param < PARAM_COUNT ? paramNames[param] : ""; }

    private:
        static const std::string paramNames[PARAM_COUNT];
        std::string slot;
        Param parameters[PARAM_COUNT];
        float velocity;
        Oscillator osc;
        AdEnv oscEnv;
        WhiteNoise noise;
        AdEnv noiseEnv;

};



#endif
