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
static QState tappedState          (struct Tapdie *me);
static QState rollingState         (struct Tapdie *me);
static QState finalRollState       (struct Tapdie *me);
static QState finalRollFlashState  (struct Tapdie *me);
static QState finalRollFadingState (struct Tapdie *me);
static QState finalRollEndState    (struct Tapdie *me);


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


#define MAX_BRIGHTNESS 250
#define MIN_BRIGHTNESS 50


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
	tapdie.mode = D6;
}


static QState tapdieInitial(struct Tapdie *me)
{
	return Q_TRAN(&deepSleepState);
}


static void seed_rng(uint8_t seed8)
{
	uint32_t rn;
	uint8_t index;

	rn = random();
	index = random() & 0b11;
	((uint8_t*)(&rn))[index] = seed8;
	srandom(rn);
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
		return Q_TRAN(tappedState);
	}
	return Q_SUPER(tapdieState);
}


static void rotate_mode(struct Tapdie *me)
{
	switch (me->mode) {
	case D4: me->mode = D6; break;
	case D6: me->mode = D8; break;
	case D8: me->mode = D10; break;
	case D10: me->mode = D12; break;
	case D12: me->mode = D20; break;
	case D20: me->mode = D100; break;
	case D100: me->mode = D4; break;
	}
}


static void display_mode(struct Tapdie *me)
{
	char ch0 = '9', ch1 = '9';

	switch (me->mode) {
	case D4: ch0 = ' '; ch1 = '4'; break;
	case D6: ch0 = ' '; ch1 = '6'; break;
	case D8: ch0 = ' '; ch1 = '8'; break;
	case D10: ch0 = '1'; ch1 = '0'; break;
	case D12: ch0 = '1'; ch1 = '2'; break;
	case D20: ch0 = '2'; ch1 = '0'; break;
	case D100: ch0 = '0'; ch1 = '0'; break;
	}
	Q_ASSERT( nEventsFree((QActive*)&dashboard) >= 2 );
	QActive_post((QActive*)&dashboard, DASH_LCHAR_SIGNAL, ch0);
	QActive_post((QActive*)&dashboard, DASH_RCHAR_SIGNAL, ch1);
}


static QState aliveState(struct Tapdie *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		Q_ASSERT( nEventsFree((QActive*)(&dashboard)) >= 3 );
		QActive_post((QActive*)&dashboard,
			     DASH_BRIGHTNESS_SIGNAL, MAX_BRIGHTNESS);
		QActive_post((QActive*)&dashboard,
			     DASH_MIN_BRIGHTNESS_SIGNAL, MIN_BRIGHTNESS);
		QActive_post((QActive*)&dashboard,
			     DASH_MAX_BRIGHTNESS_SIGNAL, MAX_BRIGHTNESS);
		return Q_HANDLED();
	}
	return Q_SUPER(tapdieState);
}


static void show_number(uint8_t number, uint8_t leftdp, uint8_t rightdp)
{
	char ch0, ch1;

	/* We represent 100 with "00". */
	if (number == 100) {
		ch0 = '0';
		ch1 = '0';
	} else if (number < 10) {
		ch0 = ' ';
		ch1 = number + '0';
	} else {
		ch0 = number / 10 + '0';
		ch1 = number % 10 + '0';
	}
	if (leftdp) {
		ch0 |= 0x80;
	}
	if (rightdp) {
		ch1 |= 0x80;
	}
	Q_ASSERT( nEventsFree((QActive*)&dashboard) >= 2 );
	QActive_post((QActive*)&dashboard, DASH_LCHAR_SIGNAL, ch0);
	QActive_post((QActive*)&dashboard, DASH_RCHAR_SIGNAL, ch1);
}


/**
 * Get a random number and put that on the displays.
 *
 * We set the right display's decimal point to distinguish between 6 and 9, and
 * between 2 and 5.  We don't do anything with the display's flashing or fading
 * mode, or its brightness, leaving that up to the caller.
 */
static void generate_and_show_random(struct Tapdie *me, uint8_t realrandom)
{
	uint8_t rn;

	/* If we get the same number as the last random roll, get another one
	   so that the display will seem to change.  But if realrandom is set,
	   use the first one. */
	do {
		rn = (random() % me->mode) + 1;
	} while ((!realrandom) && (rn == me->randomnumber));
	if (realrandom) {
		show_number(rn, 0, 1);
	} else {
		show_number(rn, 0, 0);
	}
	me->randomnumber = rn;
}


