#ifndef BD8_H
#define BD8_H

#include "daisy_patch.h"
#include "daisysp.h"
#include <string>
#include "IDrum.h"
#include "Utility.h"
#include "Param.h"

using namespace daisy;
using namespace daisysp;

class Bd8: public IDrum {

    public:
        // Number of settable parameters for this model.
        static const uint8_t PARAM_COUNT = 6;
        // This is the order params will appear in the UI.
        static const uint8_t PARAM_FREQUENCY = 0;
        static const uint8_t PARAM_MOD_AMT = 1;
        static const uint8_t PARAM_AMP_DECAY = 2;
        static const uint8_t PARAM_PITCH_DECAY = 3;
        static const uint8_t PARAM_AMP_ATTACK = 4;
        static const uint8_t PARAM_PITCH_ATTACK = 5;
        // TODO: add aCurve and pCurve

        void Init(std::string slot, float sample_rate);
        void Init(std::string slot, float sample_rate, float frequency, float ampAttack, float ampDecay, float pitchAttack, float pitchDecay, float modAmount);
        float Process();
        void Trigger(float velocity);

        float GetParam(uint8_t param);
        std::string GetParamString(uint8_t param);
        float UpdateParam(uint8_t param, float value);
        void SetParam(uint8_t param, float value);
        void ResetParams();

        std::string Name() { return "Bd8"; }
        std::string Slot() { return slot; }
        std::string GetParamName(uint8_t param) { return param < PARAM_COUNT ? paramNames[param] : ""; }

    private:
        std::string paramNames[PARAM_COUNT] = { "Freq", "Mod", "aDcy", "pDcy", "aAtk", "pAtk" };
        std::string slot;
        Param parameters[PARAM_COUNT];
        Oscillator osc;
        AdEnv ampEnv;
        AdEnv pitchEnv;
        float velocity;

};



#endif
