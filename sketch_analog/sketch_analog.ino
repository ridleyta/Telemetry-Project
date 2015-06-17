/*
Test two.
Create a buffered input to write to SD memory in 512 byte blocks.
*/

#include "RingBuf.h"
#include <SPI.h>
#include <avr/interrupt.h>
#include <SdFat.h>
#include <SdFatUtil.h>
#include <wiring_private.h>
#include <pins_arduino.h>

#define NUM_SENSORS	2
#define S_FREQ_1	1000
#define S_FREQ_2	250

void readADC(uint8_t);
/*This function starts the ADC*/

//-------------------------------------------//
//					GLOBAL					 //
//-------------------------------------------//
// SD chip select pin
const uint8_t chipSelect = SS;

// number of blocks in the contiguous file
const uint32_t BLOCK_COUNT = 100UL;

// time to produce a block of data
const uint32_t MICROS_PER_BLOCK = 10000;

// file system
SdFat sd;

// test file
SdFile file;

// file extent
uint32_t bgnBlock, endBlock;

// Serial output stream
ArduinoOutStream cout(Serial);

// store error strings in flash to save RAM
#define error(s) sd.errorHalt_P(PSTR(s))

//count interrupts for debug
volatile uint8_t count1, count2;

// interrupt buffer
ringBuf uBuf;

// analog input pins
const uint8_t ADC_CH1 = 0;
const uint8_t ADC_CH2 = 1;  
const uint8_t ADC_CH3 = 2;
const uint8_t ADC_CH4 = 3;

// frequency variables
uint8_t S_FREQ [2] = {S_FREQ_1, S_FREQ_2};
uint8_t* ISR1_f = NULL;

//-------------------------------------------//
//					SETUP					 //
//-------------------------------------------//
void setup(void) {
cli();           // disable interrupts

	//vectors for fast and slow interrupts store the number of times the interrupt will 
	//trigger before the sensor needs to be updated.
	uint8_t n = 0;
	for (uint8_t sensor = 0; sensor == (NUM_SENSORS-1); sensor++){
		if (S_FREQ[sensor] > 250){
			if (S_FREQ[sensor] >= 1000){
				ISR1_f[n] = 1;
				n++;
			}
			else{
				ISR1_f[n] = 1000 / S_FREQ_1;
				n++;
			}
		}
	}
// Timers 1, 3, 4 and 5 are 16 bits.
	//set timer1 interrupt at 1kHz
	TCCR1A = 0;// set entire TCCR1A register to 0
	TCCR1B = 0;// same for TCCR1B
	TCNT1 = 0;//initialize counter value to 0
	// set compare match register for 1000Hz increments
	OCR1A = 1999;// = (16*10^6) /(1000*8) - 1 (must be <65536)
	// turn on CTC mode
	TCCR1B |= (1 << WGM12);
	// Set CS11 bits for 8 prescaler
	TCCR1B |= (1 << CS11);
	// enable timer compare interrupt 
	TIMSK1 |= (1 << OCIE1A);

	//set timer3 interrupt at 250Hz
	TCCR3A = 0;// set entire TCCR3A register to 0
	TCCR3B = 0;// same for TCCR3B
	TCNT3 = 100;//initialize counter value to 100
	// set compare match register for 250hz increments
	OCR3A = 19999;// = (16*10^6) / (250*256) - 1 (must be <65536)
	// turn on CTC mode
	TCCR3B |= (1 << WGM32);
	// Set CS30 and CS32 bits for 1024 prescaler
	//TCCR3B |= (1 << CS32) | (1 << CS30);
	// Set 8 prescaler
	TCCR3B |= (1 << CS31);
	// disable timer compare interrupt until loop starts
	TIMSK3 &= ~(1 << OCIE3A);

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
	// set prescaler for 1MHz clock
	ADCSRA |= (1 << ADPS2);
	// enable ADC
	ADCSRA |= (1 << ADEN);
	// do a conversion to initialise circuit
	ADCSRA |= (1 << ADSC);
	while (bit_is_set(ADCSRA, ADSC));
	// reset the interrupt flag "(0 << ADIF=4)" => 11101111
	ADCSRA &= ~(1 << ADIF);
	// enable conversion complete interrupt
	ADCSRA |= (1 << ADIE);

sei();		// enable interrupts 

	//start serial console (debug)
	Serial.begin(9600);
}

