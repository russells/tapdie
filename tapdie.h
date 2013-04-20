#ifndef tapdie_h_INCLUDED
#define tapdie_h_INCLUDED

#include "qpn_port.h"


enum TapdieSignals {
	/**
	 * Sent for timing, and so we can confirm that the event loop is
	 * running.
	 */
	WATCHDOG_SIGNAL = Q_USER_SIG,
	TAP_SIGNAL,
	NEXT_DIGIT_SIGNAL,
	DASH_OFF_SIGNAL,
	DASH_ON_SIGNAL,
	MAX_PUB_SIG,
	MAX_SIG,
};


/**
 * Create the tapdie.
 */
void tapdie_ctor(void);


/**
 */
struct Tapdie {
	QActive super;
	char digits[2];
	uint8_t randomnumber;
	uint8_t counter;
};


extern struct Tapdie tapdie;


/**
 * Call this instead of calling QActive_post().
 *
 * It checks that there is room in the event queue of the receiving state
 * machine.  QP-nano does this check itself anyway, but the assertion from
 * QP-nano will always appear at the same line in the same file, so we won't
 * know which state machine's queue is full.  If this check is done in user
 * code instead of library code we can tell them apart.
 */
#define postpar(o, sig, par)						\
	do {								\
		QActive *_me = (QActive *)(o);				\
		QActiveCB const Q_ROM *ao = &QF_active[_me->prio];	\
		Q_ASSERT(_me->nUsed < Q_ROM_BYTE(ao->end));		\
		QActive_post(_me, sig, par);				\
	} while (0)
#define post(o, sig) postpar(o, sig, 0);

/**
 * Call this instead of calling QActive_postISR().
 *
 * @see post()
 */
#define postISRpar(o, sig, par)						\
	do {								\
		QActive *_me = (QActive *)(o);				\
		QActiveCB const Q_ROM *ao = &QF_active[_me->prio];	\
		Q_ASSERT(_me->nUsed < Q_ROM_BYTE(ao->end));		\
		QActive_postISR(_me, sig, par);				\
	} while (0)
#define postISR(o, sig) postISRpar(o, sig, 0);

/**
 * Find out how many events are in the queue.  According to the QP-nano source,
 * this count potentially includes one currently being processed by the state
 * machine.
 */
static inline uint8_t nEventsUsed(QActive *o)
{
	return o->nUsed;
}

#endif
