#include "displays.h"
#include "qpn_port.h"


Q_DEFINE_THIS_FILE;


struct SevenSegmentDisplay displays[2];


static struct SegmentMap {
	char ch;
	uint8_t segments;
} segmentmap[] = {
	{ '0', 0b01111110 },
	{ '1', 0b00110000 },
};

static uint8_t get_segmentmap(char ch)
{
	struct SegmentMap *smp = segmentmap;
	while (smp->ch) {
		if (ch == smp->ch)
			return smp->segments;
		smp++;
	}
	Q_ASSERT(0);
	return 0;
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
	uint8_t segments = get_segmentmap(ch);
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
