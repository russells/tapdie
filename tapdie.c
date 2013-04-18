/**
 * @file
 *
 */

#include <stdlib.h>
#include "tapdie.h"
#include "bsp.h"
#include "displays.h"


/** The only active Tapdie. */
struct Tapdie tapdie;


Q_DEFINE_THIS_FILE;

static QState tapdieInitial        (struct Tapdie *me);
static QState tapdieState          (struct Tapdie *me);
static QState deepSleepState       (struct Tapdie *me);
static QState numbersState         (struct Tapdie *me);


static QEvent tapdieQueue[4];

QActiveCB const Q_ROM Q_ROM_VAR QF_active[] = {
	{ (QActive *)0              , (QEvent *)0      , 0                        },
	{ (QActive *)(&tapdie)      , tapdieQueue      , Q_DIM(tapdieQueue)       },
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

	QF_run();

	goto startmain;
}

void tapdie_ctor(void)
{
	QActive_ctor((QActive *)(&tapdie), (QStateHandler)&tapdieInitial);
	tapdie.counter = 0;
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


static QState deepSleepState(struct Tapdie *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		BSP_deep_sleep();
		return Q_HANDLED();
	case TAP_SIGNAL:
		return Q_TRAN(numbersState);
	}
	return Q_SUPER(tapdieState);
}


static QState numbersState(struct Tapdie *me)
{
	switch (Q_SIG(me)) {
	case Q_ENTRY_SIG:
		me->digit = '0';
		me->counter = 0;
		post(me, NEXT_DIGIT_SIGNAL);
		return Q_HANDLED();
	case NEXT_DIGIT_SIGNAL:
		if (me->counter >= 2)
			return Q_TRAN(deepSleepState);
		me->randomnumber = random() % 100;
		uint8_t x = me->randomnumber % 100;
		me->digits[1] = x % 10 + '0';
		me->digits[0] = x / 10 + '0';
		if (me->counter & 0b1) {
			set_digits(me->digits[1] | 0x80, 127, me->digits[0] | 0x80, 127);
		} else {
			set_digits(me->digits[1], 127, me->digits[0], 127);
		}
		QActive_arm((QActive*)me, 9);
		me->digit ++;
		if (me->digit > '9') {
			me->digit = '0';
			me->counter ++;
		}
		return Q_HANDLED();
	case Q_TIMEOUT_SIG:
		post(me, NEXT_DIGIT_SIGNAL);
		return Q_HANDLED();
	case Q_EXIT_SIG:
		set_digits(' ', 127, ' ', 127);
		return Q_HANDLED();
	}
	return Q_SUPER(tapdieState);
}
