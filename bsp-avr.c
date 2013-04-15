#include "bsp.h"
#include "tapdie.h"
#include "morse.h"
#include <avr/wdt.h>
#include "common-diode.h"

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


void BSP_watchdog(void)
{

}


void BSP_startmain(void)
{

}


void BSP_init(void)
{
	/* TODO: set up at least one timer, and perhaps the watchdog. */
	PCMSK1 = (1 << PCINT10); /* Pin change interrupt on PCINT10. */
	PCMSK0 = 0;
	CB(DDRB, 2);		/* Input on PB2, PCINT10. */
	SB(GIMSK, PCIE0);
}


/**
 * Put the MCU to sleep, using as little power as possible.
 *
 * Because we turn off all the timers, QP is not aware of the sleep, and when
 * we wake up continues as though nothing different happened.
 */
void BSP_deep_sleep(void)
{
	/* TODO: make all the LED control lines into inputs. */
	/* TODO: when a timer is running, turn it off before cli(), then do a
	   couple of NOP instructions to ensure we can process any possible
	   pending timer interrupts before sleeping to ensure they don't wake
	   us early. */
	cli();
	PRR = 0b00001111;	/* All peripherals off. */

	SB(MCUCR, SM1);		/* Power down sleep mode. */
	CB(MCUCR, SM0);
	SB(MCUCR, SE);

	/* Don't separate the following two assembly instructions.  See Atmel's
	   NOTE03. */
	__asm__ __volatile__ ("sei" "\n\t" :: );
	__asm__ __volatile__ ("sleep" "\n\t" :: );

	/* Now we're awake again. */
	CB(MCUCR, SE);          /* Disable sleep mode. */
}


SIGNAL(PCINT1_vect)
{
	/* TODO: when there is a timer available, use that to ensure we don't
	   send too many TAP_SIGNALs one after the other. */
	post(&tapdie, TAP_SIGNAL);
}


#define MORSE_DDR DDRB
#define MORSE_PORT PORTB
#define MORSE_BIT 2


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
