#include "bytebeat_synth.h"

// *******************************************************************
//
// Bytebeat formulas
//
// *******************************************************************

// Bytebeat Formulas (Function Pointer Table)
BytebeatSynth::BytebeatFunc BytebeatSynth::formulaTable[] = {
    &BytebeatSynth::BytebeatFormula0, 
    &BytebeatSynth::BytebeatFormula1, 
    &BytebeatSynth::BytebeatFormula2,
    &BytebeatSynth::BytebeatFormula3, 
    &BytebeatSynth::BytebeatFormula4, 
    &BytebeatSynth::BytebeatFormula5
};


// Bytebeat Equation Definitions (Now use a, b, c)
uint8_t BytebeatSynth::BytebeatFormula0(uint32_t t, BytebeatSynth* synth) { 
    int a = synth->a;  
    int b = synth->b;

    uint8_t result = (t * (t >> a) & 42) | (t * (t >> b) & 84);
    return result;
}

uint8_t BytebeatSynth::BytebeatFormula1(uint32_t t, BytebeatSynth* synth) {
    int a = synth->a;  
    int b = synth->b;
    int c = synth->c;
    return (t >> a) | (t << b) | (t * (t >> c)); 
}

uint8_t BytebeatSynth::BytebeatFormula2(uint32_t t, BytebeatSynth* synth) {
    int a = synth->a;  
    int b = synth->b; 
    return (t * (t >> a) | (t * (t >> b) & 50)); 
}

uint8_t BytebeatSynth::BytebeatFormula3(uint32_t t, BytebeatSynth* synth) {
    int b = synth->b;
    int c = synth->c; 
    return ((t * 3) & (t >> b)) | (t * (t >> c)); 
}

uint8_t BytebeatSynth::BytebeatFormula4(uint32_t t, BytebeatSynth* synth) { 
    int a = synth->a;  
    int b = synth->b;
    return (t * 5 & (t >> a)) | (t * (t >> b) & 123); 
}

uint8_t BytebeatSynth::BytebeatFormula5(uint32_t t, BytebeatSynth* synth) {
    int a = synth->a;  
    int b = synth->b; 
    return (t >> a) ^ (t * (t >> b) & 32); 
}


// *******************************************************************
//
// Bytebeat Synth
//
// *******************************************************************

float BytebeatSynth::GenerateSample() {
    static uint32_t t = 0;
    t++; // Increment counter each sample
    
    // Get the current formula
    BytebeatFunc currentFormula = formulaTable[formulaIndex];
    // Calculate the formula
    uint8_t output = currentFormula(t, this);
    float sample = ((float)output / 255.0f) * 2.0f - 1.0f;
    
    // Basic Bytebeat formula
    // Don't lose this formula it sounded good. 
    // float sample = ((t * (t >> a | t >> b) & c & t >> 8)) / 255.0f;
    
    return sample;
}

void BytebeatSynth::UpdateControls(ControlManager& controlManager) {
    // Read control values
    a = (int)(controlManager.GetCtrl1() * 16) + 1;
    b = (int)(controlManager.GetCtrl2() * 32) + 1;
    c = (int)(controlManager.GetCtrl3() * 64) + 1;

    // Advance the formula index if the encoder is pressed.
    if (controlManager.IsEncoderPressed()) {
        formulaIndex = (formulaIndex + 1) % FORMULA_COUNT;
    }
    
}
