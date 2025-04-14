#ifndef BLANK_H
#define BLANK_H

#include "daisy_patch.h"
#include "daisysp.h"
#include <string>
#include "IDrum.h"
#include "Utility.h"
#include "Param.h"

using namespace daisy;
using namespace daisysp;

class Blank: public IDrum {

    public:
        // Number of settable parameters for this model.
        static const uint8_t PARAM_COUNT = 3;
        // This is the order params will appear in the UI.
        static const uint8_t PARAM_FREQUENCY = 0;
        static const uint8_t PARAM_ATTACK = 1;
        static const uint8_t PARAM_DECAY = 2;

        void Init(std::string slot, float sample_rate);
        void Init(std::string slot, float sample_rate, float frequency, float attack, float decay);
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
        std::string paramNames[PARAM_COUNT] = { "Freq", "Atk", "Dcy" };
        std::string slot;
        Param parameters[PARAM_COUNT];
        float velocity;
        // audio objects

};



#endif
