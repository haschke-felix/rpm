#include "Arduino.h"

class RPM {
public:
	RPM(uint8_t pin) :
	   pin_(pin), armed_(true), jitter_(0), t1_(0), period_(0)
	{
	}

	void process();
	inline uint8_t pin() const { return pin_; }
	inline unsigned long value() const;
	inline unsigned long period() const { return period_; }

private:
	uint8_t pin_;
	bool armed_; // expecting next rising flank?
	byte jitter_;
	static const byte THRESHOLD = 1;  // adapt with speed? better: interrupt-based
	unsigned long t1_;
	unsigned long period_;
};

void RPM::process() {
	byte value = digitalRead(pin_);
	Serial.print(0);
	if (armed_) {
		if (value) ++jitter_;
		else jitter_ = 0;

		if (jitter_ > THRESHOLD) {  // count a tick after several positive values in a row
			armed_ = false;
			jitter_ = 0;
			unsigned long t2 = micros();
			period_ = (t2 - t1_);
			t1_ = t2;
			Serial.print(1);
		}
	} else if (!value) {
		armed_ = true;
	}
	if (period_ != 0) {
		unsigned long period = micros() - t1_;
		if (period > period_)  // increase period if there was no new tick
			period_ = period;
	}
#if 1
//	Serial.print(armed_);
//	Serial.print(",");
	Serial.print(period_ / 100000);
	Serial.print(".");
	Serial.print(period_ % 100000);
	Serial.println("");
#endif
}

unsigned long RPM::value() const {
	return period_ > 0 ? 60000ul / period_ : 0;
}

RPM rpm(2);
unsigned long last;

void setup() {
	Serial.begin(9600);
	pinMode(rpm.pin(), INPUT);
	last = millis();
}

void loop() {
	rpm.process();
#if 0
	unsigned long now = millis();
	if (now - last > 100) {
		Serial.println(rpm.period());
		last = now;
	}
#endif
}
