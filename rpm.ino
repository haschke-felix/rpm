#include "Arduino.h"
#include <util/atomic.h>

void toggleLED() {
	static bool state = false;
	state = !state;
	digitalWrite(13, state);
}

class RPM {
public:
	RPM(uint8_t pin) : pin_(pin)
	{
	}

	inline uint8_t pin() const { return pin_; }
	static unsigned long rps(unsigned long period_ms) {
		return period_ms > 0 ? 1000ul / period_ms : 0;
	}
	static unsigned long rpm(unsigned long period_ms) {
		return period_ms > 0 ? 60000ul / period_ms : 0;
	}
	unsigned long period() const;
	void tick();

//private:
	uint8_t pin_;
	volatile bool value_ = false;
	volatile unsigned long previous_ = 0;  // time of previous valid flank
	volatile unsigned long latest_ = 0;  // time of latest observed flank
	volatile unsigned long period_ = 0;

	volatile unsigned int jitter_ = 0;
};

unsigned long RPM::period() const {
#if 1
	unsigned long p;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		p = period_;
	}
	return p;
#else
	unsigned long period = micros(); // compute period since t1_ with next command
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		unsigned long p  = period_;
		period -= previous_;
		if (period > period_)  // increase period if there was no update since t1_
			period_ = period;
		else  // otherwise return period_
			period = period_;
	}
	return period;
#endif
}

//              |      period      |
// ______|||~~~~|____________||||~~|__________|..
//              prev               latest     now
void RPM::tick() {
	// did we really had a change on our pin?
	const bool curval = digitalRead(pin_);
	if (curval == value_)
		return;
	value_ = curval;

	const unsigned long now = millis();
	unsigned long diff = now - latest_;
	if (curval == LOW) // for latest, consider falling flanks only
		latest_ = now;
	if (diff < 1)  // consider as noise if changing too fast
		return;

	if (curval == HIGH) {  // on first rising flank after a while:
		diff = latest_ - previous_;
		if (diff < 100) return;  // Skip spurious double peaks
		period_ = latest_ - previous_;
		previous_ = latest_;
		toggleLED();
	}
}


RPM rpm(2);
unsigned long last;

static void tick () {
	rpm.tick();
}

void setup() {
	Serial.begin(115200);
	pinMode(rpm.pin(), INPUT);
	pinMode(13, OUTPUT);
	attachInterrupt(0, tick, CHANGE);
	last = millis();
}

void loop() {
#if 1
	unsigned long now = millis();
	if (now - last > 100) {
		uint8_t oldSREG = SREG;
		cli();
		unsigned long prev = rpm.previous_;
		unsigned long latest = rpm.latest_;
		unsigned long period = rpm.period_;
		SREG = oldSREG;

		Serial.print(prev);
		Serial.print(", ");
		Serial.print(latest);
		Serial.print(", ");
		Serial.print(latest-prev);
		Serial.print(", ");
		Serial.print(period);
		Serial.print(", ");
		Serial.print(RPM::rps(period));
		Serial.print(", ");
		Serial.print(RPM::rpm(period));
		Serial.println("");
		last = now;
	}
#endif
}
