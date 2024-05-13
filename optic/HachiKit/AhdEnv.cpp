#include "AhdEnv.h"
#include "Utility.h"

using namespace daisy;
using namespace daisysp;

void AhdEnv::Init(float sample_rate) {
    ad.Init(sample_rate);
    adsr.Init(sample_rate);
    adsr.SetTime(ADSR_SEG_DECAY, 0.001);
    adsr.SetSustainLevel(0.5);
}

float AhdEnv::Process() {
    // The AD envelope is used to count off the gate time of the ADSR, providing the hold time.
    ad.Process();
    return adsr.Process(ad.IsRunning());
}

void AhdEnv::Trigger() {
    ad.Trigger();
    adsr.Retrigger(true);
}

void AhdEnv::SetAttack(float time) {
    ad.SetTime(ADENV_SEG_ATTACK, time);
    adsr.SetTime(ADSR_SEG_ATTACK, time);
}

void AhdEnv::SetHold(float time) {
    ad.SetTime(ADENV_SEG_DECAY, time);
}

void AhdEnv::SetDecay(float time) {
    adsr.SetTime(ADSR_SEG_RELEASE, time);
}

bool AhdEnv::IsRunning() const {
    return adsr.IsRunning();
}
