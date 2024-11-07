// A digital frequency selective filter
// A. Kruger, 2019
// revised R. Mudumbai, 2020 & 2024

// The following defines are used for setting and clearing register bits
// on the Arduino processor. Low-level stuff: leave alone.

#include "twilio.hpp"

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif

#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

int analogPin = 33;     // Specify analog input pin. Make sure to keep between 0 and 5V.
int LED = 2;           // Specify output analog pin with indicator LED

const int n = 7; // Number of past input and output samples to buffer; change this to match order of your filter

int m = 10;        // Number of past outputs to average for hysteresis

float num[] = {0.00289819463372143,	0,	-0.00869458390116429,	0,	0.00869458390116429,	0,	-0.00289819463372143};
float den[] = {1,	-0.851172988233438,	2.61686206988090,	-1.38638472726139,	2.12575188112322,	-0.558397296097729,	0.532075368312090};

float x[n], y[n], yn_value, s[10];  // Buffers to hold input, output, and intermediate values

float threshold_val = 0.2; // Threshold value. Anything higher than the threshold will turn the LED off, anything lower will turn the LED on

// time between samples Ts = 1/Fs. If Fs = 3000 Hz, Ts=333 us
//Fs = 2000 Hz
int Ts = 500;


// Twilio Variables
static const char *ssid = "";
static const char *password = "";
static const char *account_sid = "";
static const char *auth_token = "";
static const char *from_number = "";
static const char *to_number = "";
static const char *message = "Sent from my ESP32";

Twilio *twilio;

void setup() {
   Serial.begin(1200);
   //WiFi.begin(ssid, password);
   //twilio = new Twilio(account_sid, auth_token);

   //sbi(ADCSRA, ADPS2);     // Set ADC clock prescaler for faster ADC conversions
   //cbi(ADCSRA, ADPS1);
   //cbi(ADCSRA, ADPS0);
   pinMode(analogPin, INPUT);
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

        //Important section for messaging
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

//void sendMessage() {
  //twilio->send_message(to_number, from_number, message, response);
//}


