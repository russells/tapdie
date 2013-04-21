/**
 * @file
 *
 */

#include <stdlib.h>
#include "tapdie.h"
#include "bsp.h"
#include "displays.h"
#include "dashboard.h"


/** The only active Tapdie. */
struct Tapdie tapdie;


Q_DEFINE_THIS_FILE;

static QState tapdieInitial        (struct Tapdie *me);
static QState tapdieState          (struct Tapdie *me);
static QState deepSleepState       (struct Tapdie *me);
static QState deepSleepEntryState  (struct Tapdie *me);
static QState aliveState           (struct Tapdie *me);


static QEvent tapdieQueue[4];
static QEvent dashboardQueue[9];

QActiveCB const Q_ROM Q_ROM_VAR QF_active[] = {
	{ (QActive *)0              , (QEvent *)0      , 0                        },
	{ (QActive *)(&tapdie)      , tapdieQueue      , Q_DIM(tapdieQueue)       },
	{ (QActive *)(&dashboard)   , dashboardQueue   , Q_DIM(dashboardQueue)    },
};
/* If QF_MAX_ACTIVE is incorrectly defined, the compiler says something like:
   tapdie.c:68: error: size of array ‘Q_assert_compile’ is negative
 */
Q_ASSERT_COMPILE(QF_MAX_ACTIVE == Q_DIM(QF_active) - 1);


int main(int argc, char **argv)
{
 startmain:
	BSP_startmain();
	displays_init();
	BSP_init(); /* initialize the Board Support Package */
	tapdie_ctor();
	dashboard_ctor();

	QF_run();

	goto startmain;
}

void tapdie_ctor(void)
{
	QActive_ctor((QActive *)(&tapdie), (QStateHandler)&tapdieInitial);
	tapdie.counter = 0;
	tapdie.mode = D6;
}


static QState tapdieInitial(struct Tapdie *me)
{
	return Q_TRAN(&deepSleepState);
}


static QState tapdieState(struct Tapdie *me)
{
	switch (Q_SIG(me)) {
	case WATCHDOG_SIGNAL:
		BSP_watchdog();
		return Q_HANDLED();
	}
	return Q_SUPER(&QHsm_top);
}


static QState deepSleepEntryState(struct Tapdie *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		/* Tell the dashboard to go off, then wait for that to be
		   handled. */
		post(&dashboard, DASH_OFF_SIGNAL, 0);
		QActive_arm((QActive*)me, 2);
		return Q_HANDLED();
	case TAP_SIGNAL:
		/* Ignore tap signals while we're waiting for the display to
		   turn off. */
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		return Q_TRAN(deepSleepState);
	}
	return Q_SUPER(tapdieState);
}


static QState deepSleepState(struct Tapdie *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		BSP_deep_sleep();
		return Q_HANDLED();
	case TAP_SIGNAL:
		post(&dashboard, DASH_ON_SIGNAL, 0);
		return Q_TRAN(aliveState);
	}
	return Q_SUPER(tapdieState);
}


static void rotate_mode(void)
{
	switch (tapdie.mode) {
	case D4: tapdie.mode = D6; break;
	case D6: tapdie.mode = D8; break;
	case D8: tapdie.mode = D10; break;
	case D10: tapdie.mode = D12; break;
	case D12: tapdie.mode = D20; break;
	case D20: tapdie.mode = D100; break;
	case D100: tapdie.mode = D4; break;
	default: Q_ASSERT(0); break;
	}
}


static QState aliveState(struct Tapdie *me)
{
	char ch0, ch1;

	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		me->digits[0] = 0;
		me->digits[1] = 0;
		me->counter = 0;
		Q_ASSERT( nEventsFree((QActive*)(&dashboard)) >= 4 );
		QActive_post((QActive*)&dashboard, DASH_BRIGHTNESS_SIGNAL, 127);
		QActive_post((QActive*)&dashboard, DASH_MIN_BRIGHTNESS_SIGNAL, 30);
		QActive_post((QActive*)&dashboard, DASH_MAX_BRIGHTNESS_SIGNAL, 200);
		QActive_post((QActive*)&dashboard, DASH_START_FLASHING_SIGNAL, 0);
		post(me, SET_MODE_SIGNAL, 0);
		return Q_HANDLED();

	case SET_MODE_SIGNAL:
		switch (me->mode) {
		case D4:
			ch0 = '\0'; ch1 = '4'; break;
		case D6:
			ch0 = '\0'; ch1 = '6'; break;
		case D8:
			ch0 = '\0'; ch1 = '8'; break;
		case D10:
			ch0 = '1'; ch1 = '0'; break;
		case D12:
			ch0 = '1'; ch1 = '2'; break;
		case D20:
			ch0 = '2'; ch1 = '0'; break;
		case D100:
			ch0 = '0'; ch1 = '0'; break;
		default:
			ch0 = '9'; ch1 = '9'; Q_ASSERT(0); break;
		}
		Q_ASSERT( nEventsFree((QActive*)(&dashboard)) >= 3 );
		QActive_post((QActive*)&dashboard, DASH_LCHAR_SIGNAL, ch0);
		QActive_post((QActive*)&dashboard, DASH_RCHAR_SIGNAL, ch1);
		QActive_post((QActive*)&dashboard, DASH_START_FLASHING_SIGNAL, 0);
		return Q_HANDLED();

	case NEXT_DIGIT_SIGNAL:
		if (me->counter >= 50) {
			return Q_TRAN(deepSleepEntryState);
		}
		me->randomnumber = random() % 100;
		me->digits[1] = me->randomnumber % 10 + '0';
		if (me->randomnumber >= 10) {
			me->digits[0] = me->randomnumber / 10 + '0';
		} else {
			me->digits[0] = '\0';
		}
		if (me->counter & 0b1) {
			if (me->digits[0]) {
				ch0 = me->digits[0] | 0x80;
			} else {
				ch0 = 0;
			}
		} else {
			ch0 = me->digits[0];
		}
		/* Checking the remaining space in the queue once then posting
		   the three events, instead of checking three tmes with the
		   post() macro, combined with posting here rather than in each
		   branch of the if-else code above, saves over 200 bytes of
		   program memory. */
		Q_ASSERT( nEventsFree((QActive*)&dashboard) >= 3 );
		QActive_post((QActive*)&dashboard, DASH_LCHAR_SIGNAL, ch0);
		QActive_post((QActive*)&dashboard, DASH_RCHAR_SIGNAL, me->digits[1]);
		//QActive_post((QActive*)&dashboard, DASH_BRIGHTNESS_SIGNAL, 127);
		QActive_arm((QActive*)me, 7);
		me->counter ++;
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		post(me, NEXT_DIGIT_SIGNAL, 0);
		return Q_HANDLED();
	case TAP_SIGNAL:
		return Q_TRAN(deepSleepState);
	case Q_EXIT_SIG:
		return Q_HANDLED();
	}
	return Q_SUPER(tapdieState);
}


/*
  FIXME stuff to think about.

  - The first tap wakes us up and we display the current mode.  What happens
  with the next tap?  If it's very close to the wakeup tap, do we use that as
  a mode change, or a roll?

  - Do we need waiting states all over the place to catch the very close
    together taps?

*/
