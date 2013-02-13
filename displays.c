#include "displays.h"


struct SevenSegmentDisplay displays[2];


void displays_init(void)
{
	displays[0].digit = '\0';
	displays[0].segments = 0;
	displays[0].brightness = 0;
	displays[1].digit = '\0';
	displays[1].segments = 0;
	displays[1].brightness = 0;
}
