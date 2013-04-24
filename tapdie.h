#ifndef tapdie_h_INCLUDED
#define tapdie_h_INCLUDED

#include "qpn_port.h"


enum TapdieSignals {
	/**
	 * Sent for timing, and so we can confirm that the event loop is
	 * running.
	 */
	WATCHDOG_SIGNAL = Q_USER_SIG,
	TAP_SIGNAL,		/** Comes with a counter that tells us how long
				    since the last tap. */
	DASH_OFF_SIGNAL,
	DASH_ON_SIGNAL,
	DASH_LCHAR_SIGNAL,
	DASH_RCHAR_SIGNAL,
	DASH_BRIGHTNESS_SIGNAL,
	DASH_MAX_BRIGHTNESS_SIGNAL,
	DASH_MIN_BRIGHTNESS_SIGNAL,
	DASH_START_FADING_SIGNAL,
	DASH_START_FLASHING_SIGNAL,
	DASH_STEADY_SIGNAL,
	DASH_AT_TOP_SIGNAL,	/** Sent by the dashboard when it is flashing
				    or fading and has just got to the top
				    brightness. */
	DASH_AT_BOTTOM_SIGNAL,	/** Sent by the dashboard when it is flashing
				    or fading and has just got to the bottom
				    brightness. */
	MAX_PUB_SIG,
	MAX_SIG,
};


/**
 * List of die modes.  We use the values in this enum as integers representing
 * the maximum roll when we generate a random roll, so the enum values are
 * important.
 */
enum TapdieMode {
	D4 = 4,
	D6 = 6,
	D8 = 8,
	D10 = 10,
	D12 = 12,
	D20 = 20,
	D100 = 100,
};


/**
 * Create the tapdie.
 */
void tapdie_ctor(void);


/**
 */
struct Tapdie {
	QActive super;
	uint8_t randomnumber;
	enum TapdieMode mode;
	uint8_t rolls;
	uint8_t rollwait;
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
#define post(o, sig, par)						\
	do {								\
		QActive *_me = (QActive *)(o);				\
		QActiveCB const Q_ROM *ao = &QF_active[_me->prio];	\
		Q_ASSERT(_me->nUsed < Q_ROM_BYTE(ao->end));		\
		QActive_post(_me, sig, par);				\
	} while (0)

/**
 * Call this instead of calling QActive_postISR().
 *
 * @see post()
 */
#define postISR(o, sig, par)						\
	do {								\
		QActive *_me = (QActive *)(o);				\
		QActiveCB const Q_ROM *ao = &QF_active[_me->prio];	\
		Q_ASSERT(_me->nUsed < Q_ROM_BYTE(ao->end));		\
		QActive_postISR(_me, sig, par);				\
	} while (0)

/**
 * Find out how many events are in the queue.  According to the QP-nano source,
 * this count potentially includes one currently being processed by the state
 * machine.
 */
static inline uint8_t nEventsUsed(QActive *o)
{
	return o->nUsed;
}

/**
 * Find out how many more events can fit in the queue.
 */
static inline uint8_t nEventsFree(QActive *o)
{
	QActiveCB const Q_ROM *ao = &QF_active[o->prio];
	return Q_ROM_BYTE(ao->end) - o->nUsed;
}

#endif
