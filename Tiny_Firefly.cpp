/*
  Hardware
                   ATtiny13A
                 +-----------+
         RESET - | 1       8 | - VCC
           PB3 - | 2       7 | - PB2
  LED(-) - PB4 - | 3       6 | - PB1 - LED(+)
           GND - | 4       5 | - PB0
                 +-----------+

  Configure low fuse to: 
  
   bit 7: SPIEN  - 0
   bit 6: EESAVE - 1
   bit 5: WDTON  - 1
   bit 4: CKDIV8 - 1
   bit 3: SUT1   - 1 \ slow startup (+64ms)
   bit 2: SUT0   - 0 /
   bit 1: CKSEL1 - 1 \ internal 128KHz 
   bit 0: CKSEL0 - 1 /
   ------------------
                0x7B

   Estimated power consumption on 3V battery per datasheet:
   ~ 42 uA in active mode 128 KHz oscillator (not taking the LED power into account)
   ~  4 uA in power down mode with WDT enabled

*/

#define F_CPU 128000L

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <util/delay.h>

const uint8_t LED_MINUS_BIT = 1; // This is INT0 pin
const uint8_t LED_PLUS_BIT = 4;

EMPTY_INTERRUPT(WDT_vect);
EMPTY_INTERRUPT(INT0_vect);

void wdSleepImpl(uint8_t wdtcr) {
	WDTCR |= _BV(WDCE); // enable the WDT Change Bit
	WDTCR = wdtcr; 
	wdt_reset(); // start counting with new timeout setting
	WDTCR |= _BV(WDTIF); // now reset interrupt flag [again] after all config / timer reset done
	sei();
	sleep_cpu();
	cli();
}

inline __attribute__((always_inline)) void wdSleep(uint8_t wdto) {
	// statically compute WDTCR value to enable WDT Interrupt and set timeout
	// note: bit 3 is separate
	wdSleepImpl(_BV(WDTIF) | _BV(WDTIE) | (wdto & 7) | (wdto >> 3 << WDP3));
}

void blink() {
	PORTB |= _BV(LED_PLUS_BIT);
	//wdSleep(WDTO_15MS);
	_delay_ms(1);
	PORTB &= ~_BV(LED_PLUS_BIT);
}

bool night() {
	// charge
	PORTB |= _BV(LED_MINUS_BIT);
	wdSleep(WDTO_15MS);
//	MCUCR |= _BV(PUD); // disable pull ups
	DDRB &= ~_BV(LED_MINUS_BIT);
	PORTB &= ~_BV(LED_MINUS_BIT);
//	MCUCR &= ~_BV(PUD); // enable pull ups
	// wait discharge
	GIMSK |= _BV(INT0); // enable INT0 (default = when low)
	wdSleep(WDTO_250MS);
	bool result = (PINB & _BV(LED_MINUS_BIT)) != 0; // night if has not discharged yet
	GIMSK &= ~_BV(INT0); // disable INT0
	// back to output
	DDRB |= _BV(LED_MINUS_BIT);
	return result;
}

int main() {
	// ----------------- setup -----------------
	PRR = _BV(PRTIM0) | _BV(PRADC); // turn off Timer0 & ADC
	ACSR = _BV(ACD); // turn off Analog Comparator
	DDRB = _BV(LED_MINUS_BIT) | _BV(LED_PLUS_BIT); // both LED pins are output
	PORTB = 0xff & ~(_BV(LED_MINUS_BIT) | _BV(LED_PLUS_BIT)); // pull up all other pins to ensure defined level and save power
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_enable();
	blink();
	// ----------------- loop -----------------
    while (true) {
		if (!night()) {
			wdSleep(WDTO_8S);
			continue;
		}
		wdSleep(WDTO_2S);
		blink();
    }
}
