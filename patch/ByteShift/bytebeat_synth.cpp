#include "bytebeat_synth.h"

// Global instance pointer for static callback
static BytebeatSynth* synthInstance = nullptr;

// Bytebeat Function Table
BytebeatSynth::BytebeatFunc BytebeatSynth::formulaTable[] = {
    &BytebeatSynth::BytebeatFormula0, 
    &BytebeatSynth::BytebeatFormula1, 
    &BytebeatSynth::BytebeatFormula2,
    &BytebeatSynth::BytebeatFormula3, 
    &BytebeatSynth::BytebeatFormula4, 
    &BytebeatSynth::BytebeatFormula5
};

// Bytebeat Equation Definitions
uint8_t BytebeatSynth::BytebeatFormula0(uint32_t t, BytebeatSynth* synth) { 
    int a = synth->a;
    int b = synth->b;
    return (t * (t >> a) & 42) | (t * (t >> b) & 84);
}

uint8_t BytebeatSynth::BytebeatFormula1(uint32_t t, BytebeatSynth* synth) {
    int a = synth->a;
    int b = synth->b;
    int c = synth->c;
    return (t >> a) | (t << b) | (t * (t >> c)); 
}

uint8_t BytebeatSynth::BytebeatFormula2(uint32_t t, BytebeatSynth* synth) {
    int a = synth->a;  // Stored in a register (if optimized)
    int b = synth->b; 
    return (t * (t >> a) | (t * (t >> b) & 50)); 
}

uint8_t BytebeatSynth::BytebeatFormula3(uint32_t t, BytebeatSynth* synth) {
    int b = synth->b;
    int c = synth->c; 
    return ((t * 3) & (t >> b)) | (t * (t >> c)); 
}

uint8_t BytebeatSynth::BytebeatFormula4(uint32_t t, BytebeatSynth* synth) { 
    int a = synth->a;  // Stored in a register (if optimized)
    int b = synth->b;
    return (t * 5 & (t >> a)) | (t * (t >> b) & 123); 
}

uint8_t BytebeatSynth::BytebeatFormula5(uint32_t t, BytebeatSynth* synth) {
    int a = synth->a;  // Stored in a register (if optimized)
    int b = synth->b; 
    return (t >> a) ^ (t * (t >> b) & 32); 
}

// Initialize Synth
void BytebeatSynth::Init(DaisyPatch* p) {
    patch = p;
    t = 0;
    formula_index = 0;
    synthInstance = this;

    // Initialize Parameters
    a = 4; 
    b = 6; 
    c = 8;

    patch->Init();
    patch->StartAdc();
}

// Update Controls (Knobs & Encoder)
void BytebeatSynth::UpdateControls() {
    patch->ProcessAnalogControls();
    patch->ProcessDigitalControls();

    float pitchCV = patch->GetKnobValue(DaisyPatch::CTRL_1) * 5.0f; 
    float frequency = 16.0f * powf(2.0f, pitchCV - 1.0f); 
    tScale = 48000.0f / frequency; 

    a = (int)(patch->GetKnobValue(DaisyPatch::CTRL_2) * 16) + 1;
    b = (int)(patch->GetKnobValue(DaisyPatch::CTRL_3) * 32) + 1;
    c = (int)(patch->GetKnobValue(DaisyPatch::CTRL_4) * 64) + 1;

    if (patch->encoder.RisingEdge()) {
        formula_index = (formula_index + 1) % formula_count;
    }
}

// Generate a single sample for external processing
float BytebeatSynth::GenerateSample() {
    BytebeatFunc currentFormula = formulaTable[formula_index];

    static float tAccumulator = 0;
    tAccumulator += 1.0f;
    if (tAccumulator >= tScale) {
        t += (uint32_t)(tScale);
        tAccumulator -= tScale;
    }

    uint8_t sample = currentFormula(t, this);
    return ((float)sample / 255.0f) * 2.0f - 1.0f;
}

void BytebeatSynth::NextFormula() {
    formula_index = (formula_index + 1) % formula_count;
}

// OLED Display Update
void BytebeatSynth::UpdateDisplay() {
    static int last_formula = -1;
    static int last_a = -1, last_b = -1, last_c = -1;

    if (formula_index != last_formula || a != last_a || b != last_b || c != last_c) {
        patch->display.Fill(false);
        patch->display.SetCursor(0, 0);
        patch->display.WriteString("Bytebeat Synth", Font_7x10, true);

        char buffer[32];
        snprintf(buffer, sizeof(buffer), "Formula: %d", formula_index);
        patch->display.SetCursor(0, 12);
        patch->display.WriteString(buffer, Font_7x10, true);

        snprintf(buffer, sizeof(buffer), "a: %d b: %d c: %d", a, b, c);
        patch->display.SetCursor(0, 24);
        patch->display.WriteString(buffer, Font_7x10, true);

        patch->display.Update();

        last_formula = formula_index;
        last_a = a;
        last_b = b;
        last_c = c;
    }
}