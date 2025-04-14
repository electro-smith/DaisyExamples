/**
 * Param tracks the value of a model parameter in response to a control (e.g. analog pot)
 * - ignores small changes to avoid jitter.
 * - doesn't update the stored value until the input has been swept past that value, to avoid jumps.
 * 
 * Raw value is between 0 and 1. Scaled value can be in any range; Param does not compute the
 * scaling, so must be managed externally.
 * 
*/
#ifndef PARAM_H
#define PARAM_H

#include "daisy_patch.h"
#include "daisysp.h"
#include "Utility.h"

using namespace daisy;
using namespace daisysp;

class Param {

    public:

        Param() {
            Reset();
        }

        /**
         * GetScaledValue returns the stored scaled value.
        */
        float GetScaledValue() {
            return scaled;
        }

        /**
         * SetScaledValue stores the value and overrides the jitter and jump checking.
        */
        void SetScaledValue(float scaled) {
            this->scaled = scaled;
            Reset();
        }

        /**
         * Reset sets the Param back to a state where it is waiting for control.
        */
        void Reset() {
            raw = -1.0f;   // any value (0-1) will appear as a change
            active = lower = higher = false;
        }

        /**
         * Update checks the raw value for jitter and prevents jump in value.
         * Should be called with both the raw value and the computed scaled value.
         * 
         * Returns the scaled value.
        */
        float Update(float raw, float scaled) {
            if (!active) {
                if (scaled <= this->scaled || (raw <= 0.01 && raw >= 0.0f)) lower = true;
                if (scaled >= this->scaled || raw >= 0.99) higher = true;
                active = (lower && higher);
            }
            if (active && std::abs(raw - this->raw) > delta) {
                this->raw = raw;
                this->scaled = scaled;
            }
            return this->scaled;
        }

    private:
        float delta = 0.001f;
        float raw = 0.0f;
        float scaled = 0.0f;
        bool active = false;
        bool lower = false;
        bool higher = false;

};


#endif
