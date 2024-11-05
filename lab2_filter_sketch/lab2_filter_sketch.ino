// A digital frequency selective filter
// A. Kruger, 2019
// revised R. Mudumbai, 2020 & 2024

// The following defines are used for setting and clearing register bits
// on the Arduino processor. Low-level stuff: leave alone.

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif

#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

int analogPin = 36;     // Specify analog input pin. Make sure to keep between 0 and 5V.
int LED = 12;           // Specify output analog pin with indicator LED

const int n = 3;   // Number of past input and output samples to buffer; change this to match order of your filter
//const int n = 6; Possibly more correct

int m = 10;        // Number of past outputs to average for hysteresis

// Coefficients for Second-Order Sections (SOS) and gain
float den[] = {1.0, -1.50178733768644, 0.934754753103344, -1.63304298324027, 0.943624840480612, -1.52527119243690, 0.881618592363189};  // Denominator coefficients
float num[] = {0.0609109758872590, 0.0, -0.0609109758872590, 0.0609109758872590, 0.0, -0.0609109758872590, 0.0591907038184050};  // Numerator coefficients
float gain = 1.0 * 0.0609109758872590 * 0.0609109758872590 * 0.0591907038184050;  // Overall gain value

float x[n], y[n], yn_value, s[10];  // Buffers to hold input, output, and intermediate values

float threshold_val = 0.2; // Threshold value. Anything higher than the threshold will turn the LED off, anything lower will turn the LED on

// time between samples Ts = 1/Fs. If Fs = 3000 Hz, Ts=333 us
//int Ts = 333;
int Ts = 200;

void setup() {
   Serial.begin(1200);

   //sbi(ADCSRA, ADPS2);     // Set ADC clock prescaler for faster ADC conversions
   //cbi(ADCSRA, ADPS1);
   //cbi(ADCSRA, ADPS0);

   pinMode(LED, OUTPUT);   // Makes the LED pin an output

   for (int i = 0; i < n; i++) {
       x[i] = y[i] = 0;
   }

   for (int i = 0; i < m; i++) {
       s[i] = 0;
   }
   yn_value = 0;
}

void loop() {
   unsigned long t1;
   float changet = micros();

   while (1) {
      t1 = micros();

      // Shift values in buffers for new sample
      for (int i = n - 1; i > 0; i--) {
          x[i] = x[i - 1];
          y[i] = y[i - 1];
      }

      for (int i = m - 1; i > 0; i--) {  // Shift absolute output
          s[i] = s[i - 1];
      }

      int val = analogRead(analogPin);  // New input
      x[0] = val * (5.0 / 1023.0) - 2.5;  // Scale to match ADC resolution and range

      // Calculate the next value of the difference equation
      yn_value = num[0] * x[0];

      for (int i = 1; i < n; i++) {
          yn_value = yn_value - den[i] * y[i] + num[i] * x[i];
      }

      y[0] = yn_value;  // New output

      // Use the output of the filter for its intended purpose
      s[0] = abs(2 * yn_value);  // Absolute value of the filter output

      // SAMPLE Hysteresis: Take the max of the past 10 samples and compare that with the threshold
      float maxs = 0;
      for (int i = 0; i < m; i++) {
          if (s[i] > maxs)
              maxs = s[i];
      }

      // Check the output value against the threshold value every 10^6 microseconds or 1 second
      if ((micros() - changet) > 1000000) {
          Serial.println(maxs);
          changet = micros();

          if (maxs < threshold_val) {
              digitalWrite(LED, HIGH);
          } else {
              digitalWrite(LED, LOW);
          }
      }

      // The filter was designed for a 3000 Hz sampling rate. This corresponds
      // to a sample every 333 us. The code above must execute in less time
      // (if it doesn't, it is not possible to do this filtering on this processor).
      // Below we tread some water until it is time to process the next sample

      if ((micros() - t1) > Ts) {
          // if this happens, you must reduce Fs, and/or simplify your filter to run faster
          Serial.println("MISSED A SAMPLE");
      }
      while ((micros() - t1) < Ts);
   }
} 


