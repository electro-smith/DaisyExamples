#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
MidiHandler midi;
Logger<> logger;

// Typical Switch case for Message Type.
void HandleMidiMessage(MidiEvent m)
{
    switch(m.type)
    {
		case NoteOff:
		{
			NoteOffEvent p = m.AsNoteOff();
			logger.Print("Note Off\n");
			logger.Print("   Note: %d\n", p.note);
			logger.Print("   Velocity: %d:\n", p.velocity);
		}
		break;
		case NoteOn:
        {
            NoteOnEvent p = m.AsNoteOn();
			logger.Print("Note On\n");
			logger.Print("   Note: %d\n", p.note);
			logger.Print("   Velocity: %d:\n", p.velocity);
        }
        break;
		case PolyphonicKeyPressure:
		{
			PolyphonicKeyPressureEvent p = m.AsPolyphonicKeyPressure();
			logger.Print("PKP Event\n");
			logger.Print("   Note: %d\n", p.note);
			logger.Print("   Pressure: %d:\n", p.pressure);
		}
		break;
        case ControlChange:
        {
            ControlChangeEvent p = m.AsControlChange();
			logger.Print("Control Change\n");
			logger.Print("   Control Number: %d\n", p.control_number);
			logger.Print("   Value: %d:\n", p.value);

        }
		break;
		case PitchBend:
        {
            PitchBendEvent p = m.AsPitchBend();
			logger.Print("Pitch Bend\n");
			logger.Print("   Value: %d:\n", p.value);

        }
		break;
        default: break;
    }
}

int main(void)
{
	hw.Configure();
	hw.Init();

    midi.Init(MidiHandler::INPUT_MODE_UART1, MidiHandler::OUTPUT_MODE_NONE);
    midi.StartReceive();
	logger.StartLog();
    for(;;)
    {
        midi.Listen();
        // Handle MIDI Events
        while(midi.HasEvents())
        {
            HandleMidiMessage(midi.PopEvent());
        }
    }
}