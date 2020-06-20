# Description

Simple drumset with whitenoise snare and oscillator kick.

# Controls
Press one button to trigger the kick, and the other to trigger the snare.

# Breadboard

[Drum_bb.png](https://github.com/electro-smith/DaisyExamples/blob/master/seed/Drum/resource/Drum_bb.png)
11

# Code Snippet
    //Get the next volume samples
    snr_env_out = snareEnv.Process();
    kck_env_out = kickVolEnv.Process();
    
    //Apply the pitch envelope to the kick
    osc.SetFreq(kickPitchEnv.Process());
    //Set the kick volume to the envelope's output
    osc.SetAmp(kck_env_out);
    //Process the next oscillator sample
    osc_out = osc.Process();

    //Get the next snare sample
    noise_out = noise.Process();
    //Set the sample to the correct volume
    noise_out *= snr_env_out;

    //Mix the two signals at half volume
    sig = .5 * noise_out + .5 + osc_out;

# Schematic

[Drum_schem.png](https://github.com/electro-smith/DaisyExamples/blob/master/seed/Drum/resource/Drum_schem.png)
