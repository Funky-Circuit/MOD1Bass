#include <NimBLEDevice.h>

#include "USB.h"
#include "USBMIDI.h"

#include "esp_sleep.h"

#include "Adafruit_MAX1704X.h"

#include <Adafruit_SSD1327.h>

#include "Adafruit_seesaw.h"

#include <Wire.h>





// -- -- -- -- -- constants / objects / variables -- -- -- -- --





//BLE
#define MIDI_SERVICE_UUID        "03B80E5A-EDE8-4B33-A751-6CE34EC4C700"
#define MIDI_CHARACTERISTIC_UUID "7772E5DB-3868-4112-A1A9-F2669D106BF3"
//
NimBLECharacteristic* pCharacteristic;
//
class ServerCallbacks : public NimBLEServerCallbacks 
{
  void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc)
  {
    pServer->updateConnParams(desc->conn_handle, 6, 6, 0, 60);
  }

  void onDisconnect(NimBLEServer* pServer) 
  {
    NimBLEDevice::startAdvertising();
  }
};
//
uint8_t BLE_data_buffer[80];
uint8_t BLE_buffer_index = 0;
uint8_t last_millis_read;


//USB
USBMIDI MIDI("MOD1Bass");


//Battery Monitor
uint8_t current_battery_level;
uint8_t last_battery_level;
//
Adafruit_MAX17048 maxlipo;


//Display
#define OLED_CLK 13
#define OLED_MOSI 11
#define OLED_RESET -1
//
Adafruit_SSD1327 display(128, 128, &Wire, OLED_RESET, 1000000);


//ANO encoder
#define SS_SWITCH_SELECT 1
#define SS_SWITCH_UP     2
#define SS_SWITCH_LEFT   3
#define SS_SWITCH_DOWN   4
#define SS_SWITCH_RIGHT  5
//
#define SEESAW_ADDR      0x49
//
uint8_t button_state[5];
uint8_t button_state_backup[5];
bool button_state_change_flag;
int encoder_values[2][4] = {{40, 45, 50, 55}, {0, 0, 0, 0}};
int encoder_values_backup;
int select_press_count;
int matrix_pos_x;
int matrix_pos_y;
//
Adafruit_seesaw ss;


//string simulation struct
uint8_t trigger_threshold = 50;
uint8_t attack_delay = 50;
//
struct simulated_string
{
  //IOs I2C variable
  uint8_t IOs_address;
  uint8_t IOs_base_value;
  uint8_t current_IOs_value;
  uint8_t last_IOs_value;
  //sensor analog variables
  uint8_t sensor_input_pin;
  uint8_t current_sensor_value;
  uint8_t last_sensor_value;
  bool trigger_flag;
  int trigger_timing;
  bool attack_flag;
  //MIDI channel variable
  uint8_t MIDI_channel;
};
//
simulated_string four_strings_bass[4];





// -- -- -- -- -- setup function -- -- -- -- --





void setup() {
  //Device init delay
  delay(5000);

  //BLE init
  NimBLEDevice::init("MOD1Bass");
  NimBLEDevice::setMTU(185);

  NimBLEServer* pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  NimBLEService* pService = pServer->createService(MIDI_SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
    MIDI_CHARACTERISTIC_UUID,
    NIMBLE_PROPERTY::READ |
    NIMBLE_PROPERTY::WRITE_NR |
    NIMBLE_PROPERTY::NOTIFY
  );

  pService->start();

  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(MIDI_SERVICE_UUID);
  pAdvertising->start();

  //USB init
  MIDI.begin();
  USB.begin();

  //Battery Monitor init
  maxlipo.begin();

  //Display init
  display.begin(0x3D);
  display_settings();

  //ANO encoder init
  ss.begin(SEESAW_ADDR);

  ss.pinMode(SS_SWITCH_UP, INPUT_PULLUP);
  ss.pinMode(SS_SWITCH_DOWN, INPUT_PULLUP);
  ss.pinMode(SS_SWITCH_LEFT, INPUT_PULLUP);
  ss.pinMode(SS_SWITCH_RIGHT, INPUT_PULLUP);
  ss.pinMode(SS_SWITCH_SELECT, INPUT_PULLUP);

  ss.enableEncoderInterrupt();

  //Generic I2C communication init
  Wire.begin();

  //GPIO init
  pinMode(A4, INPUT_PULLDOWN);

  //string simulation init
  for(uint8_t i = 0; i < 4; i++)
  {
    four_strings_bass[i].IOs_address = 0x11 + i;
    four_strings_bass[i].IOs_base_value = encoder_values[0][i];
    four_strings_bass[i].sensor_input_pin = 18 - i;
    four_strings_bass[i].MIDI_channel = encoder_values[1][i];
  }
}





