#include "bsp.h"
#include "tapdie.h"
#include "morse.h"
#include "displays.h"
#include <avr/wdt.h>
#include "common-diode.h"

Q_DEFINE_THIS_FILE;


#define SB(reg,bit) ((reg) |= (1 << (bit)))
#define CB(reg,bit) ((reg) &= ~(1 << (bit)))


/**
 * We use this to count timer interrupts (effectively QF_tick() calls), and
 * then use that count to limit the rate at which we send tap signals.
 */
static uint8_t time_counter = 0;

#define TIME_COUNTER_MAX 200

/**
 * Incremented by our timer interrupt.  We can use this to return a random
 * uint8_t to the main loop, since the timer will run faster than a person can
 * comprehend.
 */
static volatile uint8_t free_running_interrupt_counter;

/**
 * Get a random number for the pseudo RNG.  We return the free-running
 * interrupt counter incremented by the timer interrupt.
 */
uint8_t BSP_get_random(void)
{
	return free_running_interrupt_counter;
}


void QF_onStartup(void)
{

}

void QF_onIdle(void)
{
	/* Minimum action: re-enable interrupts. */
	sei();
}

void Q_onAssert(char const Q_ROM * const Q_ROM_VAR file, int line)
{
	morse_assert(file, line);
}


/**
 * Reset the watchdog timer very early in the startup sequence.  If we try to
 * do this from the beginning of main(), it often takes too long to get there
 * due to the data segment initialisation code, and we end up in a reset loop.
 */
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));
void wdt_init(void)
{
	wdt_disable();
}


static void start_watchdog(void)
{
	wdt_reset();
	wdt_enable(WDTO_60MS);
	SB(WDTCSR, WDIE);
}


void BSP_watchdog(void)
{
	start_watchdog();
}


SIGNAL(WDT_vect)
{
	postISR(&tapdie, WATCHDOG_SIGNAL, 0);
	/* QF_tick() is now called from the timer interrupt. */
}


#ifdef COMMON_CATHODE
#define DPSTART() do { SB(DDRB, 1); SB(DDRB, 2); } while (0)
#define DPON()    do { SB(PORTB, 1); CB(PORTB, 2); } while (0)
#define DPOFF()   do { CB(PORTB, 1); } while (0)
#define DPSTOP()  do { DDRB = 0; PORTB = 0; } while (0)

#else

#ifdef COMMON_ANODE
#define DPSTART() do { SB(DDRB, 1); SB(DDRB, 2); } while (0)
#define DPON()    do { CB(PORTB, 1); SB(PORTB, 2); } while (0)
#define DPOFF()   do { SB(PORTB, 1); } while (0)
#define DPSTOP()  do { DDRB = 0; PORTB = 0; } while (0)
#endif
#endif

#define DP(ton,toff)		 \
	do {			 \
		DPON();		 \
		_delay_ms(ton);	 \
		DPOFF();	 \
		_delay_ms(toff); \
	} while (0)

void BSP_startmain(void)
{
	uint8_t prr;
	uint8_t sreg;
	uint8_t mcusr;

	time_counter = 0;

	wdt_disable();
	sreg = SREG;
	cli();
	prr = PRR;
	PRR = 0xf;

	/* Flash three times to say we're alive. */
	DPSTART();
	DP(100,200);
	DP(100,200);
	DP(100,800);

	/* Show the reset reason.  The bits of MCUSR are shifted out to the
	   left, so the order is PORF, EXTRF, BORF, WDRF.  Two flashes means
	   set, one flash means not set. */
	mcusr = MCUSR;
	MCUSR = 0;
	for (uint8_t i=0; i<4; i++) {
		if (mcusr & 0x1) {
			DP(100,150);
			DP(100,650);
		} else {
			DP(100,900);
		}
		mcusr >>= 1;
	}

	PRR = prr;
	SREG = sreg;
}


#ifdef COMMON_ANODE
#define COM0xn 0b10	      /* Clear OC0A on compare match, set on BOTTOM. */
#else
#ifdef COMMON_CATHODE
#define COM0xn 0b11	      /* Set OC0B on compare match, clear on BOTTOM. */
#endif
#endif


