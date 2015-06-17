/* 
Ascii Buffer
	This program converts raw values into ascii encoded text.
	The text is buffered into a 512 byte block ready to be 
	written to SD memory.
*/
#include "ringBuffer\RingBuf.h"

#include <iostream>

ringBuf asciiBuffer(ringBuf *_this);