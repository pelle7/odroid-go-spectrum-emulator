# Odroid-Go Spectrum Emulator

Features

- Emulates 48K ZX Spectrum. Runs at 100% speed, 256x192 screen,border,sound emulated.
- load/save SNA and Z80 snapshot files
- re-map buttons to spectrum keys, standard options available e.g. cursor keys
- on-screen keyboard to simulate hunt-and-peck typing.
- fast startup (about 1 second), resume from last saved position.

#Installing

This assumess you have already built the ODROID-GO and set up an SD card to run the default emulators.

Note that installing this emulator _replaces_ the existing ones, but they can be restored later.

1) Download the file ZX_Spectrum.fw from here and put it in the ordoid/firmware directory of your SD card. You probably 
also should make sure there is a file Go-Play.fw here so you can go back to the existing emulators.

Go-Play.fw can be found at: https://github.com/OtherCrashOverride/go-play/releases/

2) While you have the SD card out of the ODROID, make a new directory /roms/spectrum on the card and copy your snapshot files (SNA or Z80) in here.  Using subdirectories is fine.

3) Switch off ORDOID-GO, hold down 'B' button and switch back on, keep  B button
pressed for 5 seconds. If nothing happens you need to update the bootloader - see

https://forum.odroid.com/viewtopic.php?f=158&t=31513


Go to the 'ZX Spectrum' option and press the 'start' button to flash the firmware.

To go back to the existing emulators, follow step 3) again, but choose Go-Play instead.

--
#Tips

- in the on-screen keyboard, the Select and Start buttons are mapped to Caps Shift
and Symbol Shift, if upper-case or punctuation characters required. 'B' exits.

- in the file browser, left/right goes back/forward a page of files for large directories. 'B' goes up a directory.

---
#Building from source.

You need the esp-idf build environment with a couple of modifications on top of it.

1) SD card support - if you have not already downloaded the esp-idf framework, get the patched version by Crashoverride instead: 

https://github.com/OtherCrashOverride/esp-idf/tree/release/v3.1-odroid

or replace esp-idf/components/driver/sdspi_host.c with the one included here.

(reference:  https://forum.odroid.com/viewtopic.php?f=160&t=31383)

2) prevent SPI crashes - edit the file:
esp-idf/components/driver/spi_master.c
and either comment out or completely remove  the line:

SPI_CHECK((handle->cfg.flags & SPI_DEVICE_HALFDUPLEX) || trans_desc->rxlength <= trans_desc->length, "rx length > tx length in full duplex mode", ESP_ERR_INVALID_ARG);

(reference: https://forum.odroid.com/viewtopic.php?f=160&t=31494#p228315)

#Known bugs

- wont load 16Kb SNA snapshots (these are files <16Kb in size, '3D Death Chase' and 'Jetpac' being
  notable examples.). Convert them to regular 48K snapshots in another emulator to load them.

- At the moment it doesn't store a set of button mappings per game, just a single set that
is restored at power-on. Per-game button mapping memory possible though.

- It will try to load TAP and TXZ files, but wont work with them. Don't use these file types, OK?

# Technical Information

- based on the Spectemu emulator. Chosen as it already supports a 320x240 pixel screen.

https://sourceforge.net/projects/spectemu/files/spectemu-stable/
