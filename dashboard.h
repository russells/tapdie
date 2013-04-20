#include "qpn_port.h"

struct Dashboard {
	QActive super;
	char lchar;
	char rchar;
	uint8_t brightness;
	uint8_t min_brightness;
	uint8_t max_brightness;
};

extern struct Dashboard dashboard;

void dashboard_ctor(void);
