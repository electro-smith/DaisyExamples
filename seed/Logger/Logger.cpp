#include <cstring>
#include <cstdio>
#include <cmath>
#include "daisy_seed.h"

using namespace daisy;

static DaisySeed hw;

/** grab the internal logger configuration value LOGGER_BUFFER for verification
 * note that the last character is the null terminator
 * so [1 + LOGGER_BUFFER] buffer would contain LOGGER_BUFFER meaningful characters
 */
char test_msg1[1 + LOGGER_BUFFER + 1] = "This should really overflow";
char test_msg2[1 + LOGGER_BUFFER]     = "This should be treated as overflow";
char test_msg3[1 + LOGGER_BUFFER - 1] = "This should be safe for Print()";
char test_msg4[1 + LOGGER_BUFFER - 2] = "This should be safe for Print()";
char test_msg5[1 + LOGGER_BUFFER - 3]
    = "This should be safe for Print() and PrintLine()";

/** append dots (.) till the end of the buffer as a visual cue
 */
template <size_t N>
void fillup_msg(char (&buf)[N])
{
    for(size_t i = strlen(buf); i < N - 1; i++)
    {
        buf[i] = '.';
    }
    buf[N - 1] = '\0';
}

/** fill up message for buffer overflow testing
 */
void prepare_messages()
{
    fillup_msg(test_msg1);
    fillup_msg(test_msg2);
    fillup_msg(test_msg3);
    fillup_msg(test_msg4);
    fillup_msg(test_msg5);
}

typedef void(PrintFunc)(const char*, ...);

/** profile a single printout function
 */
template <PrintFunc function, typename... Va>
float ProfileFunction(const char* format, Va... va)
{
    constexpr int32_t count = 100000;
    uint32_t          t0    = System::GetTick();
    for(int i = 0; i < count; i++)
    {
        function(format, va...);
    }
    uint32_t    dt = System::GetTick() - t0;
    const float perf
        = (float)dt
          / (200 * count); // using secret knowledge about timer frequency...
    return perf;
}

/** Run all profiling tests for a function 
 */
template <PrintFunc function>
void ProfilePrint(const char* class_caption, const char* func_caption)
{
    const float print_0 = ProfileFunction<function>("");
    const float print_1 = ProfileFunction<function>("a");
    const float print_36
        = ProfileFunction<function>("abcdefghijklmnopqrstuvwxyz0123456789");
    const float print_d = ProfileFunction<function>("%d", 123);
    const float print_u = ProfileFunction<function>("%u", 123);
    const float print_f = ProfileFunction<function>("%.3f", 123.0f);
    const float print_F1
        = ProfileFunction<function>(FLT_FMT(1), FLT_VAR(1, 123.0f));
    const float print_F2
        = ProfileFunction<function>(FLT_FMT(2), FLT_VAR(2, 123.0f));
    const float print_F3
        = ProfileFunction<function>(FLT_FMT3, FLT_VAR3(123.0f));
    const float print_F4
        = ProfileFunction<function>(FLT_FMT(4), FLT_VAR(4, 123.0f));

    /* linear fit */
    const float slope     = (print_36 - print_1) / (36 - 1);
    const float intercept = print_1 - slope;

    hw.PrintLine("%s - %s (  empty string):" FLT_FMT3 " us/call",
                 class_caption,
                 func_caption,
                 FLT_VAR3(print_0));
    hw.PrintLine("%s - %s (   text string):" FLT_FMT3 " us/call +" FLT_FMT3
                 " us/char",
                 class_caption,
                 func_caption,
                 FLT_VAR3(intercept),
                 FLT_VAR3(slope));
    hw.PrintLine("%s - %s ( integer value):" FLT_FMT3 " us/call",
                 class_caption,
                 func_caption,
                 FLT_VAR3(print_d));
    hw.PrintLine("%s - %s (unsigned value):" FLT_FMT3 " us/call",
                 class_caption,
                 func_caption,
                 FLT_VAR3(print_u));
    hw.PrintLine("%s - %s ( float.3 value):" FLT_FMT3 " us/call",
                 class_caption,
                 func_caption,
                 FLT_VAR3(print_f));
    hw.PrintLine("%s - %s (FLT_VAR1 value):" FLT_FMT3 " us/call",
                 class_caption,
                 func_caption,
                 FLT_VAR3(print_F1));
    hw.PrintLine("%s - %s (FLT_VAR2 value):" FLT_FMT3 " us/call",
                 class_caption,
                 func_caption,
                 FLT_VAR3(print_F2));
    hw.PrintLine("%s - %s (FLT_VAR3 value):" FLT_FMT3 " us/call",
                 class_caption,
                 func_caption,
                 FLT_VAR3(print_F3));
    hw.PrintLine("%s - %s (FLT_VAR4 value):" FLT_FMT3 " us/call",
                 class_caption,
                 func_caption,
                 FLT_VAR3(print_F4));
}

/** Profile the whole destination
 */
