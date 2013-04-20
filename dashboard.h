#include "qpn_port.h"

struct Dashboard {
	QActive super;
	char lchar;
	char rchar;
	/** The brightness to be used when steady. */
	uint8_t brightness;
	/** Lower brightness to be used in fading or flashing. */
	uint8_t min_brightness;
	/** Higher brightness to be used in fading or flashing. */
	uint8_t max_brightness;
	/** Current brightness when fading. */
	uint8_t fading_brightness;
};

extern struct Dashboard dashboard;

void dashboard_ctor(void);
