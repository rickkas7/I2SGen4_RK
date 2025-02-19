#include "Particle.h"

#include "rtl_i2c.h"

SYSTEM_MODE(SEMI_AUTOMATIC);

// System thread defaults to on in 6.2.0 and later and this line is not required
#ifndef SYSTEM_VERSION_v620
SYSTEM_THREAD(ENABLED);
#endif

SerialLogHandler logHandler(LOG_LEVEL_INFO);

void setup() {
    waitFor(Serial.isConnected, 10000);
    delay(2000);
    
    runTest();
}

void loop() {

}