template <LoggerDestination dest>
void ProfileDest(const char* class_caption)
{
    Logger<dest>::StartLog(false);
    ProfilePrint<Logger<dest>::Print>(class_caption, "Print     ");
    ProfilePrint<Logger<dest>::PrintLine>(class_caption, "PrintLine ");
}
/** Verify FLT_FMT/FLT_VAR accuracy
 */
void VerifyFloats()
{
    char ref[32];
    char tst[32];
    bool result = true;
    for(float f = -100.0; f < 100.0; f += 0.001)
    {
        sprintf(tst, FLT_FMT(3), FLT_VAR(3, f));

        const double f_tst = strtod(tst, nullptr);

        /* verify down to the least significant digit, which is off because of truncation */
        if(abs((double)f - f_tst) > 1.0e-3)
        {
            sprintf(ref, "%.3f", f);
            hw.PrintLine(
                "mismatch: ref = %s, dsy = %s, float = %.6f", ref, tst, f);
            result = false;
        }
    }
    hw.PrintLine("FLT_FMT(3)/FLT_VAR(3) verification: %s",
                 result ? "PASS" : "FAIL");
}

int main(void)
{
    /* prepare test messages for corner cases */
    prepare_messages();

    /* try printing out something before the hardware init */
    hw.PrintLine("This should too appear in the log");

    hw.Configure();
    hw.Init();
    hw.StartLog(
        true); /* true == wait for PC: will block until a terminal is connected */

    /** check that floating point printf is supported 
     * linker flags modified in the Makefile to enable this
     */
    hw.PrintLine("Verify CRT floating point format: %.3f", 123.0f);

    VerifyFloats();

    /** Profile different destinations 
     * Don't use LOGGER_INTERNAL, since it will flood the connected terminal
     */
    ProfileDest<LOGGER_EXTERNAL>("LOGGER_EXTERNAL");
    ProfileDest<LOGGER_SEMIHOST>("LOGGER_SEMIHOST");
    ProfileDest<LOGGER_NONE>("LOGGER_NONE    ");

    /* use static method directly */
    Logger<LOGGER_INTERNAL>::PrintLine("This may be used anywhere too");

    /** use a different output destination.
     * Note that this would require the linker to include the whole object with own buffers!
     */
    Logger<LOGGER_EXTERNAL>::Print(
        "This would not be visible, but would not stop the program");

    hw.PrintLine("Verifying newline character handling:");
    hw.PrintLine("1. This should be a single line\r");
    hw.PrintLine("2. This should be a single line\n");
    hw.PrintLine("3. This should be a single line\r\n");
    hw.PrintLine("4. This should be a single line\n\r");
    hw.PrintLine("5. This should be a single line\r\r\r\n\n\n\r\n\r");
    hw.PrintLine(""); /* this should be an empty line */

    hw.PrintLine("Printing 5 test messages using the Print() service");
    hw.PrintLine("Verify overflow indicators ($$) below");
    hw.Print("1. ");
    hw.Print(test_msg1);
    hw.PrintLine("");
    hw.Print("2. ");
    hw.Print(test_msg2);
    hw.PrintLine("");
    hw.Print("3. ");
    hw.Print(test_msg3);
    hw.PrintLine("");
    hw.Print("4. ");
    hw.Print(test_msg4);
    hw.PrintLine("");
    hw.Print("5. ");
    hw.Print(test_msg5);
    hw.PrintLine("");

    hw.PrintLine("Printing 5 test messages using the PrintLine() service");
    hw.PrintLine("Verify overflow indicators ($$) below");
    hw.Print("1. ");
    hw.PrintLine(test_msg1);
    hw.PrintLine("");
    hw.Print("2. ");
    hw.PrintLine(test_msg2);
    hw.PrintLine("");
    hw.Print("3. ");
    hw.PrintLine(test_msg3);
    hw.PrintLine("");
    hw.Print("4. ");
    hw.PrintLine(test_msg4);
    hw.PrintLine("");
    hw.Print("5. ");
    hw.PrintLine(test_msg5);
    hw.PrintLine("");

    hw.PrintLine("Starting timer printout. Verify fractional values");

    uint32_t counter = 0;
    while(1)
    {
        System::Delay(500);

        const float time_s = System::GetNow() * 1.0e-3f;

        /** showcase floating point output 
         * note that FLT_FMT is part of the format string
         */
        hw.PrintLine("%6u: Elapsed time: " FLT_FMT3 " seconds",
                     counter,
                     FLT_VAR3(time_s));

        /* LSB triggers the LED */
        hw.SetLed(counter & 0x01);
        counter++;
    }

    return 0;
}


