#include "dashboard.h"
#include "tapdie.h"


static QState dashboardInitial(struct Dashboard *me);
static QState dashboardState(struct Dashboard *me);
static QState offState(struct Dashboard *me);
static QState onState(struct Dashboard *me);


struct Dashboard dashboard;


void dashboard_ctor(void)
{
	QActive_ctor((QActive *)(&dashboard), (QStateHandler)&dashboardInitial);
}


static QState dashboardInitial(struct Dashboard *me)
{
	return Q_TRAN(&offState);
}


static QState dashboardState(struct Dashboard *me)
{
	return Q_SUPER(&QHsm_top);
}


static QState offState(struct Dashboard *me)
{
	switch (Q_SIG(me)) {
	case DASH_ON_SIGNAL:
		return Q_TRAN(onState);
	case Q_ENTRY_SIG:
		/* FIXME turn off the displays. */
		break;
	case Q_EXIT_SIG:
		/* FIXME turn on the displays. */
		break;
	}
	return Q_SUPER(&dashboardState);
}


static QState onState(struct Dashboard *me)
{
	switch (Q_SIG(me)) {
	case DASH_OFF_SIGNAL:
		return Q_TRAN(offState);
	}
	return Q_SUPER(&dashboardState);
}
