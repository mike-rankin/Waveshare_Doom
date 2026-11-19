# Waveshare_Doom
Doom on the Waveshare ESP32-S3 1.8 inch amoled board

Claude created 100% of the code for this.

# What hardware do I need? <BR>
-the original Waveshare 1.8 inch amoled board. I hear there are two versions of the hardware. If on the back of yours you see this for the display and touch controller hardware inside then you are good.
- <img width="508" height="640" alt="Back" src="https://github.com/user-attachments/assets/c71c2a8d-74a2-4fe6-8e0e-394444113755" />
-a microSD card over 4Megs in size

# What software do I need? <BR>
-the Arduino IDE, I'm using version 2.3.11
-the library from Library Manager ESP-DOOM
-compile it yourself or use the web flasher and my compiled .bin files https://espressif.github.io/esptool-js/ or https://espressif.github.io/esp-launchpad
-a shareware doom .wad file, I'm using the full shareware DOOM1.WAD (v1.9) that is 4Megs ins size

# What do I need to do? <BR>
-format your microSD card, copy doom1.wad onto it and insert it into your Waveshare board
-flash the portrait or ladscape version on the bin file using an online flasher. Because it is a merged, partitioned file, set the offset location to 0x00 instead of the default 0x1000
<img width="1098" height="1030" alt="IDE_Settings" src="https://github.com/user-attachments/assets/9aa57acc-1377-4b38-8b0a-9b453f5d0a25" />
<img width="1371" height="442" alt="ESP_Flasher" src="https://github.com/user-attachments/assets/d17ee541-dac5-4064-9dd4-e6978d6021b3" />

Install the bin file over web: 


