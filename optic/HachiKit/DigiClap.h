#ifndef DIGICLAP_H
#define DIGICLAP_H

#include "daisy_patch.h"
#include "daisysp.h"
#include <string>
#include "IDrum.h"
#include "Utility.h"
#include "Param.h"

using namespace daisy;
using namespace daisysp;

class DigiClap: public IDrum {

    public:
        // Number of settable parameters for this model.
        static const uint8_t PARAM_COUNT = 3;
        // This is the order params will appear in the UI.
        static const uint8_t PARAM_SPREAD = 0;
        static const uint8_t PARAM_DECAY = 1;
        static const uint8_t PARAM_FREQUENCY = 2;

        static const uint8_t REPEATS = 3;

        void Init(std::string slot, float sample_rate);
        void Init(std::string slot, float sample_rate, float spread, float decay, float frequency);
        float Process();
        void Trigger(float velocity);

        float GetParam(uint8_t param);
        std::string GetParamString(uint8_t param);
        float UpdateParam(uint8_t param, float value);
        void SetParam(uint8_t param, float value);
        void ResetParams();

        std::string Name() { return "Blank"; }
        std::string Slot() { return slot; }
        std::string GetParamName(uint8_t param) { return param < PARAM_COUNT ? paramNames[param] : ""; }

    private:
        std::string paramNames[PARAM_COUNT] = { "Sprd", "Dcy", "Freq" };
        std::string slot;
        Param parameters[PARAM_COUNT];
        float velocity;
        float repeat;
        // audio objects
        ClockedNoise clockedNoise;
        AdEnv env;

        float SetParam(uint8_t param, float value, bool isRaw);

};



#endif
