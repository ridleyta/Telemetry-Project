/*
Ring Buffer
	This program provides a class for a ring buffer on the Arduino mega.
	It is intended only for buffering sensor values to be written to an SD card.
	Data will be written to SD memory in 512 byte blocks. While the first 512
	bytes are written, data is buffered in the second 512 bytes.
	02/03/2015
*/
#include <string.h>
#include <stdint.h>

#ifndef __RINGBUF_H
	#define __RINGBUF_H
	#define RBUF_SIZE		512		/* Do NOT change */
	#define TWO_BUF_SIZE	1024	/* use two		 */
	#define FULL_MASK		0x03FF	/* 9 bits (512)  */
	#define COUNT_MASK		0x07FF	/*10 bits (1024) */

	class ringBuf{
	public:
		ringBuf();
		~ringBuf();

		uint_fast8_t* rBuf_get(uint8_t* l);
		/* A function to get 512 bytes from the buffer */

		void rBuf_put(uint8_t c, uint8_t h = 0);
		/* A function to put an unsigned integer into the ascii buffer */

		uint_fast8_t rBuf_empty();
		/* A function to return true if the buffer is empty */

		uint_fast8_t rBuf_full();
		/* A function to return true if the buffer is full */

		uint_fast16_t rBuf_len();
		/* A function which returns the value of count */

		void rBuf_flush(const uint_fast8_t clearBuffer);
		/* A function to flush the queue (head,tail,count=0) and set buffered bytes to 0 */

	protected:
		uint_fast8_t buf[TWO_BUF_SIZE];
		/* Stored buffer values - two consecutive buffers*/

		uint_fast16_t head;
		/* The index of the most recent value added to the queue */

		uint_fast16_t tail;
		/* The index of the most recent value processed from the queue */

		uint_fast16_t count;
		/* The total number of elements in the queue that have not been processed */
	
		void init(ringBuf *_this);
		/* A function to initialise the buffer */

		void rBuf_bput(uint8_t c);
		/* A function to put a byte into the buffer */

	};

#endif