
// An app for Daisy Patch that creates a Bytebeat synthesizer.

#include "daisy_patch.h"
#include "control_manager.h"
#include "bytebeat_synth.h"
#include "byteshift.h"

using namespace daisy;

DaisyPatch patch;
ControlManager controlManager;
BytebeatSynth bytebeat;
ByteShift byteshift;

// *******************************************************************
//
// ⚡ Audio callback function
//
// *******************************************************************

void AudioCallback(const float* const* in, float** out, size_t size) {
    for (size_t i = 0; i < size; i++) {
        // TODO: pass the control values to Generate sample to decouple controlManager
        float rawSample = bytebeat.GenerateSample();  // Bytebeat audio
        float shiftedSample = byteshift.ProcessSample(rawSample);  // Apply pitch shifting

        out[0][i] = shiftedSample;  // Left Channel
        out[1][i] = shiftedSample;  // Right Channel
    }
}


// *******************************************************************
// 
// Main
// 
// *******************************************************************

int main(void) {
    // initialize classes
    patch.Init();
    controlManager.Init(&patch);
    bytebeat.Init(&patch); // TODO: Does this need patch? 
    byteshift.Init(patch.AudioSampleRate());

    patch.StartAudio(AudioCallback);

    while (true) {
        controlManager.Update();
        bytebeat.UpdateControls(controlManager);  // Update Bytebeat with controls
        byteshift.SetPitchShift(controlManager.GetEncoderCount());

        // Clear the display and show the app name
        patch.display.Fill(false);
        patch.display.SetCursor(0, 0);
        patch.display.WriteString("ByteShift v1.0", Font_7x10, true);

        char buffer[32];
        
        // Display control values 
        snprintf(buffer, sizeof(buffer), "a: %d", bytebeat.a);
        patch.display.SetCursor(0, 12);
        patch.display.WriteString(buffer, Font_7x10, true);

        snprintf(buffer, sizeof(buffer), "b: %d", (int)(controlManager.GetCtrl2() * 100));
        patch.display.SetCursor(42, 12);
        patch.display.WriteString(buffer, Font_7x10, true);

        snprintf(buffer, sizeof(buffer), "c: %d", (int)(controlManager.GetCtrl3() * 100));
        patch.display.SetCursor(84, 12);
        patch.display.WriteString(buffer, Font_7x10, true);

        // Display pitche shift 
        snprintf(buffer, sizeof(buffer), "Pitch: %d", controlManager.GetEncoderCount());
        patch.display.SetCursor(0, 24);
        patch.display.WriteString(buffer, Font_7x10, true);

        // Display Bytebeat formula
        snprintf(buffer, sizeof(buffer), "Formula: %d", bytebeat.formulaIndex);
        patch.display.SetCursor(0, 36);
        patch.display.WriteString(buffer, Font_7x10, true);

        patch.display.Update();
    }
}