
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <util/delay.h>

const uint8_t LED_MINUS_PIN = 1; // This is INT0 pin
const uint8_t LED_PLUS_PIN = 4;

EMPTY_INTERRUPT(WDT_vect);
EMPTY_INTERRUPT(INT0_vect);

void wdSleep(uint8_t wdto) {
	wdt_reset();
	WDTCR = _BV(WDCE); // enable change
	WDTCR = _BV(WDTIF) | _BV(WDTIE) | wdto;
	sei();
	sleep_cpu();
	cli();
}

int main() {
	MCUSR = _BV(SE) | _BV(SM1); // enable sleep mode -- Power Down; low level INT0
	DDRB = _BV(LED_MINUS_PIN) | _BV(LED_PLUS_PIN); // both led pins are output
    while(1) {
		PORTB |= _BV(LED_PLUS_PIN);
		wdSleep(WDTO_15MS);
		PORTB &= ~_BV(LED_PLUS_PIN);
        wdSleep(WDTO_1S);
    }
}