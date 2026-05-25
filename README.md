# CASIO-MIDI-CV-Keyboard
Turned my CASIO SA-45 keyboard into a MIDI and CV keyboard Controller

This project uses an arduino nano and heavily relies on the keypad and MIDI libraries.

I'm using a 0.91in OLED module for the display.

MCP4725 is a DAC for generating CV to get 1V/OCT.
MCP4725's output will vary with and change in supply voltage (even USB is never going to be precisely 5V every where),
so I've add the option to tune the caliberation to get exactly 1V/OCT.

After caliberation you can save the updated caliberation value in the EEPROM so no need to caliberate everytime you power up the controller.
