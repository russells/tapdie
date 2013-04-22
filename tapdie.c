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
	tapdie.mode = 6;
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


static QState aliveState(struct Tapdie *me)
{
	uint8_t rn;

	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		me->digits[0] = 0;
		me->digits[1] = 0;
		me->counter = 0;
		Q_ASSERT( nEventsFree((QActive*)(&dashboard)) >= 6 );
		QActive_post((QActive*)&dashboard, DASH_BRIGHTNESS_SIGNAL, 127);
		QActive_post((QActive*)&dashboard, DASH_MIN_BRIGHTNESS_SIGNAL, 30);
		QActive_post((QActive*)&dashboard, DASH_MAX_BRIGHTNESS_SIGNAL, 200);
		QActive_post((QActive*)&dashboard, DASH_START_FLASHING_SIGNAL, 0);
		QActive_post((QActive*)&dashboard, DASH_LCHAR_SIGNAL, ' ' | 0x80);
		QActive_post((QActive*)&dashboard, DASH_RCHAR_SIGNAL, '6' | 0x80);
		QActive_arm((QActive*)me, 300); /* Ten seconds. */
		return Q_HANDLED();

	case TAP_SIGNAL:
		QActive_arm((QActive*)me, 300); /* Ten seconds. */
		rn = (uint8_t) (random() % me->mode) + 1;
		Q_ASSERT( nEventsFree((QActive*)(&dashboard)) >= 3 );
		QActive_post((QActive*)&dashboard, DASH_LCHAR_SIGNAL, ' ');
		QActive_post((QActive*)&dashboard, DASH_RCHAR_SIGNAL, rn+'0');
		QActive_post((QActive*)&dashboard, DASH_START_FADING_SIGNAL, 0);
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		return Q_TRAN(deepSleepEntryState);
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
