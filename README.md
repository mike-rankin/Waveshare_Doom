# Waveshare_Doom

Claude created 100% of the code for this. Did I learn anything about coding? No, and I'm ok with this for this particular project. After listening to the audiobook Doom Guy: Life in First Person narrated by John Romero it brought back so many great memories. <BR>
<img width="150" alt="book" src="https://github.com/user-attachments/assets/04153fdd-b28b-47d3-ba7d-9105f1d15745" /> <BR>
Previous side scrollers before this was all we had but Doom changed everything. After the book I grabbed Doom off Steam and played it once more. Lately I've been having a great time using Claude bringing new ideas to life that were never possible. This project idea for Doom on a tiny device needs a lot of hardware and this Waveshare board can do it all: <BR>

 <BR>
-16Megs of flash <BR>
-8Megs of PSRAM <BR>
-a 1.8" amoled, capacitive touch 368 × 448 resolution screen with 16.7M colors <BR>
-a ES8311 sound codec chip <BR>
-built in battery and speaker <BR> 

# What hardware do I need? <BR>
-a microSD card over 4Megs in size <BR>
-the original Waveshare 1.8 inch amoled board. I hear there are two versions of the hardware. If on the back of yours you see this for the display and touch controller hardware inside then you are good. <BR>
<img width="250" alt="Back" src="https://github.com/user-attachments/assets/c71c2a8d-74a2-4fe6-8e0e-394444113755" />


# What software do I need? <BR>
-the Arduino IDE, I'm using version 2.3.11 <BR>
-the Boards Manager, esp32 by Espressif, I'm using version 3.3.13 <BR>
-the library from Library Manager ESP-DOOM <BR>
-feel free to compile your own Arduino sketch or upload a pre-compiled .bin file using the Espressif ESP32 Flash Download tool <BR>
-a shareware doom .wad file, I'm using the full shareware DOOM1.WAD (v1.9) that is 4Megs ins size that you can find with a quick search <BR>
-two versions were created but the landscape version is my favourite and has the ability to save games on the sd card

# What do I need to do? <BR>
-format your microSD card, copy doom1.wad onto it and insert it into your Waveshare board <BR>
-compile and load the portrait or landscape version using the IDE <BR>
-or compile the code yourself in the IDE by using these settings <BR>
<img width="500" alt="IDE_Settings" src="https://github.com/user-attachments/assets/9aa57acc-1377-4b38-8b0a-9b453f5d0a25" /> <BR>
-first compile that a long time but worth the wait

# What option do I choose? <BR>
-the first portrait version has a screen up top and buttons below. I found it difficult to play with the small top screen but has no save game option yet  <BR>

-the second landscape version is a much larger screen with button on top of the play area. This seems a bit better and also allows you to save games <BR>
# Landscape or Portrait Versions

<img width="270"  alt="Splash_Landscape" src="https://github.com/user-attachments/assets/b0b7c3d1-ab97-4cca-ba4e-74e66f8a8fc2" />
<img width="250" alt="Splash_Portrait" src="https://github.com/user-attachments/assets/c45c0ab1-779a-45b1-804a-9d9fbad23793" />

