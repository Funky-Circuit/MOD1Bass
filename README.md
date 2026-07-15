# MOD1Bass
Bass guitar inspired USB and BLE MIDI controller based on an Adafruit Feather ESP32-S3 board


-- -- --


all the code for this project was made using the Arduino IDE and the following libraries:
- NimBLE:
https://github.com/h2zero/NimBLE-Arduino
- Arduino ESP32:
https://github.com/espressif/arduino-esp32
- Adafruit MAX1704X
https://github.com/adafruit/Adafruit_MAX1704X
- Adafruit SSD1327
https://github.com/adafruit/Adafruit_SSD1327
- Adafruit Seesaw:
https://github.com/adafruit/adafruit_seesaw


-- -- --


how to assemble this project:
1. you will need the board indicated in the link section below with the bottom side preassembled

2. get 4 Atmega328p: for each of them, you will need to burn the bootloader and upload the file "MOD1Bass_fretboard.ino" into them (to do so, you will need an external programmer or an Arduino who is going to act as a programmer thanks to the Arduino IDE example "ArduinoISP")

3. each time you upload the file "MOD1Bass_fretboard.ino" into an Atmega328p after the first, on line 17, you will need to modify the SPI address to 0x12 for the second, 0x13 for the third and 0x14 fo the fourth (for example "Wire.begin(0x12);" for the second) before uploading

4. once all the Atmega328p are ready, put each of the in a socket on the board (they have a specific order: from left to right, viewed from the top side: 0x11, 0x12, 013, 0x14)

5. connect the board to the screen, the screen to the ANO controller and the ANO controller to the Adafruit Feather ESP32-S3 board with qwiic cables
