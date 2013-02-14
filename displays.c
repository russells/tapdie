#include "displays.h"
#include "qpn_port.h"


Q_DEFINE_THIS_FILE;


struct SevenSegmentDisplay displays[2];


static uint8_t segmentmap[] = {
	0b01111110,		/* '0' */
	0b00110000,		/* '1' */
	0b01101101,		/* '2' */
	0b01111001,		/* '3' */
	0b01111001,		/* '4' */
	0b01011011,		/* '5' */
	0b01011111,		/* '6' */
	0b01110000,		/* '7' */
	0b01111111,		/* '8' */
	0b01111011,		/* '9' */
};

static uint8_t get_segmentmap(char ch)
{
	if (' ' == ch || '\0' == ch) {
		return 0b00000000;
	}
	Q_ASSERT(ch >= '0');
	Q_ASSERT(ch <= '9');
	return segmentmap[ ch - '0' ];
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


void set_digit(uint8_t digit, char ch, uint8_t brightness)
{
	uint8_t segments;

	segments = get_segmentmap(ch & 0x7f);
	if (ch & 0x80) segments |= 0x80;
	switch (digit) {
	case 0:
		QF_INT_LOCK();
		displays[0].digit = ch;
		displays[0].segments = segments;
		displays[0].brightness = brightness;
		QF_INT_UNLOCK();
		break;
	case 1:
		QF_INT_LOCK();
		displays[1].digit = ch;
		displays[1].segments = segments;
		displays[1].brightness = brightness;
		QF_INT_UNLOCK();
		break;
	default:
		Q_ASSERT(0);
	}
}


void set_digits(char ch0, uint8_t br0, char ch1, uint8_t br1)
{
	set_digit(0, ch0, br0);
	set_digit(1, ch1, br1);
}


void set_brightness(uint8_t brightness)
{
	displays[0].brightness = brightness;
	displays[1].brightness = brightness;
}
