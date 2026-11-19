# Waveshare_Doom

Claude created 100% of the code for this. Did I learn anything about coding? No, and I'm ok with this for this particular project. After listening to the audiobook Doom Guy: Life in First Person narritaed by John Romero it brought back so many great memories. Previous side scrollers before this was all we had but Doom changed everything. After the book I grabbed Doom off Steam and played it once more. Lately I've been having a great time using Claude bringing new ideas to life that was never before possible. This project idea for Doom on a tiny device needs a lot of hardware and this Waveshare board can do it all:
-16Megs of flash <BR>
-8Megs of PSRAM <BR>
-a 1.8" amoled, capacitive touch 368 × 448 resolution screen with 16.7M colors <BR>
-a ES8311 sound codec chip <BR>
-built in battery and speaker <BR> 

# What hardware do I need? <BR>
-the original Waveshare 1.8 inch amoled board. I hear there are two versions of the hardware. If on the back of yours you see this for the display and touch controller hardware inside then you are good. <BR>
- <img width="250" alt="Back" src="https://github.com/user-attachments/assets/c71c2a8d-74a2-4fe6-8e0e-394444113755" />
-a microSD card over 4Megs in size <BR>

# What software do I need? <BR>
-the Arduino IDE, I'm using version 2.3.11 <BR>
-the library from Library Manager ESP-DOOM <BR>
-compile it yourself or use the web flasher and my compiled .bin files https://espressif.github.io/esptool-js/ or https://espressif.github.io/esp-launchpad <BR>
-a shareware doom .wad file, I'm using the full shareware DOOM1.WAD (v1.9) that is 4Megs ins size <BR>

# What do I need to do? <BR>
-format your microSD card, copy doom1.wad onto it and insert it into your Waveshare board <BR>
-flash the portrait or ladscape version on the bin file using an online flasher. Because it is a merged, partitioned file, set the offset location to 0x00 instead of the default 0x1000 <BR>
<img width="350" alt="ESP_Flasher" src="https://github.com/user-attachments/assets/d17ee541-dac5-4064-9dd4-e6978d6021b3" /> <BR>
-or compile the code yourself in the IDE by using these settings <BR>
<img width="500" alt="IDE_Settings" src="https://github.com/user-attachments/assets/9aa57acc-1377-4b38-8b0a-9b453f5d0a25" /> <BR>





