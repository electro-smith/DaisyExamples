#ifndef COW8_H
#define COW8_H

#include "daisy_patch.h"
#include "daisysp.h"
#include <string>
#include "IDrum.h"
#include "ISource.h"
#include "Utility.h"
#include "Param.h"
#include "HhSource68.h"

using namespace daisy;
using namespace daisysp;

class Cow8: public IDrum {

    public:
        // Number of settable parameters for this model.
        static const uint8_t PARAM_COUNT = 4;
        // This is the order params will appear in the UI.
        static const uint8_t PARAM_ATTACK = 0;
        static const uint8_t PARAM_DECAY = 1;
        // These are pass-thru params that belong to the sound source and aren't tracked in Ch
        static const uint8_t PARAM_HPF = 2;
        static const uint8_t PARAM_LPF = 3;

        static const float HPF_MAX;
        static const float HPF_MIN;
        static const float LPF_MAX;
        static const float LPF_MIN;

        void Init(std::string slot, float sample_rate);
        void Init(std::string slot, float sample_rate, float attack, float decay, HhSource68 *source, float hpfCutoff, float lpfCutoff);
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
        std::string paramNames[PARAM_COUNT] = { "Atk", "Dcy", "Hpf", "Lpf" };
        std::string slot;
        Param parameters[PARAM_COUNT];
        float velocity;
        // ISource *source = NULL;
        HhSource68 *source = NULL;
        AdEnv env;
        Svf hpf, lpf;

};



#endif
