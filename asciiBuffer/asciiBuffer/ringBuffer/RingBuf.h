/*
Ring Buffer
	This program provides a class for a ring buffer on the arduino mega.
	It is intended only for buffering sensor values to be written to an SD card.
	02/03/2015
*/
#include <string.h>
#include <stdint.h>

#ifndef __RINGBUF_H
	#define __RINGBUF_H
	#define RBUF_SIZE 512 /* Do NOT change */

	class ringBuf{
	public:
		ringBuf();
		~ringBuf();

		uint_fast8_t ringBuf_get();
		/* A function to get a byte from the buffer */

		void ringBuf_put(const unsigned char c);
		/* A function to put a byte into the buffer */

		uint_fast8_t empty();
		/* A function to return true if the buffer is empty */

		uint_fast8_t full();
		/* A function to return true if the buffer is full */

		uint_fast16_t len();
		/* A function which returns the value of count */

	protected:
		uint_fast8_t buf[RBUF_SIZE];
		/* Stored buffer values */

		uint_fast16_t head;
		/* The index of the most recent value added to the queue */

		uint_fast16_t tail;
		/* The index of the most recent value processed from the queue */

		uint_fast16_t count;
		/* The total number of elements in the queue that have not been processed */
	
		void init(ringBuf *_this);
		/* A function to initialise the buffer */

		void flush(const uint_fast8_t clearBuffer);
		/* A function to flush the queue (head,tail,count=0) and set buffered bytes to 0 */

	} ringBuf;

#endif