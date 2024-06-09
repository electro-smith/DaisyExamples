#ifndef IDRUM_H
#define IDRUM_H

using namespace daisy;
using namespace daisysp;

#include "daisy_patch.h"
#include "daisysp.h"
#include <string>


class IDrum {

    public:
        // Number of settable parameters for this model.
        static const uint8_t PARAM_COUNT = 0;

        virtual ~IDrum() {
            
        }

        /** Initialize model with default parameters.
         * \param sample_rate audio sample rate.
        */
        virtual void Init(std::string slot, float sample_rate) = 0;

        /**
         * Calculate the next sample value.
        */
        virtual float Process() = 0;

        /**
         * Trigger the sound from the beginning.
        */
        virtual void Trigger(float velocity) = 0;

        /** Get the current value of a parameter.
         * \param param index of the desired parameter (must be < PARAM_COUNT).
        */
        virtual float GetParam(uint8_t param) = 0;

        /** Set the value of a parameter, regardless of the current value.
         *  Useful for initializing values on creation, or for setting from snapshots.
         *  Value is whatever is appropriate for this parameter; allows more intuitive creation
         *  of default values.
         * 
         * \param param index of the desired parameter (must be <PARAM_COUNT).
         * \param value
        */
        virtual float UpdateParam(uint8_t param, float value) = 0;

        /**
         * Set the value of a param, skipping jitter and value jump checks.
        */
        virtual void SetParam(uint8_t param, float value) = 0;

        /**
         * ResetParams resets all params for editing without jumps.
        */
        virtual void ResetParams() = 0;

        virtual std::string Name() = 0;
        virtual std::string Slot() = 0;
        virtual std::string GetParamName(uint8_t param) = 0;

        /** Get the current value of a parameter as a readable string.
         * \param param index of the desired parameter (must be <PARAM_COUNT).
        */
        virtual std::string GetParamString(uint8_t param) = 0;

};


#endif
