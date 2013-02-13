#ifndef leds_h_INCLUDED
#define leds_h_INCLUDED

struct SevenSegmentDisplay {
	char digit;		/* The digit we're displaying. */
	uint8_t dp;		/* Whether the DP is on or not. */
	uint8_t segments;	/* Individual segments, including the DP. */
	uint8_t brightness;	/* PWM brightness value, 0 - 255. */
};

extern struct SevenSegmentDisplay displays[2];

void set_digit(uint8_t digitnum, char ch, uiint8_t br);

#endif