// -- -- -- -- -- custom functions -- -- -- -- --





//BLE MIDI functions
void BLE_MIDI_Note_On(uint8_t channel, uint8_t note, uint8_t velocity)
{
  uint16_t timestamp = millis() & 0x1FFF;
  uint8_t header = 0x80 | ((timestamp >> 7) & 0x3F);
  uint8_t timeLow = 0x80 | (timestamp & 0x7F);
  if(BLE_buffer_index == 0)
  {
    BLE_data_buffer[BLE_buffer_index] = header; BLE_buffer_index ++;
  }
  BLE_data_buffer[BLE_buffer_index] = timeLow; BLE_buffer_index ++;
  BLE_data_buffer[BLE_buffer_index] = 0x90 + channel; BLE_buffer_index ++;
  BLE_data_buffer[BLE_buffer_index] = note; BLE_buffer_index ++;
  BLE_data_buffer[BLE_buffer_index] = velocity; BLE_buffer_index ++;
}


void BLE_MIDI_Note_Off(uint8_t channel, uint8_t note, uint8_t velocity)
{
  uint16_t timestamp = millis() & 0x1FFF;
  uint8_t header = 0x80 | ((timestamp >> 7) & 0x3F);
  uint8_t timeLow = 0x80 | (timestamp & 0x7F);
  if(BLE_buffer_index == 0)
  {
    BLE_data_buffer[BLE_buffer_index] = header; BLE_buffer_index ++;
  }
  BLE_data_buffer[BLE_buffer_index] = timeLow; BLE_buffer_index ++;
  BLE_data_buffer[BLE_buffer_index] = 0x80 + channel; BLE_buffer_index ++;
  BLE_data_buffer[BLE_buffer_index] = note; BLE_buffer_index ++;
  BLE_data_buffer[BLE_buffer_index] = velocity; BLE_buffer_index ++;
}


void BLE_MIDI_Control_Change(uint8_t channel, uint8_t function, uint8_t value)
{
  uint16_t timestamp = millis() & 0x1FFF;
  uint8_t header = 0x80 | ((timestamp >> 7) & 0x3F);
  uint8_t timeLow = 0x80 | (timestamp & 0x7F);
  if(BLE_buffer_index == 0)
  {
    BLE_data_buffer[BLE_buffer_index] = header; BLE_buffer_index ++;
  }
  BLE_data_buffer[BLE_buffer_index] = timeLow; BLE_buffer_index ++;
  BLE_data_buffer[BLE_buffer_index] = 0xB0 + channel; BLE_buffer_index ++;
  BLE_data_buffer[BLE_buffer_index] = function; BLE_buffer_index ++;
  BLE_data_buffer[BLE_buffer_index] = value; BLE_buffer_index ++;
}


void BLE_MIDI_send_buffer()
{
  if(BLE_buffer_index > 0)
  {
    if(millis() - last_millis_read > 5)
    {
      pCharacteristic->setValue(BLE_data_buffer, BLE_buffer_index);
      pCharacteristic->notify();
      BLE_buffer_index = 0;
      last_millis_read = millis();
    }
  }
}


//selected mode MIDI fuctions
void Mode_Note_On(uint8_t mode, uint8_t channel, uint8_t note, uint8_t velocity)
{
  if(mode == 0)
  {
    MIDI.noteOn(note, velocity, channel);
  }
  else
  {
    BLE_MIDI_Note_On(channel, note, velocity);
  }
}


