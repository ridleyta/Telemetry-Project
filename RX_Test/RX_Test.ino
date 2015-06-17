/* This is a program to test the transmission of serial data encoded using a
	Manchester method. It is intended to be used with a radio link. 
	This is the receiver end. */

/* TX Functions */

uint8_t manEnc(uint8_t bitsIn);
/* A function which returns the 8 bit manchester encoding of the 4 least significant bits */

void manEnArray(uint8_t* in, uint8_t* out, uint8_t len);
/* A function to encode an input array of length len*/

void txPacket(uint8_t* data, uint32_t header, uint8_t* packet);
/* A function which creates a serial packet ready for transmission */

/* RX Functions */

uint8_t decPack(uint8_t* recBuf, uint8_t* decBuf, uint8_t* pos, uint8_t len);
/* A function to attempt to decode a whole packet. Packets are an expected number of bytes. */

uint8_t decByte(uint8_t recByte);
/* A function to decode received bytes */

/* Variables */

// buffers
uint8_t buf1[50], buf2[50], *b1, *b2, pos, sz, error;
// packet number
uint32_t header1, header2;


void setup()
{

	/* add setup code here */
	Serial3.begin(9600);
	Serial.begin(115200);

	/*
	// change to 9 data bits
	UCSR1B |= (1 << UCSZ12);
	// 9th bit always zero
	UCSR1B &= 0xFE;
	*/

	// initialise
	memset(buf1, 0, 50);
	memset(buf2, 0, 50);
	header1 = 0;
	header2 = 0;
	b1 = buf1;
	b2 = buf2;
}

void loop()
{
	// wait for the next byte
	while (!Serial3.available());

	// when data is available look for the preamble
	Serial3.readBytes(b1, 1);

	if (decByte(*b1) == 0xF0){ // valid preamble

		Serial.println("Valid preamble received");

		// get a packet and attempt decode
		sz = 22; // number of bytes to get
		while (sz > 0){
			while (Serial3.available() < sz);
			Serial3.readBytes(b1, sz);
			//Serial.write(b1, sz);
			error = decPack(b1, b2, &pos, 20);

			if (error == 0)	// successful decoding
				sz = 0;
			else if (error == 0xFF){ // illegal code
				sz -= pos;
				// look for start condition in the rest of the buffer
				error = decPack(b1, b2, &pos, sz);
				if (error == 0xF0) // start condition found
					sz = 22 - (sz - pos);
				else
					sz = 0; // no start
			}
			else if (error == 0xF0){ // start of new packet
				sz = 22 - (sz - pos);
			}
		}

		// check packet number 
		if ((header2 = (buf2[0]) | (buf2[1] << 8) | (buf2[2] << 16)) > header1){
			header1 = header2;
			Serial.write(b2+3,8);
			Serial.println(header1);
		}

	}

}

uint8_t manEnc(uint8_t bitsIn){
	/* A function which returns the 8 bit manchester encoding of the 4 least significant bits */
	uint8_t i;
	uint8_t bitsOut = 0;
	for (i = 0; i < 4; i++){
		// repeat 4 times for 4 bits
		bitsOut >>= 2;
		// shift left - the last bits are less significant
		if ((bitsIn & 0x01) == 0x01)
			bitsOut |= 0x80; // 1: 1->0
		else
			bitsOut |= 0x40; // 0: 0->1
		bitsIn >>= 1;
		// next bit is on the right
	}
	return bitsOut;
}

void manEnArray(uint8_t* in, uint8_t* out, uint8_t len){
	/* A function to encode an input array of length len*/
	uint8_t i;
	// loop for each byte
	for (i = 0; i < len; i++){
		// manchester req 2 bytes
		*(out + (2 * i)) = manEnc(*(in + i));			// lower
		*(out + (2 * i + 1)) = manEnc(*(in + i) >> 4);	// upper
	}
}

void txPacket(uint8_t* data, uint32_t header, uint8_t* packet){
	/* A function which creates a serial packet ready for transmission */

	// preamble
	*packet = 0xF0;
	// packet number (3 bytes)
	*(packet + 1) = manEnc(header);
	*(packet + 2) = manEnc(header >> 4);
	*(packet + 3) = manEnc(header >> 8);
	*(packet + 4) = manEnc(header >> 12);
	*(packet + 5) = manEnc(header >> 16);
	*(packet + 6) = manEnc(header >> 20);
	// data
	manEnArray(data, (packet + 7), 8);
	// end packet
}

uint8_t decByte(uint8_t recByte){
	/* A function to decode received bytes */

	if (recByte == 0xF0)
		return 0xF0; // start condition

	uint8_t dec, bitPat, i; 
	dec = 0;
	bitPat = 0;

	for (i = 0; i<4; i++) {
		dec >>= 1;
		// get current bit pattern
		bitPat = recByte & 0x03;
		// decode
		if (bitPat == 0x02) // 1: 1->0
			dec |= 0x08;
		else if (bitPat == 0x01)
			dec &= 0xF7;	// 0: 0->1
		else
			return 0xFF; // illegal code

		recByte >>= 2;
	}
	//return 4 bits
	return dec;
}

uint8_t decPack(uint8_t* recBuf, uint8_t* decBuf, uint8_t* pos, uint8_t len){
	/* A function to attempt to decode a whole packet. Packets are an expected number of bytes. */
	uint8_t hi,lo,i;
	*pos = 0;
	i = 0;

	while ( *pos < len ){
		// decode lower nibble
		lo = decByte(*(recBuf + *pos));
		*pos+=1;
		// error check
		if (lo == 0xF0)
			return 0xF0; // start condition
		else if (lo == 0xFF)
			return 0xFF; // illegal code

		// decode upper nibble
		hi = decByte(*(recBuf + *pos));		
		*pos+=1;
		// error check
		if (hi == 0xF0)
			return 0xF0; // start condition
		else if (hi == 0xFF)
			return 0xFF; // illegal code

		// combine byte
		*(decBuf + i) = (lo) | (hi << 4);
		i++;
	}

	// no errors
	return 0;

}