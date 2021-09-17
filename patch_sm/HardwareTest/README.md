# HardwareTest

Hardware verification test.

This is set up to use the patch.Init() hardware to verify the Patch SM is functioning as expected.

To verify the various functionality:

* USB Serial port is used to print data
  * ADC Values for all 12 ADCs are printed (four of these are on the expander bank)
  * Gate input values
  * B7 and B8 (check actual pin names) values based on toggle/button position
  * SDRAM Pass/Fail
  * QSPI Pass/Fail
  * SDMMC Pass/Fail
* If an SD Card is connected at bootup the it will write, and readback a text file to verify it functions.
* Audio will be passing through.
   IN_L is connected to IN_R when on the hardware when no jack is plugged into the IN_R jack.
* The LED and CV out will be generating 1Hz ramp waves.
* The Gate Outputs will be outputting Squarewaves at 500mHz (one pulse per two seconds)

## Checking the values via USB

THIS SERIAL MONITOR SCRIPT HAS **NOT** BEEN ADDED YET.

Included is a script that will open a serial monitor.

Running `test.sh` (or `test.bat` on windows) with python installed a terminal will open and start printing.

By double-clicking, or running with no arguments, the test will find the first available COM port connected and connect to it.
This may not always be the Daisy.

You can pass in an argument with the name of the COM Port (usually checked in your PCs Device Manager)

This will look something like:

```
% Windows
test.bat COM17
```

```
# Mac/Linux
./test.sh usbmodem12345678
```
