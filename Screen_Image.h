/*
 * Screen_Image.h
 *
 *  Created on: 2016-06-01
 *      Author: Wenyi Zou
 *      this one can only use for MSP430 Trunk display project
 */

#ifndef SCREEN_IMAGE_H_
#define SCREEN_IMAGE_H_
#define BLANK 0x00

unsigned char NUM_0[]={
		0b11110000, 0b00000111,
		0b00010000, 0b00000100,
		0b00010000, 0b00000100,
		0b11110000, 0b00000111
};

unsigned char NUM_1[]={
		0b00000000, 0b00000000,
		0b00000000, 0b00000000,
		0b11110000, 0b00000111,
		0b00000000, 0b00000000,
};

unsigned char NUM_2[]={
		0b10010000, 0b00000111,
		0b10010000, 0b00000100,
		0b10010000, 0b00000100,
		0b11110000, 0b00000100
};

unsigned char NUM_3[]={
		0b00010000, 0b00000100,
		0b10010000, 0b00000100,
		0b10010000, 0b00000100,
		0b11110000, 0b00000111
};

unsigned char NUM_4[]={
		0b11110000, 0b00000000,
		0b10000000, 0b00000000,
		0b10000000, 0b00000000,
		0b11110000, 0b00000111,
};

unsigned char NUM_5[]={
		0b11110000, 0b00000100,
		0b10010000, 0b00000100,
		0b10010000, 0b00000100,
		0b10010000, 0b00000111
};

unsigned char NUM_6[]={
		0b11110000, 0b00000111,
		0b10010000, 0b00000100,
		0b10010000, 0b00000100,
		0b00010000, 0b00000111
};

unsigned char NUM_7[]={
		0b00010000, 0b00000000,
		0b00010000, 0b00000000,
		0b00010000, 0b00000000,
		0b11110000, 0b00000111
};

unsigned char NUM_8[]={
		0b11110000, 0b00000111,
		0b10010000, 0b00000100,
		0b10010000, 0b00000100,
		0b11110000, 0b00000111
};

unsigned char NUM_9[]={
		0b11110000, 0b00000100,
		0b10010000, 0b00000100,
		0b10010000, 0b00000100,
		0b11110000, 0b00000111
};

unsigned char CAR_YES[]={
		0b00000000, 0b00011111,
		0b10000000, 0b00001010,
		0b10000000, 0b00001110,
		0b10000000, 0b00001110,
		0b10000000, 0b00001110,
		0b10000000, 0b00001110,
		0b10000000, 0b00001010,
		0b00000000, 0b00011111
};

unsigned char CAR_NO[]={              // pure blank 0
		0b00000000, 0b00000000,
		0b00000000, 0b00000000,
		0b00000000, 0b00000000,
		0b00000000, 0b00000000,
		0b00000000, 0b00000000,
		0b00000000, 0b00000000,
		0b00000000, 0b00000000,
		0b00000000, 0b00000000
};

unsigned char PED_YES[]={               //pedestrian
		0b00000000, 0b00001100,
		0b00000000, 0b01000010,
		0b11100000, 0b01111110,
		0b11100000, 0b00011111,
		0b11100000, 0b11111110,
		0b00000000, 0b00000010,
		0b10000000, 0b00000001
};

unsigned char PED_NO[]={               //pedestrian
		0b00000000, 0b00000000,
		0b00000000, 0b00000000,
		0b00000000, 0b00000000,
		0b00000000, 0b00000000,
		0b00000000, 0b00000000,
		0b00000000, 0b00000000,
		0b00000000, 0b00000000
};

#endif /* SCREEN_IMAGE_H_ */