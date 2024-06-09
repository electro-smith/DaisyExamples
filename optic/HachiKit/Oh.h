#ifndef OH_H
#define OH_H

#include "daisy_patch.h"
#include "daisysp.h"
#include <string>
#include "IDrum.h"
#include "ISource.h"
#include "Utility.h"
#include "Param.h"
#include "HhSource68.h"
#include "AhdEnv.h"

using namespace daisy;
using namespace daisysp;

class Oh: public IDrum {

    public:
        // Number of settable parameters for this model.
        static const uint8_t PARAM_COUNT = 6;
        // This is the order params will appear in the UI.
        static const uint8_t PARAM_ATTACK = 0;
        static const uint8_t PARAM_HOLD = 1;
        static const uint8_t PARAM_DECAY = 2;
        // These are pass-thru params that belong to the sound source and aren't tracked in Oh
        static const uint8_t PARAM_LPF = 3;
        static const uint8_t PARAM_MORPH = 4;
        static const uint8_t PARAM_HPF = 5;

        void Init(std::string slot, float sample_rate);
        void Init(std::string slot, float sample_rate, float attack, float hold, float decay, HhSource68 *source, float morph, float hpf, float lpf);
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
        std::string paramNames[PARAM_COUNT] = { "Atk", "Hold", "Dcy", "Lpf", "Mrph", "Hpf" };
        std::string slot;
        Param parameters[PARAM_COUNT];
        float velocity;
        HhSource68 *source = NULL;
        AhdEnv env;

};



#endif
