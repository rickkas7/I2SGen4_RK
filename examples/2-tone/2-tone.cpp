#include "I2SGen4_RK.h"

SYSTEM_MODE(SEMI_AUTOMATIC);

// System thread defaults to on in 6.2.0 and later and this line is not required
#ifndef SYSTEM_VERSION_v620
SYSTEM_THREAD(ENABLED);
#endif

SerialLogHandler logHandler(LOG_LEVEL_INFO);

I2SGen4_TestSine16_RK testSine;
const int sampleRate = 16000; // Hz

int freqFunction(String cmd);

void setup()
{
    Particle.function("freq", freqFunction);
    
    // The next line is only used during development to see early log messages
    waitFor(Serial.isConnected, 10000); delay(2000);
    
    // bool I2SGen4_TestSine16_RK::allocate(int frequencyHz, int sampleRateHz) 
    testSine.allocate(1000, sampleRate);

    I2SGen4_RK::instance()
        .withSampleRate(sampleRate)
        .withStereo()
        .withDirection(I2SGen4_RK::Direction::TX_ONLY)
        .withBits16()
        .withFillCallback([](void *buf, size_t bufSize, size_t sampleCount, size_t bytesPerSample, size_t channelCount) {
            testSine.copySamples((int16_t *)buf, sampleCount, channelCount);
        })
        .withFillCallbackRunAsISR()
        .withReceiveCallback([](const void *buf, size_t bufSize, size_t sampleCount, size_t bytesPerSample, size_t channelCount) {
        })
        .setup();

    I2SGen4_RK::instance().start();    

    Particle.connect();
}

void loop() {
    I2SGen4_RK::instance().loop();
}

int freqFunction(String cmd) {
    int freq = cmd.toInt();
    if (freq < 20) {
        freq = 20;
    }
    if (freq > 20000) {
        freq = 20000;
    }

    testSine.allocate(freq, sampleRate);

    return 0;
}
