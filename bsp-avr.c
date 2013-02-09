#include "bsp.h"
#include "tapdie.h"
#include "morse.h"
#include <avr/wdt.h>


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