static QState tappedState(struct Tapdie *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		seed_rng(BSP_get_random());
		Q_ASSERT( nEventsFree((QActive*)&dashboard) >= 2 );
		QActive_post((QActive*)&dashboard,
			     DASH_BRIGHTNESS_SIGNAL, MAX_BRIGHTNESS);
		QActive_post((QActive*)&dashboard, DASH_STEADY_SIGNAL, 0);
		display_mode(me);
		QActive_arm((QActive*)me, (2 * BSP_TICKS_PER_SECOND) / 3);
		return Q_HANDLED();
	case TAP_SIGNAL:
		rotate_mode(me);
		display_mode(me);
		/* Wait longer if we have been tapped in this state. */
		QActive_arm((QActive*)me, BSP_TICKS_PER_SECOND);
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		me->maxrolls = 10;
		return Q_TRAN(rollingState);
	}
	return Q_SUPER(aliveState);
}


static QState rollingState(struct Tapdie *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		Q_ASSERT( nEventsFree((QActive*)(&dashboard)) >= 2 );
		QActive_post((QActive*)&dashboard, DASH_STEADY_SIGNAL, ' ');
		QActive_post((QActive*)&dashboard,
			     DASH_BRIGHTNESS_SIGNAL, MAX_BRIGHTNESS);
		me->rolls = me->maxrolls;
		me->rollwait = 2; /* Must be at least to, see /2 below. */
		QActive_arm((QActive*)me, me->rollwait/2);
		generate_and_show_random(me, 0);
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		if (me->rolls) {
			/* Still rolling */
			generate_and_show_random(me, 0);
			me->rolls --;
			me->rollwait ++;
			QActive_arm((QActive*)me, me->rollwait/2);
			return Q_HANDLED();
		} else {
			return Q_TRAN(finalRollState);
		}
	case TAP_SIGNAL:
		if (me->maxrolls < 30) {
			me->maxrolls += 5;
		}
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
		generate_and_show_random(me, 1);
		post(&dashboard, DASH_BRIGHTNESS_SIGNAL, MAX_BRIGHTNESS);
		me->rolls = 1;
		QActive_arm((QActive*)me, BSP_TICKS_PER_SECOND);
		return Q_HANDLED();
	case TAP_SIGNAL:
		return Q_TRAN(tappedState);
	case Q_TIMEOUT_SIG:
		/* This is a little informal state machine.  It should be done
		   with a new state, but that would take up a fair bit of
		   program memory for little gain. */
		if (me->rolls) {
			display_mode(me);
			me->rolls = 0;
			QActive_arm((QActive*)me, BSP_TICKS_PER_SECOND / 2);
			return Q_HANDLED();
		} else {
			show_number(me->randomnumber, 0, 1);
			return Q_TRAN(finalRollFlashState);
		}
	}
	return Q_SUPER(aliveState);
}


static QState finalRollFlashState(struct Tapdie *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		QActive_arm((QActive*)me, 2 * BSP_TICKS_PER_SECOND);
		post(&dashboard, DASH_BRIGHTNESS_SIGNAL, MAX_BRIGHTNESS);
	case Q_TIMEOUT_SIG:
		return Q_TRAN(finalRollFadingState);
	}
	return Q_SUPER(finalRollState);
}


static QState finalRollFadingState(struct Tapdie *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		me->rolls = 7;
		QActive_arm((QActive*)me, BSP_TICKS_PER_SECOND);
		return Q_HANDLED();
	case DASH_AT_HIGH_SIGNAL:
		Q_ASSERT( nEventsFree((QActive*)&dashboard) >= 1 );
		QActive_post((QActive*)&dashboard, DASH_STEADY_SIGNAL, 0);
		QActive_arm((QActive*)me, BSP_TICKS_PER_SECOND);
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		post(&dashboard, DASH_START_FADING_SIGNAL, 0);
		me->rolls --;
		if (! me->rolls) {
			return Q_TRAN(finalRollEndState);
		} else {
			show_number(me->mode, 0, 0);
			return Q_HANDLED();
		}
	case DASH_AT_LOW_SIGNAL:
		show_number(me->randomnumber, 0, 1);
		return Q_HANDLED();
	}
	return Q_SUPER(finalRollFlashState);
}


static QState finalRollEndState(struct Tapdie *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		/* Set the bottom brightness to very low, then we wait until we
		   fade down to that. */
		post(&dashboard, DASH_MIN_BRIGHTNESS_SIGNAL, 1);
		return Q_HANDLED();
	case DASH_AT_LOW_SIGNAL:
		return Q_TRAN(deepSleepEntryState);
	}
	return Q_SUPER(finalRollFadingState);
}


/*
  FIXME stuff to think about.

  - The first tap wakes us up and we display the current mode.  What happens
  with the next tap?  If it's very close to the wakeup tap, do we use that as
  a mode change, or a roll?

  - Do we need waiting states all over the place to catch the very close
    together taps?

*/
