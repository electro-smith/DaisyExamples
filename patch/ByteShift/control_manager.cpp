#include "control_manager.h"

void ControlManager::Init(daisy::DaisyPatch* p) {
    patch = p;
    ctrl1 = ctrl2 = ctrl3 = ctrl4 = 0.0f;
    encoderCount = 0;
    encoderPressed = false;
}

void ControlManager::Update() {
    patch->ProcessAnalogControls();
    patch->ProcessDigitalControls();

    ctrl1 = patch->GetKnobValue(daisy::DaisyPatch::CTRL_1);
    ctrl2 = patch->GetKnobValue(daisy::DaisyPatch::CTRL_2);
    ctrl3 = patch->GetKnobValue(daisy::DaisyPatch::CTRL_3);
    ctrl4 = patch->GetKnobValue(daisy::DaisyPatch::CTRL_4);

    encoderCount += patch->encoder.Increment();
    encoderPressed = patch->encoder.RisingEdge();
}