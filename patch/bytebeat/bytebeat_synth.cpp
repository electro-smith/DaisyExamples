#include "bytebeat_synth.h"

// Global instance pointer for static callback
static BytebeatSynth* synthInstance = nullptr;

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
    int a = synth->a;  // Stored in a register (if optimized)
    int b = synth->b;

    uint8_t result = (t * (t >> a) & 42) | (t * (t >> b) & 84);
    return result;
}

uint8_t BytebeatSynth::BytebeatFormula1(uint32_t t, BytebeatSynth* synth) {
    int a = synth->a;  // Stored in a register (if optimized)
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

// Static Wrapper for Audio Callback
static void AudioCallbackWrapper(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
    synthInstance->ProcessAudio(in, out, size);
}


// **************************************
// 
// Initialize Synth
//
// **************************************
void BytebeatSynth::Init(DaisyPatch* p) {
    patch = p;
    speed = 0.1f;
    t = 0;
    tScale = 1.0f; // ✅ Initialize tScale
    formula_index = 0;
    synthInstance = this;

    // Initialize Parameters (scaled)
    a = 4; b = 6; c = 8;

    patch->Init();
    patch->StartAdc();
    patch->StartAudio(AudioCallbackWrapper);
}

// Update Controls (Knobs & Encoder)
void BytebeatSynth::UpdateControls() {
    patch->ProcessAnalogControls();
    patch->ProcessDigitalControls();

    // Read Speed (Knob 1) - scaled to 0.1 to 4.1
    speed = patch->GetKnobValue(DaisyPatch::CTRL_1) * 4.0f + 0.1f;

    // Read dynamic parameters (CTRL_2, CTRL_3, CTRL_4)
    // Map Knobs 2, 3, and 4 to parameters a, b, and c
    a = (int)(patch->GetKnobValue(DaisyPatch::CTRL_2) * 16) + 1;  // Range: 1 - 16
    b = (int)(patch->GetKnobValue(DaisyPatch::CTRL_3) * 32) + 1;  // Range: 1 - 32
    c = (int)(patch->GetKnobValue(DaisyPatch::CTRL_4) * 64) + 1;  // Range: 1 - 64

    // Check encoder button for formula switching
    if (patch->encoder.RisingEdge()) {
        formula_index = (formula_index + 1) % formula_count;
    }
}

// Audio Processing Callback
// void BytebeatSynth::ProcessAudio(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
//     UpdateControls();

//     BytebeatFunc currentFormula = formulaTable[formula_index];
//     int speed_int = static_cast<int>(speed * 10);

//     for (size_t i = 0; i < size; i++) {
//         uint8_t sample = currentFormula(t, synthInstance);
//         float output = ((float)sample / 255.0f) * 2.0f - 1.0f;
//         out[0][i] = output;
//         out[1][i] = output;
//         t += 1 + speed_int;
//     }
// }

// *****************************************************
// 
// Process Audio 
// 
// *****************************************************

// Constants for min/max frequency
constexpr float MIN_FREQUENCY = 24.0f;   // 8.0 Lower limit (adjust as needed)
constexpr float MAX_FREQUENCY = 112.0f; // Upper limit (adjust as needed)

void BytebeatSynth::ProcessAudio(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
    UpdateControls();

    BytebeatFunc currentFormula = formulaTable[formula_index];

    // Read the pitch CV and scale it within the min/max frequency range
    float pitchCV = (1.0f - patch->GetKnobValue(DaisyPatch::CTRL_1)) * 5.0f; // Map 0-5V
    float frequency = MIN_FREQUENCY * powf(2.0f, pitchCV * log2f(MAX_FREQUENCY / MIN_FREQUENCY)); // Constrain range

    // Convert frequency to a time step
    uint32_t tScale = static_cast<uint32_t>(48000.0f / frequency);

    for (size_t i = 0; i < size; i++) {
        t += tScale; // Step `t` according to the musical pitch scaling

        // Generate bytebeat sample at updated `t`
        uint8_t sample = currentFormula(t, this);
        float output = ((float)sample / 255.0f) * 2.0f - 1.0f;

        // Send output
        out[0][i] = output;
        out[1][i] = output;
    }
}


// *************************************************
// 
// Update Display 
// 
// *************************************************

// OLED Display Update (Now Shows a, b, c)
void BytebeatSynth::UpdateDisplay() {
    static int last_speed = -1;
    static int last_formula = -1;
    static int last_a = -1, last_b = -1, last_c = -1;

    int speed_int = static_cast<int>(speed * 100);

    if (speed_int != last_speed || formula_index != last_formula || a != last_a || b != last_b || c != last_c) {
        patch->display.Fill(false);
        patch->display.SetCursor(0, 0);
        patch->display.WriteString("Bytebeat Synth", Font_7x10, true);

        char buffer[32];
        snprintf(buffer, sizeof(buffer), "Speed: %d", speed_int);
        patch->display.SetCursor(0, 12);
        patch->display.WriteString(buffer, Font_7x10, true);

        snprintf(buffer, sizeof(buffer), "Formula: %d", formula_index);
        patch->display.SetCursor(0, 24);
        patch->display.WriteString(buffer, Font_7x10, true);

        // Display a, b, and c values
        snprintf(buffer, sizeof(buffer), "a: %d b: %d c: %d", a, b, c);
        patch->display.SetCursor(0, 36);
        patch->display.WriteString(buffer, Font_7x10, true);

        patch->display.Update();
        
        last_speed = speed_int;
        last_formula = formula_index;
        last_a = a;
        last_b = b;
        last_c = c;
    }
}