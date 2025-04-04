#include "I2SGen4_RK.h"

I2SGen4_RK *I2SGen4_RK::_instance;

// [static]
I2SGen4_RK &I2SGen4_RK::instance() {
    if (!_instance) {
        _instance = new I2SGen4_RK();
    }
    return *_instance;
}

I2SGen4_RK::I2SGen4_RK() {
}

I2SGen4_RK::~I2SGen4_RK() {
}

void I2SGen4_RK::setup() {
    os_mutex_create(&mutex);

#if (PLATFORM_ID == PLATFORM_P2) || (PLATFORM_ID == PLATFORM_MSOM)
    g_rtl_i2s_api.platform = PLATFORM_ID;
#else
    #error "I2SGen4_RK cannot be used on this platform"
#endif

    thread = new Thread("I2S", [this]() { return threadFunction(); }, OS_THREAD_PRIORITY_DEFAULT, 3072);
}

void I2SGen4_RK::loop() {
    // Put your code to run during the application thread loop here
}

os_thread_return_t I2SGen4_RK::threadFunction(void) {
    while(true) {
        // Put your code to run in the worker thread here
        delay(1);
    }
}
 