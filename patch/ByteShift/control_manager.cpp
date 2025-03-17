#include "control_manager.h"

// *******************************************************************
//
// Control Manager 
//
// *******************************************************************

void ControlManager::Init(daisy::DaisyPatch* p) {
    patch = p;
    patch->StartAdc(); // Must start ADC before reading controls!

    ctrl1 = ctrl2 = ctrl3 = ctrl4 = 0.0f;
    encoderCount = 0;
    encoderPressed = false;
}

void ControlManager::Update() {
    // Read control values
    patch->ProcessAnalogControls();
    patch->ProcessDigitalControls();

    // Store control values 
    ctrl1 = patch->GetKnobValue(daisy::DaisyPatch::CTRL_1);
    ctrl2 = patch->GetKnobValue(daisy::DaisyPatch::CTRL_2);
    ctrl3 = patch->GetKnobValue(daisy::DaisyPatch::CTRL_3);
    ctrl4 = patch->GetKnobValue(daisy::DaisyPatch::CTRL_4);

    // Read the encoder
    encoderCount += patch->encoder.Increment();
    encoderPressed = patch->encoder.RisingEdge();

    // Reset pitch shift when changing formula
    if (encoderPressed) {
        encoderCount = 0;
    } 
}