
#include "daisy_patch.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

class vox {
private:
    Oscillator _osc;

public:
    void Init(float sampleRate, DaisyPatch &patch) {
        _osc.Init(sampleRate);
        _osc.SetWaveForm(Oscillator::WAVE_SAW);
    }

    void Read(int pitchPin, int freqPin) {
        // fmap
        oscFrqKnob = (1023.0 - )
    }
    // vox(/* args */);
    // ~vox();

};

// vox::vox(/* args */)
// {
// }

// vox::~vox()
// {
// }