const uint8_t tccr0a_init =
	(COM0xn << COM0A0) |
	(COM0xn << COM0B0) |
	(0b11 << WGM00);	/* Fast PWM. */

const uint8_t tccr0b_init =
	(0 << WGM02) |		/* Fast PWM. */
	(0b001 << CS00);	/* CLKio/1 */

const uint8_t ddra_init  = 0xff;
const uint8_t porta_init = 0x00;
const uint8_t ddrb_init  = 0b110; /* PB0 is always input. */
const uint8_t portb_init = 0x00;


void BSP_init(void)
{
	cli();

	PCMSK0 = 0;
	CB(DDRB, 0);		/* Input on PB0, PCINT8. */
	SB(GIMSK, PCIE1);
	PCMSK1 = (1 << PCINT8); /* Pin change interrupt on PCINT8. */

	/* Timer 0 is used for PWM on the displays. */
	TCCR0A = tccr0a_init;
	TCCR0B = tccr0b_init;
	TIMSK0 =(1 << TOIE0);	 /* Overflow interrupt only. */
	start_watchdog();

	PORTA = porta_init;	/* Turn off all the LED outputs. */
	DDRA  = ddra_init;
	PORTB = portb_init;
	DDRB  = ddrb_init;

	sei();
}


/**
 * Put the MCU to sleep, using as little power as possible.
 *
 * Because we turn off all the timers, QP is not aware of the sleep, and when
 * we wake up continues as though nothing different happened.
 */
void BSP_deep_sleep(void)
{
	wdt_reset();
	wdt_disable();

	PRR = 0b00001111;	/* All peripherals off. */

	/* Ensure we handle any pending timer0 interrupt. */
	__asm__ __volatile__ ("nop" "\n\t" :: );
	cli();

	DDRA = 0b00000000;	/* All lines are inputs. */
	DDRB = 0b00000000;

	SB(MCUCR, SM1);		/* Power down sleep mode. */
	CB(MCUCR, SM0);
	SB(MCUCR, SE);

	Q_ASSERT( time_counter <= TIME_COUNTER_MAX );
	/* Prepare the counter for the next wake up. */
	time_counter = 0;

	/* Don't separate the following two assembly instructions.  See Atmel's
	   NOTE03. */
	__asm__ __volatile__ ("sei" "\n\t" :: );
	__asm__ __volatile__ ("sleep" "\n\t" :: );

	/* Now we're awake again. */
	CB(MCUCR, SE);          /* Disable sleep mode. */
	CB(PRR, 2);		/* Timer 0 back on. */
	TCNT0 = 0;		/* Start counting from the beginning again. */
	start_watchdog();

	DDRA = ddra_init;
	DDRB = ddrb_init;
}


SIGNAL(PCINT1_vect)
{
	Q_ASSERT( time_counter <= TIME_COUNTER_MAX );

	/* If the event queue already has more than one event being processed
	   and one waiting, don't put any more in. */
	if (nEventsUsed((QActive*)(&tapdie)) > 2)
		return;

	/* Only send a tap signal if we haven't sent one recently, or if we're
	   just waking up now. */
	if (0 == time_counter || time_counter > BSP_TICKS_PER_SECOND / 5) {
		postISR(&tapdie, TAP_SIGNAL, time_counter);
		/* We need to set time_counter to non-zero here, or in the case
		   where we've just woken up, it may be zero again at the next
		   interrupt.  These interrupts can happen in quick succession
		   with a noisy input. */
		time_counter = 1;
		return;
	}
}


#ifdef COMMON_ANODE
#define MORSE_PORT PORTA
#define MORSE_BIT 7
#else
#ifdef COMMON_CATHODE
#define MORSE_PORT PORTB
#define MORSE_BIT 1
#endif
#endif


void BSP_enable_morse_line(void)
{
	/* Enable both lines, set them both to 0.  We toggle the right line
	   later depending on the display's common pin. */
	CB(PORTA, 7);
	SB(DDRA, 7);
	CB(PORTB, 1);
	SB(DDRB, 1);
}


