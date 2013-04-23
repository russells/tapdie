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
static QState rollingState         (struct Tapdie *me);
static QState finalRollState       (struct Tapdie *me);
static QState finalRollFadingState (struct Tapdie *me);


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
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		Q_ASSERT( nEventsFree((QActive*)(&dashboard)) >= 6 );
		QActive_post((QActive*)&dashboard, DASH_BRIGHTNESS_SIGNAL, 127);
		QActive_post((QActive*)&dashboard, DASH_MIN_BRIGHTNESS_SIGNAL, 30);
		QActive_post((QActive*)&dashboard, DASH_MAX_BRIGHTNESS_SIGNAL, 200);
		QActive_post((QActive*)&dashboard, DASH_START_FLASHING_SIGNAL, 0);
		QActive_post((QActive*)&dashboard, DASH_LCHAR_SIGNAL, ' ' | 0x80);
		QActive_post((QActive*)&dashboard, DASH_RCHAR_SIGNAL, '6' | 0x80);
		QActive_arm((QActive*)me, 10 * BSP_TICKS_PER_SECOND);
		return Q_HANDLED();

	case TAP_SIGNAL:
		return Q_TRAN(rollingState);
	case Q_TIMEOUT_SIG:
		return Q_TRAN(deepSleepEntryState);
	case Q_EXIT_SIG:
		return Q_HANDLED();
	}
	return Q_SUPER(tapdieState);
}


/**
 * Get a random number and put that on the displays.
 *
 * We set the right display's decimal point to distinguish between 6 and 9, and
 * between 2 and 5.  We don't do anything with the display's flashing or fading
 * mode, or its brightness, leaving that up to the caller.
 */
static void generate_and_show_random(struct Tapdie *me)
{
	uint8_t rn;
	char ch0, ch1;

	rn = (random() % me->mode) + 1;
	if (rn < 10) {

		ch0 = ' ';
		ch1 = rn + '0';
	} else {
		ch0 = rn / 10 + '0';
		ch1 = rn % 10 + '0';
	}
	Q_ASSERT( nEventsFree((QActive*)&dashboard) >= 2 );
	QActive_post((QActive*)&dashboard, DASH_LCHAR_SIGNAL, ch0);
	QActive_post((QActive*)&dashboard, DASH_RCHAR_SIGNAL, ch1);
	me->randomnumber = rn;
}


static QState rollingState(struct Tapdie *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		Q_ASSERT( nEventsFree((QActive*)(&dashboard)) >= 2 );
		QActive_post((QActive*)&dashboard, DASH_STEADY_SIGNAL, ' ');
		QActive_post((QActive*)&dashboard, DASH_BRIGHTNESS_SIGNAL, 200);
		me->rolls = 15;
		me->rollwait = 6;
		QActive_arm((QActive*)me, me->rollwait);
		generate_and_show_random(me);
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		if (me->rolls) {
			/* Still rolling */
			generate_and_show_random(me);
			me->rolls --;
			me->rollwait += 2;
			QActive_arm((QActive*)me, me->rollwait);
			return Q_HANDLED();
		} else {
			return Q_TRAN(finalRollState);
		}
	case TAP_SIGNAL:
		return Q_TRAN(rollingState);
	}
	return Q_SUPER(aliveState);
}


static QState finalRollState(struct Tapdie *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		/* The dashboard is already in steady brightness, as we have
		   come here from rollingState() */
		generate_and_show_random(me);
		QActive_arm((QActive*)me, 5*BSP_TICKS_PER_SECOND);
		return Q_HANDLED();
	case TAP_SIGNAL:
		return Q_TRAN(rollingState);
	case Q_TIMEOUT_SIG:
		return Q_TRAN(finalRollFadingState);
	}
	return Q_SUPER(aliveState);
}


static QState finalRollFadingState(struct Tapdie *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		QActive_arm((QActive*)me, 10 * BSP_TICKS_PER_SECOND);
		Q_ASSERT( nEventsFree((QActive*)&dashboard) >= 1 );
		QActive_post((QActive*)&dashboard, DASH_START_FADING_SIGNAL, 0);
	case Q_TIMEOUT_SIG:
		return Q_TRAN(deepSleepEntryState);
	}
	return Q_SUPER(finalRollState);
}

/*
  FIXME stuff to think about.

  - The first tap wakes us up and we display the current mode.  What happens
  with the next tap?  If it's very close to the wakeup tap, do we use that as
  a mode change, or a roll?

  - Do we need waiting states all over the place to catch the very close
    together taps?

*/
