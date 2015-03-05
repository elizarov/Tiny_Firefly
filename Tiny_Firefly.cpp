/*
  Hardware
                   ATtiny13A
                 +-----------+
         RESET - | 1       8 | - VCC
           PB3 - | 2       7 | - PB2
  LED(+) - PB4 - | 3       6 | - PB1 - LED(-)
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

// variable sleep from 1 to 15 seconds
void wdSleepSecs(uint8_t secs) {
	if (secs >= 8) {
		wdSleep(WDTO_8S);
		secs -= 8;
	}
	if (secs >= 4) {
		wdSleep(WDTO_4S);
		secs -= 4;
	}
	if (secs >= 2) {
		wdSleep(WDTO_2S);
		secs -= 2;
	}
	if (secs >= 1) {
		wdSleep(WDTO_1S);
	}
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
	DDRB &= ~_BV(LED_MINUS_BIT);
	PORTB &= ~_BV(LED_MINUS_BIT);
	// wait discharge
	GIMSK |= _BV(INT0); // enable INT0 (default = when low)
	wdSleep(WDTO_250MS);
	bool result = (PINB & _BV(LED_MINUS_BIT)) != 0; // night if has not discharged yet
	GIMSK &= ~_BV(INT0); // disable INT0
	// back to output
	DDRB |= _BV(LED_MINUS_BIT);
	return result;
}

// XABC fast random generator (with a CAFEBABE seed)
uint8_t x = 0xCA;
uint8_t a = 0XFE;
uint8_t b = 0xBA;
uint8_t c = 0xBE;

// returns random number from 0 to 255
uint8_t random() {
	x++;                 //x is incremented every round and is not affected by any other variable
	a = (a^c^x);         //note the mix of addition and XOR
	b = (b+a);           //And the use of very few instructions
	c = (c+((b>>1)^a));  //the right shift is to ensure that high-order bits from b can affect
	return c;            //low order bits of other variables
}

// return random number from 0 to n-1
// 2 <= n <= 7
uint8_t rnd(uint8_t n) {
	uint8_t result;
	do {
		result = random();
		if (n <= 2) {
			result &= 1;
		} else if (n <= 4) {
			result &= 3;
		} else {
			result &= 7;
		}
	} while (result >= n);
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
	// two fast blinks on power up
	blink();
	wdSleep(WDTO_250MS);
	blink();
	// ----------------- loop -----------------
    while (true) {
main_loop:
		wdSleep(WDTO_8S);
		if (!night())
			continue;
		// *** night mode ***
		// 4 fast 1 sec blinks (and no night checks)
		for (uint8_t i = 0; i < 4; i++) {
			wdSleep(WDTO_1S);
			blink();
		}
		// 4 blinks with up to 50% of having 2 secs wait
		for (uint8_t i = 0; i < 4; i++) {
			wdSleep(WDTO_1S);
			if (rnd(8) <= i)
				wdSleep(WDTO_1S);
			if (!night())
				goto main_loop;
			blink();	
		}
		// now random from 1 to 2 secs and increase interval periodically
		uint8_t a = 1;
		uint8_t b = 2;
		uint8_t k = 0;
		do {
			// sleep in [a, b] secs interval
			wdSleepSecs(a + rnd(b - a + 1));
			if (!night())
				goto main_loop;
			blink();
			// increase a and b periodically
			//     k =   0     4     8     12    16    20    24    28    32    36    40
			// [a,b] = [1,2]-[1,3]-[2,4]-[2,5]-[3,6]-[3,7]-[4,8]-[5,8]-[6,8]-[7,8]-[8,8]
			k++;
			if ((k & 3) == 0 && b < 8)
				b++; 
			if ((k & 7) == 0)
				a++;
		} while (a < 8); // until we are at [8,8] sleep interval
		// just continue sleeping at 8 seconds without randomness for 1 hour = 60 mins = 3600 secs = 450 x 8 sec intervals
		for (uint16_t i = 0; i < 450; i++) {
			wdSleep(WDTO_8S);
			if (!night())
				goto main_loop;
			blink();	
		}
		// just sleep for the rest of the night
		do {
			wdSleep(WDTO_8S);
		} while (night());
    }
}
