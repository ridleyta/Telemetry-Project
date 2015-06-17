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

uint_fast8_t* ringBuf::rBuf_get(uint8_t* l){
/* A function to get 512 bytes from the buffer */
	uint_fast8_t* c;
	if ((count & COUNT_MASK) >= 512U){
		memcpy(l, &buf[tail & COUNT_MASK],512);
		tail+=512U;
		count-=512U;
	}
	else
		c = NULL;
	return c;
};
void ringBuf::rBuf_put(uint8_t c, uint8_t h = 0){
/* A function to put an unsigned integer into the ascii buffer */
	rBuf_bput(h);
	rBuf_bput(0x20);                         // " "
	rBuf_bput((c / 100) + 0x30);
	rBuf_bput(((c / 10) % 10) + 0x30);
	rBuf_bput((c % 10) + 0x30);
	rBuf_bput(0xD);                          // <CR>
};
void ringBuf::rBuf_bput(uint8_t c){
/* A function to put a byte into the buffer */
	buf[head & COUNT_MASK] = c;
	head++;
	count++;
};
void ringBuf::init(ringBuf *_this){
/* A function to initialise the buffer */
	memset(_this, 0, sizeof(*_this));
};
uint_fast8_t ringBuf::rBuf_empty(){
/* A function to return true if the buffer is empty */
	return (0 == (count & COUNT_MASK));
};
uint_fast8_t ringBuf::rBuf_full(){
/* A function to return true if the buffer is full */
	return ((count & FULL_MASK) >= RBUF_SIZE);
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