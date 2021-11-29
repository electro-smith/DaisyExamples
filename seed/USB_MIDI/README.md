# USB_MIDI

**Author**: shensley

## Description

When the project boots up, a 100Hz sine wave will emit from both outputs,
and the Daisy should appear as an Audio/MIDI device on a connected host.

The oscillator is an always-on, sine wave that will update based on note on messages.

## Troubleshooting

When using Windows, there can be an issue with the device showing up if a non-standard driver has been used for an STM32H7 (like the one used to load boards with the ST dFuse software).

If this is the case follow the following steps to fix it:

* Go to Device Manager
* The Daisy will likely be showing up as a STM32 Virtual COM Port with a yellow warning sign.
* Right click that entry in the COM Ports list, and select "Update Driver"
* Select, "Browse My Computer for Drivers. . ."
* Then Select, "Let me pick from a list of available. . ."
* Then choose "USB Audio Device" from the list
* Unplug, and replug the device, and it should now show up correctly
