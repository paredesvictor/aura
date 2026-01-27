#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "EMGFilters.h"

#define TIMING_DEBUG 1

#define SensorInputPin 4 // input pin number

EMGFilters myFilter;
// discrete filters must works with fixed sample frequence
// our emg filter only support "SAMPLE_FREQ_500HZ" or "SAMPLE_FREQ_1000HZ"
// other sampleRate inputs will bypass all the EMG_FILTER
int sampleRate = SAMPLE_FREQ_500HZ;
// For countries where power transmission is at 50 Hz
// For countries where power transmission is at 60 Hz, need to change to
// "NOTCH_FREQ_60HZ"
// our emg filter only support 50Hz and 60Hz input
// other inputs will bypass all the EMG_FILTER
int humFreq = NOTCH_FREQ_50HZ;

// Calibration:
// put on the sensors, and release your muscles;
// wait a few seconds, and select the max value as the threshold;
// any value under threshold will be set to zero
static int Threshold = 15;

unsigned long timeStamp;
unsigned long timeBudget;

void setup() {
    myFilter.init(sampleRate, humFreq, true, true, true);
    Serial.begin(115200);
    Serial.println("Started");
}

void loop() {
    int value = analogRead(SensorInputPin);
    int DataAfterFilter = myFilter.update(value);
    int envelope = DataAfterFilter;
    envelope = (envelope > Threshold) ? envelope : 0;
    if (envelope > Threshold) {
        Serial.println(envelope);
    }
    delayMicroseconds(500);

}