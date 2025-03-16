#include "daisy_patch.h"
#include "control_manager.h"
#include "byteshift.h"

daisy::DaisyPatch patch;
ControlManager controlManager;
ByteShift synth;

static ByteShift* synthInstance = nullptr; // Pointer to ByteShift instance

void AudioCallbackWrapper(daisy::AudioHandle::InputBuffer in, 
                          daisy::AudioHandle::OutputBuffer out, 
                          size_t size) {
    if (synthInstance) {
        synthInstance->ProcessAudio(out, size);
    }
}

void AudioCallbackWrapper(daisy::AudioHandle::InputBuffer in, 
    daisy::AudioHandle::OutputBuffer out, 
    size_t size);

int main(void) {
    patch.Init();
    controlManager.Init(&patch);
    synth.Init(&patch);

    synthInstance = &synth; // Assign instance before calling StartAudio
    patch.StartAudio(AudioCallbackWrapper);

    while (1) {
        controlManager.Update();
        synth.SetControlValues(controlManager.GetCtrl1(), 
                                controlManager.GetCtrl2(), 
                                controlManager.GetCtrl3(), 
                                controlManager.GetCtrl4(), 
                                controlManager.GetEncoderCount());

        patch.display.Fill(false);
        patch.display.SetCursor(0, 0);
        patch.display.WriteString("ByteShift v0.1", Font_7x10, true);

        char buffer[32];
        snprintf(buffer, sizeof(buffer), "C1: %d", (int)(controlManager.GetCtrl1() * 100));
        patch.display.SetCursor(0, 12);
        patch.display.WriteString(buffer, Font_7x10, true);

        snprintf(buffer, sizeof(buffer), "Enc: %d", controlManager.GetEncoderCount());
        patch.display.SetCursor(0, 60);
        patch.display.WriteString(buffer, Font_7x10, true);

        patch.display.Update();
        daisy::System::Delay(5);
    }
}
