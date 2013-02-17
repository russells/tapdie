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


const uint8_t tccr0a =
	(0b10 << COM0A0) |    /* Clear OC0A on compare match, set on BOTTOM. */
	(0b10 << COM0B0) |    /* Clear OC0B on compare match, set on BOTTOM. */
	(0b11 << WGM00);      /* Fast PWM. */

const uint8_t tccr0b =
	(0 << WGM02) |		/* Fast PWM. */
	(0b010 << CS00);	/* CLKio/8 */


void BSP_init(void)
{
	cli();
	PORTA = 0x00;		 /* Turn off all the LED outputs. */
	DDRA = 0xff;
	CB(PORTB, 0);
	SB(DDRB, 0);
	CB(PORTB, 1);
	SB(DDRB, 1);

	PCMSK0 = 0;
	CB(DDRB, 2);		/* Input on PB2, PCINT10. */
	SB(GIMSK, PCIE1);
	PCMSK1 = (1 << PCINT10); /* Pin change interrupt on PCINT10. */

	/* Timer 0 is used for PWM on the displays. */
	TCCR0A = tccr0a;
	TCCR0B = tccr0b;
	TIMSK0 =(1 << TOIE0);	 /* Overflow interrupt only. */
	start_watchdog();
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

	TCCR0B = 0;	   /* Stop the timer, CS[2:0]=000. */
	/* Ensure we handle any pending timer0 interrupt. */
	__asm__ __volatile__ ("nop" "\n\t" :: );

	cli();
	PRR = 0b00001111;	/* All peripherals off. */

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
	PRR = 0b00001011;	/* Timer 0 back on. */
	start_watchdog();
	TCCR0B = tccr0b;	/* Start the timer again. */

	DDRA = 0xff; /* FIXME this isn't quite right, see NOTES. */
	SB(DDRB, 0);
	SB(DDRB, 1);
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
}


SIGNAL(TIM0_OVF_vect)
{
	static uint8_t dnum;
	struct SevenSegmentDisplay *displayp;

	CB(PORTB, 0);
	CB(PORTB, 1);
	if (dnum) {
		displayp = displays;
		dnum = 0;
	} else {
		displayp = displays + 1;
		dnum = 1;
	}
	PORTA = displayp->segments;
	OCR0A = displayp->brightness;
	if (dnum) {
		SB(PORTB, 1);
	} else {
		SB(PORTB, 0);
	}
}
