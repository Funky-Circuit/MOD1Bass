#include "Wire.h"



// -- -- -- -- --



void setup() 
{
  DDRB = 0;
  DDRC = 0;
  DDRD = 0;
  PORTB = 0;
  PORTC = 0;
  PORTD = 0;
  Wire.begin(0x11);
  Wire.onRequest(data_request);
}



// -- -- -- -- --



void loop() 
{

}



// -- -- -- -- --



void data_request()
{
  uint8_t array_size = 19;
  uint8_t IO_read[array_size] = 
  {
    (PIND & (1 << PD0)), (PINC & (1 << PC2)), (PIND & (1 << PD1)), (PINC & (1 << PC1)), (PIND & (1 << PD2)),
    (PINC & (1 << PC0)), (PIND & (1 << PD3)), (PINB & (1 << PB5)), (PIND & (1 << PD4)), (PINB & (1 << PB4)),
    (PINB & (1 << PB6)), (PINB & (1 << PB3)), (PINB & (1 << PB7)), (PINB & (1 << PB2)), (PIND & (1 << PD5)),
    (PINB & (1 << PB1)), (PIND & (1 << PD6)), (PINB & (1 << PB0)), (PIND & (1 << PD7))
  };
  uint8_t data = 0;
  for(int i = 0; i < sizeof(IO_read); i ++)
  {
    if(IO_read[i] > 0)
    {
      data = i + 1;
    }
  }
  Wire.write(data);
}
