#include "bsp.h"
#include "tapdie.h"
#include "morse.h"
#include "displays.h"
#include <avr/wdt.h>


Q_DEFINE_THIS_FILE;


#define SB(reg,bit) ((reg) |= (1 << (bit)))
#define CB(reg,bit) ((reg) &= ~(1 << (bit)))


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


static void start_watchdog(void)
{
	wdt_reset();
	wdt_enable(WDTO_30MS);
	SB(WDTCSR, WDIE);
}


void BSP_watchdog(void)
{
	start_watchdog();
}


SIGNAL(WDT_vect)
{
	postISR(&tapdie, WATCHDOG_SIGNAL);
	QF_tick();
}


void BSP_startmain(void)
{

}


const uint8_t tccr0a_init =
	(0b10 << COM0A0) |    /* Clear OC0A on compare match, set on BOTTOM. */
	(0b10 << COM0B0) |    /* Clear OC0B on compare match, set on BOTTOM. */
	(0b11 << WGM00);      /* Fast PWM. */

const uint8_t tccr0b_init =
	(0 << WGM02) |		/* Fast PWM. */
	(0b010 << CS00);	/* CLKio/8 */

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
	/* TODO: when there is a timer available, use that to ensure we don't
	   send too many TAP_SIGNALs one after the other. */
	post(&tapdie, TAP_SIGNAL);
}


#define MORSE_DDR DDRB
#define MORSE_PORT PORTB
#define MORSE_BIT 0


void BSP_enable_morse_line(void)
{
	SB(MORSE_DDR, MORSE_BIT);
	CB(MORSE_PORT, MORSE_BIT);
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
	TCCR0B = 0;		/* Stop timer 0 */
	TCCR1B = 0;		/* Stop timer 1 */
	PRR = 0b00001111;
}


SIGNAL(TIM0_OVF_vect)
{
	static uint8_t dnum;
	struct SevenSegmentDisplay *displayp;
	uint8_t segments;

	if (0 == dnum) {
		displayp = displays;
	} else {
		displayp = displays + 1;
	}
	segments = displayp->segments;
	PORTA = segments & 0x7f;
	if (segments & 0x80) {
		SB(PORTB, 1);
	} else {
		CB(PORTB, 1);
	}
	if (0 == dnum) {
		OCR0A = displayp->brightness;
		CB(DDRA, 7);	/* Clear before set so we don't run both. */
		SB(DDRB, 2);
	} else {
		OCR0B = displayp->brightness;
		CB(DDRB, 2);
		SB(DDRA, 7);
	}

	dnum = ! dnum;		/* Other display next time. */
}
