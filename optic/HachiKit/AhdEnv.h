#ifndef AHDENV_H
#define AHDENV_H

#include "daisy_patch.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

class AhdEnv {

    public:
        void Init(float sample_rate);
        float Process();
        void Trigger();
        void SetAttack(float time);
        void SetHold(float time);
        void SetDecay(float time);
        void SetCurve(float scalar);
        bool IsRunning() const;

    private:
        AdEnv ad;
        Adsr adsr;
};



#endif
