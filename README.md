# MOD1Bass
Bass guitar inspired USB and BLE MIDI controller based on an Adafruit Feather ESP32-S3 board


-- -- --


a bass model was designed fo this project. you can find all the files needed in the "MOD1Bass_models" folder and print them or create you own model


-- -- --


all the code for this project was made using the Arduino IDE and the following libraries:
- MiniCore:
https://github.com/mcudude/minicore
- NimBLE:
https://github.com/h2zero/NimBLE-Arduino
- Arduino ESP32:
https://github.com/espressif/arduino-esp32
- Adafruit MAX1704X:
https://github.com/adafruit/Adafruit_MAX1704X
- Adafruit SSD1327:
https://github.com/adafruit/Adafruit_SSD1327
- Adafruit Seesaw:
https://github.com/adafruit/adafruit_seesaw


-- -- --


how to assemble this project:

1. you will need the PCB contained in the "PCB_MOD1Bass_B.json" file (that you can open in the software EasyEDA) with the bottom side preassembled. it will be refered as fretboard.

2. get 4 Atmega328p: for each of them, you will need to burn the bootloader and upload the file "MOD1Bass_fretboard.ino" into them. to do so, you will need an external programmer or an Arduino who is going to act as a programmer thanks to the Arduino IDE example "ArduinoISP". you will need to conect an external 16MHz clock to the Atmega328p and then burn the appropriate bootloader from the minicore board selection.

3. each time you upload the file "MOD1Bass_fretboard.ino" into an Atmega328p after the first, on line 17, you will need to modify the I2C address to 0x12 for the second, 0x13 for the third and 0x14 fo the fourth (for example "Wire.begin(0x12);" for the second) before uploading.

4. once all the Atmega328p are ready, put each of the in on of the four DIP socket on the fretboard (they have a specific order: from left to right, viewed from the top side: 0x11, 0x12, 013, 0x14).

5. connect the fretboard to the screen, the screen to the ANO controller and the ANO controller to the Adafruit Feather ESP32-S3 board with qwiic cables.

6. connect 4 force sensitive resistors (FSRs) to A0, A1, A2 and A3 of the Adafruit Feather ESP32-S3 board, with a 10k pulldown resistor on the signal each. starting from A0 on the left, dispose the FSRs next to each other in the order they where connected (A0, then A1, then A2 and to fnish A3). the other pin of the FSRs needs to be connected to one of the 3V pin of the Adafruit Feather ESP32-S3 board.

7. connect toggle switch (with one pin on the A4 pin and the other pin on the 3V pin) and a li-po battery to the Adafruit Feather ESP32-S3 board (there is a JST 2-PH port made specificaly for power input)
