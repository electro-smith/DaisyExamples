#ifndef TOM_H
#define TOM_H

#include "daisy_patch.h"
#include "daisysp.h"
#include <string>
#include "IDrum.h"
#include "ISource.h"
#include "Utility.h"
#include "Param.h"
#include "ClickSource.h"

using namespace daisy;
using namespace daisysp;

class Tom: public IDrum {

    public:
        // Number of settable parameters for this model.
        static const uint8_t PARAM_COUNT = 6;
        // This is the order params will appear in the UI.
        static const uint8_t PARAM_FREQUENCY = 0;
        static const uint8_t PARAM_AMP_DECAY = 1;
        static const uint8_t PARAM_PITCH_MOD = 2;
        // These are pass-thru params that belong to the click source and aren't tracked in Tom
        static const uint8_t PARAM_LPF_MOD = 3;
        static const uint8_t PARAM_HPF = 4;
        static const uint8_t PARAM_LPF = 5;

        void Init(std::string slot, float sample_rate);
        void Init(std::string slot, float sample_rate, float frequency, ClickSource *clickSource);
        float Process();
        void Trigger(float velocity);

        float GetParam(uint8_t param);
        float UpdateParam(uint8_t param, float value);
        void SetParam(uint8_t param, float value);
        void ResetParams();
        std::string GetParamString(uint8_t param);

        std::string Name() { return "Tom"; }
        std::string Slot() { return slot; }
        std::string GetParamName(uint8_t param) { return param < PARAM_COUNT ? paramNames[param] : ""; }

    private:
        std::string paramNames[PARAM_COUNT] = { "Freq", "aDcy", "pMod", "fMod", "Hpf", "Lpf" };
        std::string slot;
        Param parameters[PARAM_COUNT];
        float velocity;
        Oscillator osc;
        AdEnv ampEnv, pitchEnv;
        ClickSource *clickSource;

};



#endif
