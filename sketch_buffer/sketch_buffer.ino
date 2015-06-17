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


void ReadADC();

//-------------------------------------------//
//					GLOBAL					 //
//-------------------------------------------//
// SD chip select pin
const uint8_t chipSelect = SS;

// number of blocks in the contiguous file
const uint32_t BLOCK_COUNT = 10UL;

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

// analog pins: 15 channels, A0 = 54
const uint8_t ADC_CH = A8;

//-------------------------------------------//
//					SETUP					 //
//-------------------------------------------//
void setup(void) {
cli();           // disable interrupts
  
//set timer1 interrupt at 1kHz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1000Hz increments
  OCR1A = 1999;// = (16*10^6) /(1000*8) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS11 bits for 8 prescaler
  TCCR1B |= (1 << CS11);  
  // disable timer compare interrupt until loop starts
  //TIMSK1 &= 0xFD;
  
//set timer3 interrupt at 250Hz
  TCCR3A = 0;// set entire TCCR2A register to 0
  TCCR3B = 0;// same for TCCR2B
  TCNT3  = 0;//initialize counter value to 0
  // set compare match register for 8khz increments
  OCR3A = 249;// = (16*10^6) / (250*256) - 1 (must be <256)
  // turn on CTC mode
  TCCR3B |= (1 << WGM32);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR3B |= (1 << CS32) | (1 << CS30);   
  // disable timer compare interrupt until loop starts
  TIMSK3 &= 0xFD; 
  
//initialise interrupt counters
  count1=0;
  count2=0;
  
sei();		// enable interrupts 

//start serial console (debug)
  Serial.begin(9600);
}

//-------------------------------------------//
//				INTERRUPTS					 //
//-------------------------------------------//
ISR(TIMER1_COMPA_vect){        
  // fast interrupt service routine
	cli();
	count1++;
	uBuf.rBuf_put(count1,0);
	// delay to test the limits of the processor
	// delayMicroseconds(900);
	sei();
} 

ISR(TIMER3_COMPA_vect){        
  // slow interrupt service routine 
	cli();
	count2++;
	uBuf.rBuf_put(count2,1);
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
  if (!file.createContiguous(sd.vwd(), "RAW.TXT", 512UL*BLOCK_COUNT)) {
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
  TIMSK3 |= (1 << OCIE3A);

  //init time
  uint32_t t = micros();

  //-------------------------------------------//
  //			  SD WRITE LOOP				   //
  //-------------------------------------------//

  for (uint32_t b = 0; b < BLOCK_COUNT; b++){
	// wait for the buffer to fill
	while (uBuf.rBuf_get(sdCache1) <= 0)
	{/* get returns 0 until a block has been copied */}

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
  cout << pstr("Elapsed time: ") << setprecision(3)<< 1.e-6*t;
  cout << pstr(" seconds\n");

  // close file 
  file.close();
  Serial.println();

  while (1)
	  {/* Loop forever*/}
}

uint8_t ReadADC(uint8_t pin){
	uint8_t high, low;
#if defined(ADCSRB) && defined(MUX5)
	// the MUX5 bit of ADCSRB selects whether we're reading from channels
	// 0 to 7 (MUX5 low) or 8 to 15 (MUX5 high).
	ADCSRB = (ADCSRB & ~(1 << MUX5)) | (((pin >> 3) & 0x01) << MUX5);
#endif

	// set the analog reference (high two bits of ADMUX) and select the
	// channel (low 4 bits).  this also sets ADLAR (left-adjust result)
	// to 0 (the default).
#if defined(ADMUX)
	ADMUX = (DEFAULT << 6) | (pin & 0x07);
#endif

	// without a delay, we seem to read from the wrong channel
	//delay(1);

#if defined(ADCSRA) && defined(ADCL)
	// start the conversion
	sbi(ADCSRA, ADSC);

	// ADSC is cleared when the conversion finishes
	while (bit_is_set(ADCSRA, ADSC));

	// we have to read ADCL first; doing so locks both ADCL
	// and ADCH until ADCH is read.  reading ADCL second would
	// cause the results of each conversion to be discarded,
	// as ADCL and ADCH would be locked when it completed.
	low = ADCL;
	high = ADCH;
#else
	// we dont have an ADC, return 0
	low = 0;
	high = 0;
#endif

	// combine the two bytes
	return (high << 8) | low;
}
