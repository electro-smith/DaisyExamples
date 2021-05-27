#include "daisy_pod.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

/** Fake transport for testing the MidiHandler */
class MidiTestTransport
{
  public:
    MidiTestTransport() {}
    ~MidiTestTransport() {}

    //stubs just to trick the MidiHandler
    inline void   StartRx() {}
    inline size_t Readable() { return msg_idx_ < 149; }
    inline bool   RxActive() { return true; }
    inline void   FlushRx() {}
    inline void   Tx(uint8_t* buff, size_t size) {}

    struct Config
    {
    };

    inline void Init(Config config) { msg_idx_ = 0; }

    inline uint8_t Rx()
    {
        uint8_t ret = messages_[msg_idx_];
        msg_idx_++;
        return ret;
    }

  private:
    int msg_idx_;

    //for now all on channel 0
    uint8_t messages_[149] = {
        // ============ Channel Voice Messages ==============

        0x80,
        0x00,
        0x00, // Note Off, key 0, vel 0
        0x80,
        0x40,
        0x40, // Note Off, key 64, vel 64
        0x80,
        0x7f,
        0x7f, // Note Off, key 127, vel 127

        0x90,
        0x00,
        0x00, // Note On, key 0, vel 0
        0x90,
        0x40,
        0x40, // Note On, key 64, vel 64
        0x90,
        0x7f,
        0x7f, // Note On, key 127, vel 127

        0xa0,
        0x00,
        0x00, // PKP, key 0, vel 0
        0xa0,
        0x40,
        0x40, // PKP, key 64, vel 64
        0xa0,
        0x7f,
        0x7f, // PKP, key 127, vel 127

        0xb0,
        0x00,
        0x00, // Control Change, cc 0, val 0
        0xb0,
        0x40,
        0x40, // Control Change, cc 64, val 64
        0xb0,
        0x77,
        0x7f, // Control Change, cc 119, val 127

        0xc0,
        0x00, // Program Change, program 0
        0xc0,
        0x40, // Program Change, program 64
        0xc0,
        0x7f, // Program Change, program 127

        0xd0,
        0x00, // Channel Pressure, program 0
        0xd0,
        0x40, // Channel Pressure, program 64
        0xd0,
        0x7f, // Channel Pressure, program 127

        0xe0,
        0x00,
        0x00, // Pitch Bend, val -8192
        0xe0,
        0x00,
        0x40, // Pitch Bend, val 0
        0xe0,
        0x7f,
        0x7f, // Pitch Bend, val 8191

        // ============ Channel Mode Messages ==============

        0xb0,
        0x78,
        0x00, // All Sound Off, value 0

        0xb0,
        0x79,
        0x00, // Reset All Controllers, value 0
        0xb0,
        0x79,
        0x40, // Reset All Controllers, value 64
        0xb0,
        0x79,
        0x7f, // Reset All Controllers, value 127

        0xb0,
        0x7a,
        0x00, // Local Control, value Off
        0xb0,
        0x7a,
        0x7f, // Local Control, value On

        0xb0,
        0x7b,
        0x00, // All Notes Off, value 0

        0xb0,
        0x7c,
        0x00, // Omni Mode Off
        0xb0,
        0x7d,
        0x00, // Omni Mode On

        0xb0,
        0x7e,
        0x00, // Mono Mode On, value 0
        0xb0,
        0x7e,
        0x40, // Mono Mode On, value 64
        0xb0,
        0x7e,
        0x7f, // Mono Mode On, value 127

        0xb0,
        0x7f,
        0x00, // Poly Mode On, value 0

        // ============ System Common Messages (minus sysex) ==============

        0xf1,
        0x00, // MTC Quarter Frame, type 0, value 0
        0xf1,
        0x33, // MTC Quarter Frame, type 3, value 3
        0xf1,
        0x7f, // MTC Quarter Frame, type 7, value 15

        0xf2,
        0x00,
        0x00, // Song Position Pointer, value 0
        0xf2,
        0x40,
        0x40, // Song Position Pointer, value 8256
        0xf2,
        0x7f,
        0x7f, // Song Position Pointer, value 16383

        0xf3,
        0x00, // Song Select, value 0
        0xf3,
        0x40, // Song Select, value 64
        0xf3,
        0x7f, // Song Select, value 127

        0xf4, // Undefined
        0xf5, // Undefined
        0xf6, // Tune Request
        0xf7, // End of Exclusive

        // ============ System Real Time ==============
        0xf8, // Timing Clock
        0xf9, // Undefined
        0xfa, // Start
        0xfb, // Continue
        0xfc, // Stop
        0xfd, // Undefined
        0xfe, // Active Sensing
        0xff, // Reset

        // ============ System Exclusive ==============
        0xf0,
        0xf7, // empty
        0xf0,
        0x00,
        0x01,
        0x02,
        0xf7, // 0, 1, 2
        0xf0,
        0x7b,
        0x7c,
        0x7d,
        0x7e,
        0x7f,
        0xf7, // 123, 124, 125, 126, 127
    };
};