//-------------------------------------------//
//				INTERRUPTS					 //
//-------------------------------------------//
ISR(TIMER1_COMPA_vect){
	// fast interrupt service routine
	// wait for any conversions to finish
	sei();
	while (bit_is_set(ADCSRA, ADSC));
	readADC(ADC_CH2);
	while (bit_is_set(ADCSRA, ADSC));
	readADC(ADC_CH3);
}

ISR(TIMER3_COMPA_vect){
// slow interrupt service routine 
	// wait for any conversions to finish
	sei();
	while (bit_is_set(ADCSRA, ADSC));
	readADC(ADC_CH1);
	while (bit_is_set(ADCSRA, ADSC));
	readADC(ADC_CH4);
}

ISR(ADC_vect){
// conversion complete interrupt
	cli();
	uBuf.rBuf_put(ADCH,ADMUX & 0x07);
	sei();
}

//-------------------------------------------//
//				MAIN LOOP                    //
//-------------------------------------------//
void loop(void) {
	// Background loop writes to the SD card:

	while (Serial.read() >= 0)
	{/* loop until serial buffer is empty */}

	// pstr stores strings in flash to save RAM
	cout << pstr("Type any character to start\n");
	while (Serial.read() <= 0)
	{/* loop until user responds */}
	cout << pstr("Free RAM: ") << FreeRam() << endl;

	// initialize the SD card at SPI_FULL_SPEED for best performance.
	// try SPI_HALF_SPEED if bus errors occur.
	if (!sd.begin(chipSelect, SPI_FULL_SPEED)) sd.initErrorHalt();

	// delete possible existing file
	sd.remove("RAW.TXT");

	// create a contiguous file
	if (!file.createContiguous(sd.vwd(), "RAW.TXT", 512UL * BLOCK_COUNT)) {
		error("createContiguous failed");
	}
	// get the location of the file's blocks
	if (!file.contiguousRange(&bgnBlock, &endBlock)) {
		error("contiguousRange failed");
	}
	/**********************NOTE from sdFat*****************************/
	/* NO SdFile calls are allowed while cache is used for raw writes */
	/******************************************************************/

	// tell card to setup for multiple block write with pre-erase
	if (!sd.card()->erase(bgnBlock, endBlock)) error("card.erase failed");
	if (!sd.card()->writeStart(bgnBlock, BLOCK_COUNT)) {
		error("writeStart failed");
	}

	// clear the cache and use it as a 512 byte buffer
	uint8_t* sdCache1 = (uint8_t*)sd.vol()->cacheClear();
	memset(sdCache1, ' ', 512);

	//enable timer interrupts
	TIMSK1 |= (1 << OCIE1A);
	TIMSK3 |= (1 << OCIE1A);

	//init time
	uint32_t t = micros();

	//-------------------------------------------//
	//			  SD WRITE LOOP				   //
	//-------------------------------------------//

	for (uint32_t b = 0; b < BLOCK_COUNT; b++){
		// wait for the buffer to fill
		while (uBuf.rBuf_get(sdCache1) <= 0)
		{/* get returns 0 until a block has been copied */
		}

		// write the cache to memory
		if (!sd.card()->writeData(sdCache1)) {
			// disable interrupts to stop error loop breaking
			TIMSK1 &= 0xFD;
			TIMSK3 &= 0xFD;
			error("writeData failed");
		}
	}

	//-------------------------------------------//

	//disable timer interrupts
	TIMSK1 &= 0xFD;
	TIMSK3 &= 0xFD;

	// total write time
	t = micros() - t;

	// end multiple block write mode
	if (!sd.card()->writeStop()) error("writeStop failed");

	// output successful run to console
	cout << pstr("Done\n");
	cout << pstr("Elapsed time: ") << setprecision(3) << 1.e-6*t;
	cout << pstr(" seconds\n");

	// close file 
	file.close();
	Serial.println();

	while (1)
	{/* Loop forever*/}
}

//-------------------------------------------//
//					ADC	                     //
//-------------------------------------------//
void readADC(uint8_t pin){
// this function starts the adc on a given channel
	// select the channel
	ADMUX = 0x60 | ( pin & 0x07 );

	// start the conversion
	sbi(ADCSRA, ADSC);
}

