/*
  This is a modification of example code provided by Arduino to test the ADC

  The original code description:
		 Demonstrates analog input by reading an analog sensor on analog pin 0 and
		 turning on and off a light emitting diode(LED)  connected to digital pin 13.
		 The amount of time the LED will be on and off depends on
		 the value obtained by analogRead().

		 The circuit:
		 * Potentiometer attached to analog input 0
		 * center pin of the potentiometer to the analog pin
		 * one side pin (either one) to ground
		 * the other side pin to +5V
		 * LED anode (long leg) attached to digital output 13
		 * LED cathode (short leg) attached to ground

		 * Note: because most Arduinos have a built-in LED attached
		 to pin 13 on the board, the LED is optional.


		 Created by David Cuartielles
		 modified 30 Aug 2011
		 By Tom Igoe

		 This example code is in the public domain.

		 http://arduino.cc/en/Tutorial/AnalogInput

  The modified code tests configuring the ADC to use 8-bit precision rather than the 
  default 10-bits. It does this by printing the measured analogue value on pin A8 
  to the serial console every 1s. 

  The default Arduino ADC function call has been replaced with a custom function readADC.
  This is because only one result register needs to be read when in 8-bit precision mode.

  There are 16 analogue inputs on the Mega. A pin value of 0-7 in this code corresponds
  to pins 8-15 respectively.

  Tom Ridley 2015

 */

#include <wiring_private.h>
#include <pins_arduino.h>

int sensorPin = 0;    // select the input pin 

uint8_t sensorValue = 0;  // variable to store the value coming from the sensor

uint8_t readADC(uint8_t); // function

void setup() {

  //set ADC clock and left adjust
  ADMUX = 0;
  ADCSRA = 0;
  ADCSRB = 0;
  // select 5V reference
  ADMUX |= (1 << REFS0);
  // left adjust: 8 bit result in ADCH
  ADMUX |= (1 << ADLAR);
  // use ins 8-15 (default = 8)
  ADCSRB |= (1 << MUX5);
  // set prescaler to 1MHz clock
  ADCSRA |= (1 << ADPS2);
  // enable ADC
  ADCSRA |= (1 << ADEN);
  // do a conversion to initialise circuit
  ADCSRA |= (1 << ADSC);
  while (bit_is_set(ADCSRA, ADSC));
  // reset the interrupt flag
  ADCSRA &= ~(1 << ADIF);
  // enable conversion complete interrupt
  //ADCSRA &= ~(1 << ADIE);
 
  Serial.begin(9600);
}

void loop() {
  // read the value from the sensor:
  //sensorValue = analogRead(A8);
	sensorValue = readADC(sensorPin);
  
  uint8_t d1, d2, d3;				// store value of each digit

	// sesnor number as header
	Serial.write(1 + 0x30);
	Serial.write(0x20);				// " "

	// first digit
	d1 = (sensorValue / 100);
	Serial.write(d1 + 0x30);
	// second digit
	d2 = (sensorValue / 10) - (d1 * 10);
	Serial.write(d2 + 0x30);
	// third digit
	d3 = sensorValue - (100 * d1) - (10 * d2);
	Serial.write(d3 + 0x30);

  Serial.write("\n");
  delay(1000);
}

//-------------------------------------------//
//					ADC	                     //
//-------------------------------------------//
uint8_t readADC(uint8_t pin){
	// this function starts the adc on a given channel
	// select the channel
	ADMUX &= 0xF0;
	ADMUX |= (pin & 0x07);

	// start the conversion
	sbi(ADCSRA, ADSC);
	while (bit_is_set(ADCSRA, ADSC));
	return (ADCH);
}