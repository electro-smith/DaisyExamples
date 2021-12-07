#include "daisy_field.h"
#include "daisysp.h"
#include "samplebuffer.h"
#include <array>

using namespace daisy;
using namespace daisysp;

#define NUM_BUFFERS 8
#define KEYBOARD_WIDTH 8
// 5 second sample buffers at 48kHz
#define BUFFER_SIZE (48000 * 5)

DaisyField hw;

// DSY_SDRAM_BSS macro puts objects on the SDRAM
// SDRAM has 64MB which is much more than the MCU memory
std::array<SampleBuffer<BUFFER_SIZE>, NUM_BUFFERS> DSY_SDRAM_BSS buffers;

void Controls()
{
    hw.ProcessAllControls();
    for(size_t i = 0; i < buffers.size(); i++)
    {
        // Handle lower key row as play buttons
        if(hw.KeyboardRisingEdge(i))
        {
            buffers[i].Play();
        }
        else if(hw.KeyboardFallingEdge(i))
        {
            // Remove this line to keep playing after key is released
            buffers[i].Play(false);
        }
        // Handle upper key row as record buttons
        if(hw.KeyboardRisingEdge(i + KEYBOARD_WIDTH))
        {
            buffers[i].Record();
        }
        else if(hw.KeyboardFallingEdge(i + KEYBOARD_WIDTH))
        {
            // Remove this line to keep recording after key is released
            buffers[i].Record(false);
        }
    }
}

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    Controls();
    for(size_t i = 0; i < size; i++)
    {
        // Zero samples prior to summing
        out[0][i] = 0.f;
        out[1][i] = 0.f;
        for(auto &buffer : buffers)
        {
            // Record in mono
            float sample = buffer.Process((0.5 * in[0][i] + 0.5 * in[1][i]));
            // Sum all playback channels
            out[0][i] += sample;
            out[1][i] += sample;
        }
        // Feed stereo input through to output
        out[0][i] += in[0][i];
        out[1][i] += in[1][i];
    }
}

void InitSamplers()
{
    for(auto &buffer : buffers)
    {
        buffer.Init();
    }
}

void UpdateDisplay()
{
    static const char number_row[17] = " 1 2 3 4 5 6 7 8";
    static const char blank_row[17]  = "                ";
    char              recording_row[17];
    char              playing_row[17];
    strcpy(recording_row, blank_row);
    strcpy(playing_row, blank_row);

    for(size_t b = 0; b < buffers.size(); b++)
    {
        recording_row[2 * b + 1] = buffers[b].IsRecording() ? '+' : ' ';
        playing_row[2 * b + 1]   = buffers[b].IsPlaying() ? '>' : ' ';
    }
    hw.display.Fill(false);
    hw.display.SetCursor(0, 0);
    hw.display.WriteString(number_row, Font_7x10, true);
    hw.display.SetCursor(0, 10);
    hw.display.WriteString(recording_row, Font_7x10, true);
    hw.display.SetCursor(0, 20);
    hw.display.WriteString(playing_row, Font_7x10, true);
    hw.display.Update();
}

int main(void)
{
    hw.Init(); // Don't try to use SDRAM until after this Init function
    InitSamplers();
    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    while(true)
    {
        UpdateDisplay();
        System::Delay(10);
    }
}