DaisyPod hw;
Logger<> logger;
MidiHandler<MidiTestTransport> midi;

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
		case ProgramChange:
        {
            ProgramChangeEvent p = m.AsProgramChange();
			logger.Print("Program Change\n");
			logger.Print("   Program Number: %d\n", p.program);
        }
		break;        
		case ChannelPressure:
        {
            ChannelPressureEvent p = m.AsChannelPressure();
			logger.Print("Channel Pressure\n");
			logger.Print("   Pressure: %d\n", p.pressure);
        }
		break;
		case PitchBend:
        {
            PitchBendEvent p = m.AsPitchBend();
			logger.Print("Pitch Bend\n");
			logger.Print("   Value: %d:\n", p.value);
        }
		break;
		case SystemCommon:
        {
			switch(m.sc_type){
				case SystemExclusive:
				{
					SystemExclusiveEvent p = m.AsSystemExclusive();
					logger.Print("SysEx\n");
					logger.Print("    Message Length: %d\n", p.length);
					for(int i = 0; i < p.length; i++){
						logger.Print("    SysEx data: %d\n", p.data[i]);
					}
				}
				break;
				case MTCQuarterFrame:
				{
					MTCQuarterFrameEvent p = m.AsMTCQuarterFrame();
					logger.Print("MTCQuarterFrame\n");
					logger.Print("   Message Type: %d\n", p.message_type);
					logger.Print("   Value: %d\n", p.value);
				}
				break;
				case SongPositionPointer:
				{
					SongPositionPointerEvent p = m.AsSongPositionPointer();
					logger.Print("SongPositionPointer\n");
					logger.Print("   Position: %d\n", p.position);
				}
				break;
				case SongSelect:
				{
					SongSelectEvent p = m.AsSongSelect();
					logger.Print("Song Select\n");
					logger.Print("   Song: %d\n", p.song);

				}
				break;
				case SCUndefined0:
					logger.Print("Undefined0\n");
					break;
				case SCUndefined1:
					logger.Print("Undefined1\n");
					break;
				case TuneRequest:
					logger.Print("TuneRequest\n");
					break;
				case SysExEnd:
					logger.Print("SysExEnd\n");
					break;
				default: break;
			}
        }		
		break;
		case SystemRealTime:
        {
			switch(m.srt_type){
				case TimingClock:
					logger.Print("Timing Clock\n");
					break;
				case SRTUndefined0:
					logger.Print("SRTUndefined0\n");			
					break;			
				case Start:
					logger.Print("Start\n");
					break;
				case Continue:
					logger.Print("Continue\n");
					break;
				case Stop:
					logger.Print("Stop\n");
					break;				
				case SRTUndefined1:
					logger.Print("SRTUndefined1\n");		
					break;			
				case ActiveSensing:
					logger.Print("ActiveSensing\n");
					break;
				case Reset:
					logger.Print("Reset\n");
					break;
				default: break;
			}
        }	
		break;
		case ChannelMode:
			switch(m.cm_type){
				case AllSoundOff:{
					AllSoundOffEvent p = m.AsAllSoundOff();
					logger.Print("AllSoundOff\n");
					logger.Print("   Channel: %d\n", p.channel);
				}
				break;
				case ResetAllControllers:{
					ResetAllControllersEvent p = m.AsResetAllControllers();
					logger.Print("ResetAllControllers\n");
					logger.Print("   Channel: %d\n", p.channel);
					logger.Print("   Value: %d\n", p.value);
				}
				break;
				case LocalControl:{
					LocalControlEvent p = m.AsLocalControl();
					logger.Print("LocalControl\n");
					logger.Print("   Channel: %d\n", p.channel);
					logger.Print("   LocalControlOn: %d\n", p.local_control_on);
					logger.Print("   LocalControlOff: %d\n", p.local_control_off);
				}
				break;
				case AllNotesOff:{
					AllNotesOffEvent p = m.AsAllNotesOff();
					logger.Print("AllNotesOff\n");
					logger.Print("   Channel: %d\n", p.channel);
				}
				break;
				case OmniModeOff:{
					OmniModeOffEvent p = m.AsOmniModeOff();
					logger.Print("OmniModeOff\n");
					logger.Print("   Channel: %d\n", p.channel);
				}
				break;
				case OmniModeOn:{
					OmniModeOnEvent p = m.AsOmniModeOn();
					logger.Print("OmniModeOn\n");
					logger.Print("   Channel: %d\n", p.channel);
				}
				break;
				case MonoModeOn:{
					MonoModeOnEvent p = m.AsMonoModeOn();
					logger.Print("MonoModeOn\n");
					logger.Print("   Channel: %d\n", p.channel);
					logger.Print("   Num Channels: %d\n", p.num_channels);
				}
				break;
				case PolyModeOn:{
					PolyModeOnEvent p = m.AsPolyModeOn();
					logger.Print("PolyModeOn\n");
					logger.Print("   Channel: %d\n", p.channel);
				}
				break;
				default: break;	
			}
		break;
        default: break;
    }
}

int main(void)
{
	hw.Init();

	MidiHandler<MidiTestTransport>::Config config;
	midi.Init(config);

    midi.StartReceive();
	logger.StartLog();
	hw.StartAdc();
	for(;;)
    {
		hw.ProcessAllControls();
		if(hw.buttons[0]->RisingEdge()){
			midi.Listen();
			// Handle MIDI Events
			while(midi.HasEvents())
			{
				HandleMidiMessage(midi.PopEvent());
			}
		}

    }
}