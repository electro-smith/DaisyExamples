#ifndef CH_H
#define CH_H

#include "daisy_patch.h"
#include "daisysp.h"
#include <string>
#include "IDrum.h"
#include "ISource.h"
#include "Utility.h"

using namespace daisy;
using namespace daisysp;

class Ch: public IDrum {

    public:
        // Number of settable parameters for this model.
        static const uint8_t PARAM_COUNT = 2;
        // This is the order params will appear in the UI.
        static const uint8_t PARAM_ATTACK = 0;
        static const uint8_t PARAM_DECAY = 1;

        /** Initialize model with default parameters.
         * \param sample_rate audio sample rate.
        */
        void Init(float sample_rate);

        /** Initialize model with specified parameters.
         * \param sample_rate audio sample rate.
         * \param frequency oscillator frequency in hertz.
        */
        // void Init(float sample_rate, float attack, float decay);
        void Init(float sample_rate, float attack, float decay, ISource *source);

        float Process();
        void Trigger(float velocity);

        /** Get the current value of a parameter.
         * \param param index of the desired parameter (must be <PARAM_COUNT).
        */
        float GetParam(uint8_t param);

        /** Get the current value of a parameter as a readable string.
         * \param param index of the desired parameter (must be <PARAM_COUNT).
        */
        std::string GetParamString(uint8_t param);

        /** Set the value of a parameter.
         * \param param index of the desired parameter (must be <PARAM_COUNT).
         * \param value
         * \param scaled when true, value is in the range 0-1.
        */
        float SetParam(uint8_t param, float value, bool scaled);

        std::string Name() { return "Blank"; }
        std::string Slot() { return slot; }
        std::string GetParamName(uint8_t param) { return param < PARAM_COUNT ? paramNames[param] : ""; }

    private:
        std::string paramNames[PARAM_COUNT] = { "Att", "Dec" };
        std::string slot;
        float velocity;
        float params[PARAM_COUNT];
        ISource *source = NULL;
        AdEnv env;

};



#endif
