#include "dashboard.h"
#include "displays.h"
#include "tapdie.h"


Q_DEFINE_THIS_FILE;


static QState dashboardInitial(struct Dashboard *me);
static QState dashboardState(struct Dashboard *me);
static QState offState(struct Dashboard *me);
static QState onState(struct Dashboard *me);


struct Dashboard dashboard;


void dashboard_ctor(void)
{
	QActive_ctor((QActive *)(&dashboard), (QStateHandler)&dashboardInitial);
	dashboard.brightness = 127;
	dashboard.lchar = 0;
	dashboard.rchar = 0;
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

	/* These signals should not happen until after we've woken up properly,
	   so let's find out if that happens or not by stopping the program if
	   they do. */
	case DASH_LCHAR_SIGNAL:
		Q_ASSERT(0);
	case DASH_RCHAR_SIGNAL:
		Q_ASSERT(0);
	case DASH_BRIGHTNESS_SIGNAL:
		Q_ASSERT(0);
		/* Ignore all of these until we get out of the off state. */
		return Q_HANDLED();
	case DASH_ON_SIGNAL:
		return Q_TRAN(onState);
	case Q_ENTRY_SIG:
		set_brightness(0);
		break;
	case Q_EXIT_SIG:
		break;
	}
	return Q_SUPER(&dashboardState);
}


static QState onState(struct Dashboard *me)
{
	char ch;

	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		me->brightness = 127;
		me->lchar = 0;
		me->rchar = 0;
		return Q_HANDLED();
	case DASH_LCHAR_SIGNAL:
		ch = (char) Q_PAR(me);
		me->lchar = ch;
		set_digit(0, ch, me->brightness);
		return Q_HANDLED();
	case DASH_RCHAR_SIGNAL:
		ch = (char) Q_PAR(me);
		me->rchar = ch;
		set_digit(1, ch, me->brightness);
		return Q_HANDLED();
	case DASH_BRIGHTNESS_SIGNAL:
		me->brightness = Q_PAR(me);
		set_brightness(me->brightness);
		return Q_HANDLED();
	case DASH_OFF_SIGNAL:
		return Q_TRAN(offState);
	}
	return Q_SUPER(&dashboardState);
}
