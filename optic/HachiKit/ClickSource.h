#ifndef CLICKSOURCE_H
#define CLICKSOURCE_H

#include "daisy_patch.h"
#include "daisysp.h"
#include <string>
#include "IDrum.h"
#include "ISource.h"
#include "Utility.h"
#include "Param.h"

using namespace daisy;
using namespace daisysp;

class ClickSource: public IDrum {

    public:
        // Number of settable parameters for this model.
        static const uint8_t PARAM_COUNT = 3;
        // This is the order params will appear in the UI.
        static const uint8_t PARAM_HPF = 0;
        static const uint8_t PARAM_LPF = 1;
        static const uint8_t PARAM_LPF_MOD = 2;

        void Init(std::string slot, float sample_rate);
        void Init(std::string slot, float sample_rate, float hpfFreq, float lpfFreq, float mod);
        float Process();
        void Trigger(float velocity);
        float Signal() { return signal; }

        float GetParam(uint8_t param);
        float UpdateParam(uint8_t param, float value);
        void SetParam(uint8_t param, float value);
        void ResetParams();
        std::string GetParamString(uint8_t param);

        std::string Name() { return "Tom"; }
        std::string Slot() { return slot; }
        std::string GetParamName(uint8_t param) { return param < PARAM_COUNT ? paramNames[param] : ""; }

    private:
        std::string paramNames[PARAM_COUNT] = { "Hpf", "Lpf", "fMod" };
        std::string slot;
        Param parameters[PARAM_COUNT];
        float velocity;
        float signal;
        WhiteNoise noise;
        Svf hpf, lpf;
        AdEnv lpfEnv;

};



#endif
