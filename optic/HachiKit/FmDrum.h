#ifndef FMDRUM_H
#define FMDRUM_H

#include "daisy_patch.h"
#include "daisysp.h"
#include <string>
#include "IDrum.h"
#include "Utility.h"
#include "Param.h"

using namespace daisy;
using namespace daisysp;

class FmDrum: public IDrum {

    public:
        // Number of settable parameters for this model.
        static const uint8_t PARAM_COUNT = 6;
        // This is the order params will appear in the UI.
        static const uint8_t PARAM_FREQUENCY = 0;
        static const uint8_t PARAM_RATIO = 1;
        static const uint8_t PARAM_MOD_AMT = 2;
        static const uint8_t PARAM_DECAY = 3;
        static const uint8_t PARAM_ATTACK = 4;
        static const uint8_t PARAM_ENV_CURVE = 5;

        void Init(std::string slot, float sample_rate);
        void Init(std::string slot, float sample_rate, float frequency, float ratio, float modAmount, float attack, float decay, float curve);
        float Process();
        void Trigger(float velocity);

        float GetParam(uint8_t param);
        float UpdateParam(uint8_t param, float value);
        void SetParam(uint8_t param, float value);
        void ResetParams();
        std::string GetParamString(uint8_t param);

        std::string Name() { return "Blank"; }
        std::string Slot() { return slot; }
        std::string GetParamName(uint8_t param) { return param < PARAM_COUNT ? paramNames[param] : ""; }

    private:
        std::string paramNames[PARAM_COUNT] = { "Freq", "Ratio", "Mod", "Dcy", "Atk", "Curve" };
        std::string slot;
        Param parameters[PARAM_COUNT];
        float velocity;
        Fm2 fm;
        AdEnv ampEnv;

};



#endif
