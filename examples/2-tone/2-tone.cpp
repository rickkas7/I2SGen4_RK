#include "I2SGen4_RK.h"

SYSTEM_MODE(SEMI_AUTOMATIC);

// System thread defaults to on in 6.2.0 and later and this line is not required
#ifndef SYSTEM_VERSION_v620
SYSTEM_THREAD(ENABLED);
#endif

SerialLogHandler logHandler(LOG_LEVEL_INFO);

I2SGen4_RK::AudioSettings audioSettings;
I2SGen4_TestSine16_RK testSine;
const int sampleRate = 16000; // Hz

int freqFunction(String cmd);

void setup()
{
    Particle.function("freq", freqFunction);
    
    // The next line is only used during development to see early log messages
    waitFor(Serial.isConnected, 10000); delay(2000);
    


    audioSettings
        .withSampleRateHz(sampleRate)
        .withStereo()
        .withBits16();

    testSine
        .withAudioSettings(audioSettings)
        .allocate(1000, sampleRate);

    I2SGen4_RK::instance()
        .withAudioSettings(audioSettings)
        .withDirection(I2SGen4_RK::Direction::TX_ONLY)
        .withFillFromBufferStreamable(&testSine)
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
