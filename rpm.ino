#include "Arduino.h"

class RPM {
public:
	RPM(uint8_t pin, unsigned int period = 100) :
	   pin_(pin), period_(period),
	   max_(0), offset_(10),
	   rpm_(0), ticks_(0), min_ticks_(1),
	   armed_(true)
	{
		t0_ = 0;
		t1_ = t2_ = millis();
		offset_half_ = offset_ / 2;
	}

	void process();
	inline unsigned long value() const { return rpm_; }

private:
	uint8_t pin_;
	unsigned int period_;
	int max_;
	int offset_, offset_half_;
	unsigned long t0_, t1_, t2_, rpm_;
	unsigned char ticks_, min_ticks_;
	bool armed_; // expecting next count?
};

/// Count a tick each time the read value falls first time below MIN_VALUE
/// If t2 > t1, we can indeed compute the RPM. If value raises above MAX_VALUE, arm again.
void RPM::process() {
	int adc = analogRead(A5);
	if (adc >= max_)
		max_ = adc;

	if (adc + offset_ < max_) {
		if (armed_) { // only count a tick when first falling below max - offset
			armed_ = false;
			++ticks_;
		}
	} else if (adc + offset_half_ > max_) {
		armed_ = true;
	}

	t2_ = millis();
	if (t2_ > t1_ + period_) {
		max_ -= 1; // decay max
		t1_ = t2_;

		// compute current maximal rpm estimate
		unsigned long unit = 60000ul / (t2_ - t0_); // not faster than this
		if (ticks_ >= min_ticks_) {
			rpm_ = unit * min_ticks_;
			min_ticks_ = ticks_; // increase expected number of ticks
			ticks_ = 0;
			t0_ = t2_;
		} else {
			unsigned long cur_max = (1+ticks_) * unit;
			if (rpm_ > cur_max) {
				rpm_ = cur_max;
				if (min_ticks_ > 1)
					--min_ticks_;
			}
		}
	} else if (t2_ < t1_) { // millis() overflow
		ticks_ = 0;
		t0_ = t1_ = t2_;
	}
#if 1
	Serial.print(t2_);
	Serial.print(",");
	Serial.print(adc);
	Serial.print(",");
	Serial.print(max_+1);
	Serial.print(",");
	Serial.print(ticks_);
	Serial.print(",");
	Serial.print(min_ticks_);
	Serial.print(",");
	Serial.println(rpm_);
#endif
}

RPM rpm(A5);

void setup() {
	Serial.begin(9600);
	pinMode(A5,INPUT);
}

void printDouble( double val, unsigned int precision) {
	// prints val with number of decimal places determine by precision
	// NOTE: precision is 1 followed by the number of zeros for the desired number of decimial places
	// example: printDouble( 3.1415, 100); // prints 3.14 (two decimal places)

	Serial.print (int(val));  //prints the int part
	Serial.print("."); // print the decimal point
	unsigned int frac;
	if (val >= 0)
		frac = (val - int(val)) * precision;
	else
		frac = (int(val) - val ) * precision;
	Serial.print(frac, DEC) ;
}

double getADC(int pin, int scale){
	return analogRead(pin) * (5.0 / 1023.0) * scale;
}

void printData(){
	Serial.print(rpm.value());
	int list[7] = {A0,A1,A2,A3,A4,A5,A6};
	for(int i : list){
		Serial.print(",");
		printDouble(getADC(i,11),100);
	}
}

void loop() {
	rpm.process();
}
