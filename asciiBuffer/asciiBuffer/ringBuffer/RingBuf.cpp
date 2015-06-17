/*
Ring Buffer
	This program provides a class for a ring buffer on the arduino mega.
	It is intended only for buffering sensor values to be written to an SD card.
	02/03/2015
*/
#include "RingBuf.h"

ringBuf::ringBuf(){
	ringBuf *_this;
	init(_this);
};

ringBuf::~ringBuf(){
	delete buf;
};

uint_fast8_t ringBuf::ringBuf_get(){
/* A function to get a byte from the buffer */
	uint_fast8_t c;
	if (count){
		c = buf[tail];
		tail++;
		count--;
	}
	else
		c = -1;
	return c;
};
void ringBuf::ringBuf_put(const unsigned char c){
/* A function to put a byte into the buffer */
	buf[head] = c;
	head++;
	count++;
};
void ringBuf::init(ringBuf *_this){
/* A function to initialise the buffer */
	memset(_this, 0, sizeof(*_this));
};
uint_fast8_t ringBuf::empty(){
/* A function to return true if the buffer is empty */
	return (0==count);
};
uint_fast8_t ringBuf::full(){
/* A function to return true if the buffer is full */
	return (count>=RBUF_SIZE);
};
void ringBuf::flush(const uint_fast8_t clearBuffer){
/* A function to flush the queue (head,tail,count=0) and set buffered bytes to 0 */
	head = 0;
	tail = 0;
	count = 0;
	if (clearBuffer)
		memset(buf, 0, RBUF_SIZE);
};