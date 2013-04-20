#include "qpn_port.h"

struct Dashboard {
	QActive super;
	char lchar;
	char rchar;
	uint8_t brightness;
};

extern struct Dashboard dashboard;

void dashboard_ctor(void);