/* Reference output shown below

This should too appear in the log
Daisy is online
===============
Verify CRT floating point format: 123.000
FLT_FMT(3)/FLT_VAR(3) verification: PASS
LOGGER_EXTERNAL - Print     (  empty string): 0.404 us/call
LOGGER_EXTERNAL - Print     (   text string): 0.516 us/call + 0.010 us/char
LOGGER_EXTERNAL - Print     ( integer value): 1.921 us/call
LOGGER_EXTERNAL - Print     (unsigned value): 1.905 us/call
LOGGER_EXTERNAL - Print     ( float.3 value): 4.675 us/call
LOGGER_EXTERNAL - Print     (FLT_VAR1 value): 4.194 us/call
LOGGER_EXTERNAL - Print     (FLT_VAR2 value): 4.382 us/call
LOGGER_EXTERNAL - Print     (FLT_VAR3 value): 4.520 us/call
LOGGER_EXTERNAL - Print     (FLT_VAR4 value): 4.644 us/call
LOGGER_EXTERNAL - PrintLine (  empty string): 0.450 us/call
LOGGER_EXTERNAL - PrintLine (   text string): 0.556 us/call + 0.009 us/char
LOGGER_EXTERNAL - PrintLine ( integer value): 2.005 us/call
LOGGER_EXTERNAL - PrintLine (unsigned value): 1.987 us/call
LOGGER_EXTERNAL - PrintLine ( float.3 value): 4.752 us/call
LOGGER_EXTERNAL - PrintLine (FLT_VAR1 value): 4.270 us/call
LOGGER_EXTERNAL - PrintLine (FLT_VAR2 value): 4.487 us/call
LOGGER_EXTERNAL - PrintLine (FLT_VAR3 value): 4.620 us/call
LOGGER_EXTERNAL - PrintLine (FLT_VAR4 value): 4.746 us/call
LOGGER_SEMIHOST - Print     (  empty string): 0.450 us/call
LOGGER_SEMIHOST - Print     (   text string): 0.598 us/call + 0.016 us/char
LOGGER_SEMIHOST - Print     ( integer value): 1.880 us/call
LOGGER_SEMIHOST - Print     (unsigned value): 1.862 us/call
LOGGER_SEMIHOST - Print     ( float.3 value): 4.745 us/call
LOGGER_SEMIHOST - Print     (FLT_VAR1 value): 4.256 us/call
LOGGER_SEMIHOST - Print     (FLT_VAR2 value): 4.406 us/call
LOGGER_SEMIHOST - Print     (FLT_VAR3 value): 4.543 us/call
LOGGER_SEMIHOST - Print     (FLT_VAR4 value): 4.676 us/call
LOGGER_SEMIHOST - PrintLine (  empty string): 0.432 us/call
LOGGER_SEMIHOST - PrintLine (   text string): 0.607 us/call + 0.016 us/char
LOGGER_SEMIHOST - PrintLine ( integer value): 2.082 us/call
LOGGER_SEMIHOST - PrintLine (unsigned value): 2.065 us/call
LOGGER_SEMIHOST - PrintLine ( float.3 value): 4.858 us/call
LOGGER_SEMIHOST - PrintLine (FLT_VAR1 value): 4.452 us/call
LOGGER_SEMIHOST - PrintLine (FLT_VAR2 value): 4.637 us/call
LOGGER_SEMIHOST - PrintLine (FLT_VAR3 value): 4.772 us/call
LOGGER_SEMIHOST - PrintLine (FLT_VAR4 value): 4.907 us/call
LOGGER_NONE     - Print     (  empty string): 0.000 us/call
LOGGER_NONE     - Print     (   text string): 0.000 us/call + 0.000 us/char
LOGGER_NONE     - Print     ( integer value): 0.000 us/call
LOGGER_NONE     - Print     (unsigned value): 0.000 us/call
LOGGER_NONE     - Print     ( float.3 value): 0.000 us/call
LOGGER_NONE     - Print     (FLT_VAR1 value): 0.000 us/call
LOGGER_NONE     - Print     (FLT_VAR2 value): 0.000 us/call
LOGGER_NONE     - Print     (FLT_VAR3 value): 0.000 us/call
LOGGER_NONE     - Print     (FLT_VAR4 value): 0.000 us/call
LOGGER_NONE     - PrintLine (  empty string): 0.000 us/call
LOGGER_NONE     - PrintLine (   text string): 0.000 us/call + 0.000 us/char
LOGGER_NONE     - PrintLine ( integer value): 0.000 us/call
LOGGER_NONE     - PrintLine (unsigned value): 0.000 us/call
LOGGER_NONE     - PrintLine ( float.3 value): 0.000 us/call
LOGGER_NONE     - PrintLine (FLT_VAR1 value): 0.000 us/call
LOGGER_NONE     - PrintLine (FLT_VAR2 value): 0.000 us/call
LOGGER_NONE     - PrintLine (FLT_VAR3 value): 0.000 us/call
LOGGER_NONE     - PrintLine (FLT_VAR4 value): 0.000 us/call
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
     0: Elapsed time:  18.226 seconds
     1: Elapsed time:  18.727 seconds
     2: Elapsed time:  19.228 seconds
     3: Elapsed time:  19.729 seconds
     4: Elapsed time:  20.230 seconds
     5: Elapsed time:  20.731 seconds
     6: Elapsed time:  21.232 seconds
     7: Elapsed time:  0.259 seconds
     8: Elapsed time:  0.760 seconds
*/
