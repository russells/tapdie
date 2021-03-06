#include "dashboard.h"
#include "displays.h"
#include "tapdie.h"
#include "bsp.h"


Q_DEFINE_THIS_FILE;


static QState dashboardInitial(struct Dashboard *me);
static QState dashboardState(struct Dashboard *me);
static QState offState(struct Dashboard *me);
static QState onState(struct Dashboard *me);
static QState fadingState(struct Dashboard *me);
static QState fadingUpState(struct Dashboard *me);
static QState fadingDownState(struct Dashboard *me);


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
		//me->brightness = 127;
		me->lchar = 0;
		me->rchar = 0;
		return Q_HANDLED();
	case DASH_LCHAR_SIGNAL:
		ch = (char) Q_PAR(me);
		me->lchar = ch;
		set_digit(0, ch);
		return Q_HANDLED();
	case DASH_RCHAR_SIGNAL:
		ch = (char) Q_PAR(me);
		me->rchar = ch;
		set_digit(1, ch);
		return Q_HANDLED();
	case DASH_BRIGHTNESS_SIGNAL:
		me->brightness = Q_PAR(me);
		set_brightness(me->brightness);
		return Q_HANDLED();
	case DASH_MAX_BRIGHTNESS_SIGNAL:
		me->max_brightness = Q_PAR(me);
		return Q_HANDLED();
	case DASH_MIN_BRIGHTNESS_SIGNAL:
		me->min_brightness = Q_PAR(me);
		return Q_HANDLED();
	case DASH_OFF_SIGNAL:
		return Q_TRAN(offState);
	case DASH_START_FADING_SIGNAL:
		/* Start fading from the current brightness. */
		me->fading_brightness = me->brightness;
		return Q_TRAN(fadingDownState);
	case DASH_STEADY_SIGNAL:
		set_brightness(me->brightness);
		return Q_TRAN(onState);
	}
	return Q_SUPER(&dashboardState);
}


static QState fadingState(struct Dashboard *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		Q_ASSERT( me->max_brightness > me->min_brightness );
		return Q_HANDLED();
	}
	return Q_SUPER(onState);
}


static QState fadingUpState(struct Dashboard *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		QActive_arm((QActive*)me, 1);
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		if (me->fading_brightness < me->max_brightness) {
			if (me->fading_brightness > 100
			    && me->fading_brightness < 252) {
				me->fading_brightness += 4;
			} else if (me->fading_brightness > 50) {
				me->fading_brightness += 2;
			} else {
				me->fading_brightness ++;
			}
			set_brightness(me->fading_brightness);
			QActive_arm((QActive*)me, 1);
			return Q_HANDLED();
		} else {
			post(&tapdie, DASH_AT_HIGH_SIGNAL, 0);
			return Q_TRAN(fadingDownState);
		}
	case Q_EXIT_SIG:
		return Q_HANDLED();
	}
	return Q_SUPER(fadingState);
}


static QState fadingDownState(struct Dashboard *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		QActive_arm((QActive*)me, 1);
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		if (me->fading_brightness > me->min_brightness) {
			if (me->fading_brightness > 100) {
				me->fading_brightness -= 4;
			} else if (me->fading_brightness > 50) {
				me->fading_brightness -= 2;
			} else {
				me->fading_brightness --;
			}
			set_brightness(me->fading_brightness);
			QActive_arm((QActive*)me, 1);
			return Q_HANDLED();
		} else {
			post(&tapdie, DASH_AT_LOW_SIGNAL, 0);
			return Q_TRAN(fadingUpState);
		}
	}
	return Q_SUPER(fadingState);
}
