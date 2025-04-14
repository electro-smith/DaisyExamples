#ifndef SDNOISE_H
#define SDNOISE_H

#include "daisy_patch.h"
#include "daisysp.h"
#include <string>
#include "IDrum.h"
#include "Param.h"

using namespace daisy;
using namespace daisysp;


class SdNoise: public IDrum {

    public:
        // Number of settable parameters for this model.
        static const uint8_t PARAM_COUNT = 3;
        // This is the order params will appear in the UI.
        static const uint8_t PARAM_ATTACK = 0;
        static const uint8_t PARAM_DECAY = 1;
        static const uint8_t PARAM_CURVE = 2;

        void Init(std::string slot, float sample_rate);
        void Init(std::string slot, float sample_rate, float attack, float decay, float curve);
        float Process();
        void Trigger(float velocity);

        float GetParam(uint8_t param);
        float UpdateParam(uint8_t param, float value);
        void SetParam(uint8_t param, float value);
        void ResetParams();
        std::string GetParamString(uint8_t param);

        std::string Name() { return "SdNoise"; }
        std::string Slot() { return slot; }
        std::string GetParamName(uint8_t param) { return param < PARAM_COUNT ? paramNames[param] : ""; }

    private:
        std::string paramNames[PARAM_COUNT] = { "Atk", "Dcy", "Crv" };
        std::string slot;
        Param parameters[PARAM_COUNT];
        float velocity;
        WhiteNoise noise;
        AdEnv ampEnv;

};



#endif
