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
    #if (PLATFORM_ID == PLATFORM_P2) || (PLATFORM_ID == PLATFORM_MSOM)
    g_rtl_i2s_api.platform = PLATFORM_ID;
#else
    #error "I2SGen4_RK cannot be used on this platform"
#endif

    g_rtl_i2s_api.fillCallback = fillCallbackStatic;
    g_rtl_i2s_api.receiveCallback = receiveCallbackStatic;

}

I2SGen4_RK::~I2SGen4_RK() {
}

I2SGen4_RK &I2SGen4_RK::withFillCallback(std::function<void(void *buf, size_t bufSize, size_t sampleCount, size_t bytesPerSample, size_t numChannels)> fillCallback, bool runAsISR) { 
    userFillCallback = fillCallback; 
    userFillCallbackRunAsISR = runAsISR;
    return *this; 
};


I2SGen4_RK &I2SGen4_RK::withReceiveCallback(std::function<void(const void *buf, size_t bufSize, size_t sampleCount, size_t bytesPerSample, size_t numChannels)> receiveCallback, bool runAsISR) { 
    userReceiveCallback = receiveCallback; 
    userReceiveCallbackRunAsISR = runAsISR;
    return *this; 
};


void I2SGen4_RK::setup() {
    os_mutex_create(&mutex);

    os_queue_create(&fillQueue, sizeof(void*), RTL_I2S_DMA_PAGE_COUNT, 0);
    os_queue_create(&receiveQueue, sizeof(void*), RTL_I2S_DMA_PAGE_COUNT, 0);

    thread = new Thread("I2S", [this]() { return threadFunction(); }, OS_THREAD_PRIORITY_DEFAULT, 3072);
}

void I2SGen4_RK::loop() {
    // Put your code to run during the application thread loop here
}

os_thread_return_t I2SGen4_RK::threadFunction(void) {
    while(true) {
        int err;
        void *buf;

        err = os_queue_take(fillQueue, &buf, 0, 0);
        if (err == 0) {
            fillCallback(buf);
        }

        err = os_queue_take(receiveQueue, &buf, 0, 0);
        if (err == 0) {
            receiveCallback(buf);
        }

        // Put your code to run in the worker thread here
        delay(1);
    }
}

// [static]
void I2SGen4_RK::fillCallbackStatic(void *buf) {
    if (_instance) {
        _instance->fillCallback(buf);
    }
   
}

void I2SGen4_RK::fillCallback(void *buf) {
    if (userFillCallbackRunAsISR) {
        fillCallbackInternal(buf);
    }
    else {
        os_queue_put(fillQueue, &buf, 0, 0);
    }
}


void I2SGen4_RK::fillCallbackInternal(void *buf) {
    if (userFillCallback) {
        size_t bytesPerSample = g_rtl_i2s_api.bits24 ? 4 : 2;
        size_t numChannels = g_rtl_i2s_api.stereo ? 2 : 1;
        size_t sampleCount = getDmaPageSize() / bytesPerSample / numChannels;
        
        userFillCallback(buf, getDmaPageSize(), sampleCount, bytesPerSample, numChannels);
    } 
    else {
        memset(buf, 0, getDmaPageSize());
    }
    g_rtl_i2s_api.sendPage(buf);
}




// [static]
void I2SGen4_RK::receiveCallbackStatic(void *buf) {
    if (_instance) {
        _instance->receiveCallback(buf);
    }
}

void I2SGen4_RK::receiveCallback(void *buf) {
    if (userReceiveCallbackRunAsISR) {
        receiveCallbackInternal(buf);
    }
    else {
        os_queue_put(receiveQueue, &buf, 0, 0);
    }
}

void I2SGen4_RK::receiveCallbackInternal(void *buf) {
    if (userReceiveCallback) {
        size_t bytesPerSample = g_rtl_i2s_api.bits24 ? 4 : 2;
        size_t numChannels = g_rtl_i2s_api.stereo ? 2 : 1;
        size_t sampleCount = getDmaPageSize() / bytesPerSample / numChannels;

        userReceiveCallback(buf, getDmaPageSize(), sampleCount, bytesPerSample, numChannels);
    }

    g_rtl_i2s_api.returnRecvPage();
}

//
// I2SGen4_Test_RK
//


// This is from the RTL SDK I2S example code
// [static]
const int16_t I2SGen4_Test_RK::sine16[16] = 
{
    0, 12539/4, 23170/4, 30273/4, 32767/4, 30273/4, 23170/4, 12539/4,
    0, -12539/4, -23170/4, -30273/4, -32767/4, -30273/4, -23170/4, -12539/4
};

// This is from the RTL SDK I2S example code
// [static]
const int32_t I2SGen4_Test_RK::sine24[16] =
{
    0, 12539*256/4, 23170*256/4, 30273*256/4, 32767*256/4, 30273*256/4, 23170*256/4, 12539*256/4,
    0, -12539*256/4, -23170*256/4, -30273*256/4, -32767*256/4, -30273*256/4, -23170*256/4, -12539*256/4
};


// This is from the RTL SDK I2S example code
// [static]
void I2SGen4_Test_RK::generateSample16(int16_t *buf, size_t sampleCount, size_t channelCount)
{
	for (size_t ii = 0; ii < sampleCount; ii += channelCount){
		buf[ii] = sine16[(ii/channelCount)%16];
		if(channelCount>=2) {
        	buf[ii+1] = sine16[(ii/channelCount)%16];
        }
    }
}


// This is from the RTL SDK I2S example code
// [static]
void I2SGen4_Test_RK::generateSample24(int *buf, size_t sampleCount, size_t channelCount)
{
	for (size_t ii = 0; ii < sampleCount; ii += channelCount){
		buf[ii] = sine24[(ii/channelCount)%16]&0xFFFFFF;
		if(channelCount>=2) {
			buf[ii+1] = sine24[(ii/channelCount)%16]&0xFFFFFF;
        }
    }
}
