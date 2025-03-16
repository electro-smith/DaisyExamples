#ifndef BYTEBEAT_SYNTH_H
#define BYTEBEAT_SYNTH_H

#include "daisy_patch.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

class BytebeatSynth {
public:
    void Init(DaisyPatch* p);
    void UpdateControls();
    float GenerateSample();  // Now returns a single sample for processing
    void UpdateDisplay();
    float ProcessSample();
    void NextFormula();
    int GetFormulaIndex() const { return formula_index; }

private:
    DaisyPatch* patch;
    uint32_t t;
    int formula_index;
    static const int formula_count = 6;

    // Dynamic formula parameters
    int a, b, c;

    // Scaling factor for musically tuned timing
    float tScale;

    using BytebeatFunc = uint8_t (*)(uint32_t, BytebeatSynth*);
    static BytebeatFunc formulaTable[];

    static uint8_t BytebeatFormula0(uint32_t t, BytebeatSynth* synth);
    static uint8_t BytebeatFormula1(uint32_t t, BytebeatSynth* synth);
    static uint8_t BytebeatFormula2(uint32_t t, BytebeatSynth* synth);
    static uint8_t BytebeatFormula3(uint32_t t, BytebeatSynth* synth);
    static uint8_t BytebeatFormula4(uint32_t t, BytebeatSynth* synth);
    static uint8_t BytebeatFormula5(uint32_t t, BytebeatSynth* synth);
};

#endif // BYTEBEAT_SYNTH_H