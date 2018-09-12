#include "Arduino.h"
#include <util/atomic.h>

void toggleLED() {
	static bool state = false;
	state = !state;
	digitalWrite(13, state);
}

#define ZERO_TIME 1000 // after 1s of no ticks consider no motion
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

private:
	uint8_t pin_;
	volatile bool value_ = false;
	volatile unsigned long latest_ = 0;  // time of most recently observed falling flank
	volatile unsigned long previous_ = 0;  // time of previous falling flank indicating cycle start
	volatile unsigned long period_ = 0;
};

unsigned long RPM::period() const {
	uint8_t oldSREG = SREG;
	cli();
	unsigned long period = period_;
	unsigned long latest = latest_;
	SREG = oldSREG;
	unsigned long estimate = millis() - latest; // estimate period from time since latest flank
	return estimate > ZERO_TIME ? 0 : period; // report no motion
//	return estimate > period ? estimate : period; // return the larger period of the two
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
#if 0 // doesn't help for smoothing after restart?
		if (now - latest_ > 2*ZERO_TIME) // reset timestamps to now after long time
			latest_ = now;
#endif
		previous_ = latest_;
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
		last = now;
		Serial.println(RPM::rpm(rpm.period()));
	}
#endif
}
