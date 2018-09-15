#include "Arduino.h"
#include <util/atomic.h>

#define PIN_LED 13
void toggleLED() {
	static bool state = false;
	state = !state;
	digitalWrite(PIN_LED, state);
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

static void tick () {
	rpm.tick();
}

#define PIN_PWM 3
#define PIN_RELAY 4
void pwm() {
	static unsigned long last = 0;
	static byte relay = 0;

	byte pwm;
	int value = analogRead(A5);
	if(value < 220)
		pwm	= 0;
	else if(value > 680)
		pwm = 255;
	else
		pwm = map(value, 220, 680, 0, 255);

	byte relay_new = value > 200;
	if (relay_new != relay) {
		// avoid switching the relay too fast
		unsigned long now = millis();
		if (now - last > 1000) {
			last = now;
			relay = relay_new;
			digitalWrite(PIN_RELAY, !relay);
		}
	}
	analogWrite(PIN_PWM, pwm);
}


void setup() {
	Serial.begin(115200);
	pinMode(rpm.pin(), INPUT);
	pinMode(A0,INPUT);
	pinMode(A1,INPUT);
	pinMode(A2,INPUT);
	pinMode(A3,INPUT);
	pinMode(A4,INPUT);
	pinMode(PIN_LED, OUTPUT);

	attachInterrupt(0, tick, CHANGE);

	pinMode(A5,INPUT);
	pinMode(PIN_PWM, OUTPUT);
	pinMode(PIN_RELAY, OUTPUT);
	digitalWrite(PIN_RELAY, 1);
}

void loop() {
	static unsigned long last = 0;

	pwm();

	unsigned long now = millis();
	if (now - last > 250) {
		last = now;
		Serial.print(analogRead(A0));
		Serial.print(",");
		Serial.print(analogRead(A1));
		Serial.print(",");
		Serial.print(analogRead(A2));
		Serial.print(",");
		Serial.print(analogRead(A3));
		Serial.print(",");
		Serial.print(analogRead(A4));
		Serial.print(",");
		Serial.println(RPM::rpm(rpm.period()));
	}
}
