#include "displays.h"
#include "qpn_port.h"
#include "common-diode.h"


Q_DEFINE_THIS_FILE;


struct SevenSegmentDisplay displays[2];

#ifdef COMMON_ANODE
#define DISPLAYSEGMENTS(x) ((~(x)) & 0x7f)
#define BLANK 0x7f
#endif

#ifdef COMMON_CATHODE
#define DISPLAYSEGMENTS(x) (x)
#define BLANK 0x00
#endif

/**
 */
static const Q_ROM uint8_t segmentmap[] = {
	DISPLAYSEGMENTS( 0b00111111 ), /* '0' */
	DISPLAYSEGMENTS( 0b00000110 ), /* '1' */
	DISPLAYSEGMENTS( 0b01011011 ), /* '2' */
	DISPLAYSEGMENTS( 0b01001111 ), /* '3' */
	DISPLAYSEGMENTS( 0b01100110 ), /* '4' */
	DISPLAYSEGMENTS( 0b01101101 ), /* '5' */
	DISPLAYSEGMENTS( 0b01111101 ), /* '6' */
	DISPLAYSEGMENTS( 0b00000111 ), /* '7' */
	DISPLAYSEGMENTS( 0b01111111 ), /* '8' */
	DISPLAYSEGMENTS( 0b01101111 ), /* '9' */
};

static uint8_t get_segmentmap(char ch)
{
	if (' ' == ch || '\0' == ch) {
		return BLANK;
	}
	Q_ASSERT(ch >= '0');
	Q_ASSERT(ch <= '9');
	return Q_ROM_BYTE(segmentmap[ ch - '0' ]);
}


void displays_init(void)
{
	displays[0].digit = '\0';
	displays[0].segments = 0;
	displays[0].brightness = 0;
	displays[1].digit = '\0';
	displays[1].segments = 0;
	displays[1].brightness = 0;
}


void set_digit(uint8_t digit, char ch)
{
	uint8_t segments;

	segments = get_segmentmap(ch & 0x7f);
	if (ch & 0x80) segments |= 0x80;
	switch (digit) {
	case 0:
		QF_INT_LOCK();
		displays[0].digit = ch;
		displays[0].segments = segments;
		QF_INT_UNLOCK();
		break;
	case 1:
		QF_INT_LOCK();
		displays[1].digit = ch;
		displays[1].segments = segments;
		QF_INT_UNLOCK();
		break;
	default:
		Q_ASSERT(0);
	}
}


void set_digits(char ch0, char ch1)
{
	set_digit(0, ch0);
	set_digit(1, ch1);
}


void set_brightness(uint8_t brightness)
{
	displays[0].brightness = brightness;
	displays[1].brightness = brightness;
}
