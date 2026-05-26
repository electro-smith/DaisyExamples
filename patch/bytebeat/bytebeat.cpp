#include "bytebeat_synth.h"

// *******************************************************************
//
// Bytebeat Synth main
// 
// *******************************************************************

int main(void) {
    // Create an instance of DaisyPatch assigned to local var patch
    DaisyPatch patch;
    // Creat an instance of BytebeatSynth assigned to local var synth
    BytebeatSynth synth;
    // Initialize synth pass patch. Is this a reference what is & ?
    synth.Init(&patch);

    // Create a main program loop
    while (1) {
        // Update display
        synth.UpdateDisplay();
        // Chill for 50ms 
        System::Delay(50);
    }
}
