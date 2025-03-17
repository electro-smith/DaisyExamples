#ifndef BYTEBEAT_SYNTH_H
#define BYTEBEAT_SYNTH_H

#include "daisy_patch.h"
#include "control_manager.h"


// *******************************************************************
//
// Bytebeat Synth h
//
// *******************************************************************

class BytebeatSynth {
public:
    void Init(daisy::DaisyPatch* p) { patch = p; }
    float GenerateSample();  
    void UpdateControls(ControlManager& controlManager);

    int a, b, c, formulaIndex;

    static const int FORMULA_COUNT = 6;

private:
    daisy::DaisyPatch* patch;

    // Bytebeat formulas
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