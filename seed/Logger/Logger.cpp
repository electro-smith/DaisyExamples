#include <string.h>
#include "daisy_seed.h"

using namespace daisy;

static DaisySeed hw;

// grab the internal logger configuration value LOGGER_BUFFER for verification
// note that the last character is the null terminator
// so [1 + LOGGER_BUFFER] buffer would contain LOGGER_BUFFER meaningful characters
char test_msg1[1 + LOGGER_BUFFER + 1] = "This should really overflow";
char test_msg2[1 + LOGGER_BUFFER    ] = "This should be treated as overflow";
char test_msg3[1 + LOGGER_BUFFER - 1] = "This should be safe for Print()";
char test_msg4[1 + LOGGER_BUFFER - 2] = "This should be safe for Print()";
char test_msg5[1 + LOGGER_BUFFER - 3] = "This should be safe for Print() and PrintLine()";

// append dots (.) till the end of the buffer as a visual cue
template<size_t N>
void fillup_msg(char (&buf)[N])
{
    for (size_t i = strlen(buf); i < N - 1; i++)
    {
        buf[i] = '.';
    }
    buf[N-1] = '\0';
}

void prepare_messages()
{
    fillup_msg(test_msg1);
    fillup_msg(test_msg2);
    fillup_msg(test_msg3);
    fillup_msg(test_msg4);
    fillup_msg(test_msg5);
}

int main(void)
{
    // prepare test messages for corner cases
    prepare_messages();

    // try printing out something before the hardware init
    hw.PrintLine("This should too appear in the log");

    hw.Configure();
    hw.Init();
    hw.StartLog(true);  // true == wait for PC: will block until a terminal is connected

    // use static method directly
    Logger<LOGGER_INTERNAL>::PrintLine("This may be used anywhere too");

    // use a different output destination. Note that this would require the linker to include the whole object with own buffers!
    Logger<LOGGER_EXTERNAL>::Print("This would not be visible, but would not stop the program");

    hw.PrintLine("Verifying newline character handling:");
    hw.PrintLine("1. This should be a single line\r");
    hw.PrintLine("2. This should be a single line\n");
    hw.PrintLine("3. This should be a single line\r\n");
    hw.PrintLine("4. This should be a single line\n\r");
    hw.PrintLine("5. This should be a single line\r\r\r\n\n\n\r\n\r");
    hw.PrintLine("");   // this should be an empty line

    hw.PrintLine("Printing 5 test messages using the Print() service");
    hw.PrintLine("Verify overflow indicators ($$) below");
    hw.Print("1. ");  hw.Print(test_msg1);      hw.PrintLine("");
    hw.Print("2. ");  hw.Print(test_msg2);      hw.PrintLine("");
    hw.Print("3. ");  hw.Print(test_msg3);      hw.PrintLine("");
    hw.Print("4. ");  hw.Print(test_msg4);      hw.PrintLine("");
    hw.Print("5. ");  hw.Print(test_msg5);      hw.PrintLine("");

    hw.PrintLine("Printing 5 test messages using the PrintLine() service");
    hw.PrintLine("Verify overflow indicators ($$) below");
    hw.Print("1. ");  hw.PrintLine(test_msg1);  hw.PrintLine("");
    hw.Print("2. ");  hw.PrintLine(test_msg2);  hw.PrintLine("");
    hw.Print("3. ");  hw.PrintLine(test_msg3);  hw.PrintLine("");
    hw.Print("4. ");  hw.PrintLine(test_msg4);  hw.PrintLine("");
    hw.Print("5. ");  hw.PrintLine(test_msg5);  hw.PrintLine("");

    hw.PrintLine("Starting timer printout. Verify fractional values");

    uint32_t counter = 0;
    while(1)
    {
        dsy_system_delay(500);
                
        const float time_s = dsy_tim_get_ms() * 1.0e-3f;    // showcase floating point output
        hw.PrintLine("%6u: Elapsed time: " FLT_FMT3 " seconds", counter, FLT_VAR3(time_s)); // note that FLT_FMT is part of the format string

        hw.SetLed(counter & 0x01); // LSB triggers the LED
        counter++;
    }

    return 0;
}


/* Reference output shown below

This should too appear in the log
Daisy is online
This may be used anywhere too
Verifying newline character handling:
1. This should be a single line
2. This should be a single line
3. This should be a single line
4. This should be a single line
5. This should be a single line

Printing 5 test messages using the Print() service
Verify overflow indicators ($$) below
1. This should really overflow...................................................................................................$$
2. This should be treated as overflow............................................................................................$$
3. This should be safe for Print()................................................................................................
4. This should be safe for Print()...............................................................................................
5. This should be safe for Print() and PrintLine()..............................................................................
Printing 5 test messages using the PrintLine() service
Verify overflow indicators ($$) below
1. This should really overflow...................................................................................................$$
2. This should be treated as overflow............................................................................................$$
3. This should be safe for Print()...............................................................................................$$
4. This should be safe for Print()...............................................................................................$$
5. This should be safe for Print() and PrintLine()..............................................................................

Starting timer printout. Verify fractional values
     0: Elapsed time:  6.561 seconds
     1: Elapsed time:  7.062 seconds
     2: Elapsed time:  7.563 seconds
     3: Elapsed time:  8.064 seconds
     4: Elapsed time:  8.565 seconds
     5: Elapsed time:  9.065 seconds
     6: Elapsed time:  9.567 seconds
     7: Elapsed time:  10.068 seconds
     8: Elapsed time:  10.569 seconds
     9: Elapsed time:  11.070 seconds
    10: Elapsed time:  11.571 seconds
    11: Elapsed time:  12.072 seconds
    12: Elapsed time:  12.573 seconds
    13: Elapsed time:  13.074 seconds
    14: Elapsed time:  13.575 seconds
    15: Elapsed time:  14.076 seconds


*/
