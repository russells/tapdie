/**
 * @file
 *
 */

#include "tapdie.h"
#include "bsp.h"
#include "displays.h"


/** The only active Tapdie. */
struct Tapdie tapdie;


Q_DEFINE_THIS_FILE;

static QState tapdieInitial        (struct Tapdie *me);
static QState tapdieState          (struct Tapdie *me);
static QState deepSleepState       (struct Tapdie *me);


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
	}
	return Q_SUPER(tapdieState);
}