void Mode_Note_Off(uint8_t mode, uint8_t channel, uint8_t note, uint8_t velocity)
{
  if(mode == 0)
  {
    MIDI.noteOff(note, velocity, channel);
  }
  else
  {
    BLE_MIDI_Note_Off(channel, note, velocity);
  }
}


void Mode_Control_Change(uint8_t mode, uint8_t channel, uint8_t function, uint8_t value)
{
  if(mode == 0)
  {
    MIDI.controlChange(function, value, channel);
  }
  else
  {
    BLE_MIDI_Control_Change(channel, function, value);
  }
}


//power functions
void sleep_mode()
{
  if(digitalRead(A4) == 0)
  {
    NimBLEDevice::deinit(true);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_14, 1);
    esp_deep_sleep_start();
  }
}

void display_battery()
{
  last_battery_level = current_battery_level;
  current_battery_level = (uint8_t)maxlipo.cellPercent();
  if(current_battery_level != last_battery_level)
  {
    display_settings();
  }
}


//display fuction
void display_settings()
{
  //display settings
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextWrap(false);
  display.setTextColor(SSD1327_WHITE);
  display.setCursor(0,0);
  //display device name and version
  display.print("MOD1Bass V-1.0");
  //spacing
  for(int i = 0; i < 3; i++)
  {
    display.println();
  }
  //display mode
  display.print("mode: ");
  if(select_press_count == 0)
  {
    display.print("USB");
  }
  else
  {
    display.print("BLE");
  }
  //spacing
  for(int i = 0; i < 3; i++)
  {
    display.println();
  }
  //display battery state
  display.print("Battery: ");
  display.print((uint8_t)maxlipo.cellPercent());
  display.print("%");
  //spacing
  for(int i = 0; i < 3; i++)
  {
    display.println();
  }
  //print controller strings names (base notes)
  display.println("/ - E - A - D - G  /");
  display.println();
  //display rows for transposition and channel setting for each strings
  for(int i = 0; i <= 1; i++)
  {
    display.print("/ ");
    for(int j = 0; j <= 3; j++)
    {
      if(i == matrix_pos_y && j == matrix_pos_x) 
      {
        display.print(">");
      }
      else 
      {
        display.print(" ");
      }
      if(encoder_values[i][j] < 16)
      {
        display.print(0);
      }
      display.print(encoder_values[i][j], HEX);
      display.print(" ");
    }
    display.println(" /");
    display.println();
  }
  display.display();
}


