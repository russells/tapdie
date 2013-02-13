#ifndef displays_h_INCLUDED
#define displays_h_INCLUDED

#include "inttypes.h"

struct SevenSegmentDisplay {
	char digit;		/* The digit we're displaying. Bit 7 is DP. */
	uint8_t segments;	/* Individual segments, including the DP. */
	uint8_t brightness;	/* PWM brightness value, 0 - 255. */
};

extern struct SevenSegmentDisplay displays[];

void displays_init(void);
void set_digit(uint8_t digitnum, char ch, uint8_t br);
void set_digits(char ch1, uint8_t br1, char ch2, uint8_t br2);

#endif
