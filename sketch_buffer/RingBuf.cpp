/*
Ring Buffer
	This program provides a class for a ring buffer on the arduino mega.
	It is intended only for buffering sensor values to be written to an SD card.
	02/03/2015
*/
#include "RingBuf.h"

ringBuf::ringBuf(){
	rBuf_flush(1);
};
ringBuf::~ringBuf(){
	delete buf;
};
uint_fast8_t ringBuf::rBuf_get(uint8_t* loc){
/* A function to copy 512 bytes from the buffer */
	if ((count) > FULL_MASK){
		// copy block
		memcpy(loc, &buf[tail],RBUF_SIZE);

		// increment tail
		tail+=RBUF_SIZE;		

		// mask overflow
		tail = tail & COUNT_MASK;

		// subtraction prevents count overflow 
		count-=RBUF_SIZE;		

		return 1;
	}
	else
		return 0;
};
void ringBuf::rBuf_put(uint8_t c, uint8_t h){
/* A function to put an unsigned integer into the ascii buffer */
	uint8_t d1, d2, d3;				// store value of each digit

	// sesnor number as header
	rBuf_bput(h + 0x30);
	rBuf_bput(0x20);				// " "

	// first digit
	d1 = (c / 100);
	rBuf_bput(d1 + 0x30);
	// second digit
	d2 = (c / 10) - (d1 * 10);
	rBuf_bput(d2 + 0x30);
	// third digit
	d3 = c - (100 * d1) - (10 * d2);
	rBuf_bput(d3 + 0x30);

	// new line
	rBuf_bput(0xD);					// <CR>
	rBuf_bput(0xA);					// <LF>
};
void ringBuf::rBuf_bput(uint8_t c){
/* A function to put a byte into the buffer */
	buf[head & COUNT_MASK] = c;		// mask overflow
	head++;
	count++;
};
void ringBuf::init(ringBuf *_this){
/* A function to initialise the buffer */
	memset(_this, 0, sizeof(*_this));
};
uint_fast8_t ringBuf::rBuf_empty(){
/* A function to return true if the buffer is empty */
	return (0 == (count));
};
uint_fast8_t ringBuf::rBuf_full(){
/* A function to return true if the buffer is full */
	return ((count) >= (RBUF_SIZE - 1));
};
void ringBuf::rBuf_flush(const uint_fast8_t clearBuffer = 0){
/* A function to flush the queue (head,tail,count=0) and set buffered bytes to 0 */
	head = 0;
	tail = 0;
	count = 0;
	if (clearBuffer)
		memset(buf, 0, RBUF_SIZE);
};
uint_fast16_t ringBuf::rBuf_len(){
	/* A function which returns the value of count */
	return (count & COUNT_MASK);
};