//ANO encoder settings control
void settings_control()
{
  //get button current and previous state
  for(int i = 1; i <= 5; i++)
  {
    button_state_backup[i] = button_state[i];
    button_state[i] = ss.digitalRead(i);
    if(button_state[i] != button_state_backup[i])
    {
      button_state_change_flag = 1;
    }
  }
  //select button pressed
  if(button_state[SS_SWITCH_SELECT] == 0 && button_state[SS_SWITCH_SELECT] != button_state_backup[SS_SWITCH_SELECT])
  {
    select_press_count += 1;
    if(select_press_count > 1) 
    {
      select_press_count = 0;
    }
    if(select_press_count == 1)
    {
      NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
      pAdvertising->addServiceUUID(MIDI_SERVICE_UUID);
      pAdvertising->start();
    }
  }
  //directinal buttons pressed
  if(button_state[SS_SWITCH_UP] == 0 && button_state[SS_SWITCH_UP] != button_state_backup[SS_SWITCH_UP])
  {
    matrix_pos_y += 1;
    if(matrix_pos_y > 1) 
    {
      matrix_pos_y = 0;
    }
  }
  if(button_state[SS_SWITCH_DOWN] == 0 && button_state[SS_SWITCH_DOWN] != button_state_backup[SS_SWITCH_DOWN])
  {
    matrix_pos_y -= 1;
    if(matrix_pos_y < 0) 
    {
      matrix_pos_y = 1;
    }
  }
  if(button_state[SS_SWITCH_RIGHT] == 0 && button_state[SS_SWITCH_RIGHT] != button_state_backup[SS_SWITCH_RIGHT])
  {
    matrix_pos_x += 1;
    if(matrix_pos_x > 3) 
    {
      matrix_pos_x = 0;
    }
  }
  if(button_state[SS_SWITCH_LEFT] == 0 && button_state[SS_SWITCH_LEFT] != button_state_backup[SS_SWITCH_LEFT])
  {
    matrix_pos_x -= 1;
    if(matrix_pos_x < 0) 
    {
      matrix_pos_x = 3;
    }
  }
  //encoder movement
  encoder_values_backup = encoder_values[matrix_pos_y][matrix_pos_x];
  encoder_values[matrix_pos_y][matrix_pos_x] += ss.getEncoderDelta();
  if(encoder_values[0][matrix_pos_x] > 108)
  {
    encoder_values[0][matrix_pos_x] = 21;
  }
  if(encoder_values[0][matrix_pos_x] < 21)
  {
    encoder_values[0][matrix_pos_x] = 108;
  }
  if(encoder_values[1][matrix_pos_x] > 15)
  {
    encoder_values[1][matrix_pos_x] = 0;
  }
  if(encoder_values[1][matrix_pos_x] < 0)
  {
    encoder_values[1][matrix_pos_x] = 15;
  }
  //check for setting value change and display it if needed
  if((button_state_change_flag == 1) || (encoder_values[matrix_pos_y][matrix_pos_x] != encoder_values_backup))
  {
    four_strings_bass[matrix_pos_x].IOs_base_value = encoder_values[0][matrix_pos_x];
    four_strings_bass[matrix_pos_x].MIDI_channel = encoder_values[1][matrix_pos_x];
    button_state_change_flag = 0;
    BLE_MIDI_Control_Change(0, 0x7B, 0);
    MIDI.controlChange(0x7B, 0, 0);
    display_settings();
  }
}


void string_simulator(simulated_string& string)
{
  //get wich fret is pressed and backup of wich was pressed last loop
  string.last_IOs_value = string.current_IOs_value;
  Wire.requestFrom(string.IOs_address, 1);
  while (Wire.available()) 
  {
    string.current_IOs_value = string.IOs_base_value + Wire.read();
  }

  //get pressure value on FSR and the backup value of the last loop
  string.last_sensor_value = string.current_sensor_value;
  string.current_sensor_value = constrain(map(analogRead(string.sensor_input_pin), 0, 4095, 0, 127), 0, 127);

  //trigger attack timer
  if(string.current_sensor_value > trigger_threshold && string.trigger_flag == 0)
  {
    string.trigger_timing = millis();
    string.trigger_flag = 1;
  }

  //note attack at timer ends
  if(((millis() - string.trigger_timing) > attack_delay) && (string.trigger_flag == 1) && (string.attack_flag == 0))
  {
    Mode_Note_On(select_press_count, string.MIDI_channel, string.current_IOs_value, string.current_sensor_value);
    string.attack_flag = 1;
  }

  //change note if a note is still playing
  if((string.current_IOs_value != string.last_IOs_value) && (string.attack_flag == 1))
  {
    Mode_Note_On(select_press_count, string.MIDI_channel, string.current_IOs_value, string.current_sensor_value);
    Mode_Note_Off(select_press_count, string.MIDI_channel, string.last_IOs_value, 0);
  }

  //release note
  if((string.current_sensor_value < trigger_threshold) && (string.attack_flag == 1))
  {
    Mode_Note_Off(select_press_count, string.MIDI_channel, string.current_IOs_value, 0);
    string.trigger_flag = 0;
    string.attack_flag = 0;
  }
}





// -- -- -- -- -- loop function -- -- -- -- --





void loop() 
{
  sleep_mode();
  display_battery();
  settings_control();
  for(uint8_t i = 0; i < 4; i++)
  {
    string_simulator(four_strings_bass[i]);
  }
  if(select_press_count == 1)
  {
    BLE_MIDI_send_buffer();
  }
}