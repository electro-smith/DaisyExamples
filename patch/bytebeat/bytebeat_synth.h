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
    void ProcessAudio(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size);
    void UpdateDisplay();

private:
    DaisyPatch* patch;
    float speed;
    uint32_t t;
    float tScale;  // ✅ Declare tScale
    int formula_index;
    static const int formula_count = 6;

    int a, b, c; // Bytebeat parameters

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