void BSP_morse_signal(uint8_t onoff)
{
	if (onoff)
		SB(MORSE_PORT, MORSE_BIT);
	else
		CB(MORSE_PORT, MORSE_BIT);
}


void BSP_stop_everything(void)
{
	cli();
	wdt_reset();
	wdt_disable();
	TCCR0A = 0;		/* Stop timer 0 */
	TCCR0B = 0;
	TCCR1A = 0;		/* Stop timer 1 */
	TCCR1B = 0;
	PRR = 0b00001111;
	DDRA = 0;
	DDRB = 0;
}


/**
 * Force a chip reset.  We do this by enabling the watchdog, then disabling
 * interrupts and waiting.  But first, there's a long flash on one decimal
 * point segment.
 */
void BSP_do_reset(void)
{
	cli();
	wdt_disable();

	/* */
	DPSTART();
	_delay_ms(250);
	DPON();
	_delay_ms(1000);
	DPOFF();
	_delay_ms(500);
	DPSTOP();

	wdt_enable(WDTO_15MS);
	while (1)
		;
}


/**
 * @todo: Change this to display only one segment at a time.  Currently we
 * display all the segments of a digit at once.  This will result in varying
 * brightness of different digits because we only have one series resistor (op
 * the common pin) and because we have a very limited current supply (one
 * CR2032 battery.)
 */
SIGNAL(TIM0_OVF_vect)
{
	static uint8_t interrupt_counter = 0;
	static uint8_t qf_tick_counter = 0;
	static uint8_t dnum;
	static uint8_t segmentmask = 0b1;
	struct SevenSegmentDisplay *displayp;
	uint8_t segments;

	free_running_interrupt_counter ++;

	/* Only count timer or call QF_tick every so often. */
	if (!qf_tick_counter) {
		Q_ASSERT( time_counter <= TIME_COUNTER_MAX );
		if (time_counter < TIME_COUNTER_MAX) {
			time_counter ++;
		}
		QF_tick();
		qf_tick_counter = 65; /* 3906/65 ~= 60 */
	} else {
		qf_tick_counter --;
	}

	/* This makes us exit the ISR early 3 of 4 times.  CLKio is 1e6, so the
	   interrupt rate is 1e6/256==3906Hz.  Running 1 of 4 of the interrupt
	   routines gives us an effective rate of about 976Hz.  We have 16
	   segments on two displays, so the flashing rate on each is
	   976/16==61Hz.  Each segment will actually flash four times each time
	   it is displayed, before we move on to the next segment. */
	if (interrupt_counter) {
		interrupt_counter >>= 1;
		return;
	} else {
		interrupt_counter = 0b100;
	}

	CB(DDRA, 7);		/* Clear before set so we don't run both. */
	CB(DDRB, 2);

	segmentmask <<= 1;
	if (0 == segmentmask) {
		dnum = !dnum;
		segmentmask = 0b1;
	}

	if (0 == dnum) {
		displayp = displays;
	} else {
		displayp = displays + 1;
	}
	segments = displayp->segments;
	/* We can ignore bit 7 (MSB) of PORTA, because it is used as timer 0's
	   OC0A output bit. */
#ifdef COMMON_CATHODE
	PORTA = segments & segmentmask;
#else
#ifdef COMMON_ANODE
	PORTA = segments | (~segmentmask);
#endif
#endif

#ifdef COMMON_CATHODE
	if (segments & 0x80) {
		SB(PORTB, 1);
	} else {
		CB(PORTB, 1);
	}
#else
#ifdef COMMON_ANODE
	if (segments & 0x80) {
		CB(PORTB, 1);
	} else {
		SB(PORTB, 1);
	}
#else
#error Must define COMMON_ANODE or COMMON_CATHODE
#endif
#endif

	if (0 == dnum) {
		OCR0A = displayp->brightness;
		SB(DDRB, 2);
	} else {
		OCR0B = displayp->brightness;
		SB(DDRA, 7);
	}
}
