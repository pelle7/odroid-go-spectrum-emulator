# Odroid-Go Spectrum Emulator

Features

- runs 100% speed, 256x192 screen,border,sound emulated.
- load/save SNA and Z80 snapshot files
- re-map buttons to spectrum keys, standard options available e.g. cursor keys
- on-screen keyboard to simulate hunt-and-peck typing.

#Installing

This assumess you have already built the ODROID-GO and set up an SD card to run the default emulators.

1) Rename the existing firmware.bin file on your SD card to something like firmware.saf, then download the firmware.bin file here in its place. 

If there isn't already a firmware.bin file on your SD card you need to update the bootloader on your odroid - see 

https://github.com/OtherCrashOverride/go-play/releases/tag/20180708

and

https://forum.odroid.com/viewtopic.php?f=158&t=31513

2) While you have the SD card out of the ODROID, make a new directory /roms/spectrum on the card and copy your snapshot files (SNA or Z80) in here.  Using subdirectories is fine.

2) Switch off ORDOID-GO, hold down 'B' button and switch back on, keep  B button
pressed for 5 seconds. If nothing happens you need to update the bootloader - see 1).

press the 'start' button to flash the spectrum firmware.

To go back to the existing emulators, copy firmware.saf back to firmware.bin
and folllow 2) again.

---
#Building from source.

You need the esp-idf build environemnt with a couple of modifications on top of it.

1) SD card support - if you have not already downloaded the esp-idf fraamework, get the patched version by Crashoverride instead: 

https://github.com/OtherCrashOverride/esp-idf/tree/release/v3.1-odroid

or replace esp-idf/components/driver/sdspi_host.c with the one included here.

2) long filename support - edit the file:
esp-idf/components/fatfs/src/ffconf.h
and add a couple of lines:

  #define CONFIG_FATFS_LFN_HEAP 1

  #define FF_MAX_LFN 255

(reference for 1) and 2):  https://forum.odroid.com/viewtopic.php?f=160&t=31383)

3) prevent SPI crashes - edit the file:
esp-idf/components/driver/spi_master.c
and either comment out or completely remove  the line:

SPI_CHECK((handle->cfg.flags & SPI_DEVICE_HALFDUPLEX) || trans_desc->rxlength <= trans_desc->length, "rx length > tx length in full duplex mode", ESP_ERR_INVALID_ARG);

(reference: https://forum.odroid.com/viewtopic.php?f=160&t=31494#p228315)

#Known bugs

- wont load 16Kb snapshots (these are files <16Kb in size, '3D Death Chase' being
  a notable example.). Convert them to regular 48K snapshots in another emulator to load them

- At the moment it doesn't store a set of button mappings per game, just a single set that
is restored at power-on. per-game button mapping memory possible though.

- It will try to load TAP and TXZ files, but wont work with them. Don't use these file type, OK?

# Technical Information

- based on the Spectemu emulator. Chosen as it already supports a 320x240 pixel screen.

https://sourceforge.net/projects/spectemu/files/spectemu-stable/
