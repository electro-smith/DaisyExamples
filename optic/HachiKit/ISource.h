#ifndef ISOURCE_H
#define ISOURCE_H

#include "daisy_patch.h"
#include "daisysp.h"
#include "Utility.h"

using namespace daisy;
using namespace daisysp;

class ISource {

    public:
        virtual void Init(float sample_rate) = 0;
        virtual float Process() = 0;
        virtual float Signal() = 0;

    private:

};



#endif
