#include "qpn_port.h"

struct Dashboard {
	QActive super;
};

extern struct Dashboard dashboard;

void dashboard_ctor(void);
