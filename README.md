# KAOS
Pi Pico Skylander Portal Emulator

Supports following types of Skylander formats:
 + .bin
 + .dmp
 + .dump
 + .sky (created via RPCS3)

## Usage
To Emulate the portal, just plug in the Microcontroller into a USB Port of a Console. As soon as the RP2040 is ready it will be detected as a Portal.
What does which button do?

(Image of Board with numbers here)

 + Button 1: Changes the selected slot to the Left (Player 2 -> Player 1)
 + Button 2: Changes the selected slot to the Right (Player 1 -> Player 2)
 + Button 3: Sends the Sense Command to the Game (Tells the game how many Skylanders are on the Portal. Only use if the game doesnt detect the skylander)
 + Button 4: Change File Selection to the Left (If File 2 would be Spyro.bin and File 1 Kaos.bin, it would switch Spyro for Kaos)
 + Button 5: Change File Selection to the Right (If File 2 would be Spyro.bin and File 1 Kaos.bin, it would switch Kaos for Spyro)
 + Button 6: Load/Unload Skylander from selected slot

## Creating the Pico Board
Required Things for the Breadboard (atleast how I built it):
 + RP2040 Based Microcontroller
 + 6 Buttons
 + SPI SD Card Reader Module
 + LCD1602 With I2C Backpack
 + 2 Breadboards with 830 Contacts each
 + Jumpercables

Connect all the things like the Connection Diagram is telling you

Connection Diagram
(insert Diagram here)
 
it should look something like this:

(my bad board here)

## Installation
Copy the KAOS.uf2 File onto the Microcontroller, while it is in flash mode.

## Building
To build you need the pi pico sdk
You need to copy the tusb_config.h into this location <pico-sdk_location>\lib\tinyusb\src\tusb_config.h
