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

void I2SGen4_RK::setup() {
    os_mutex_create(&mutex);

    os_queue_create(&fillQueue, sizeof(void*), RTL_I2S_DMA_PAGE_COUNT + 1, 0);
    os_queue_create(&receiveQueue, sizeof(void*), RTL_I2S_DMA_PAGE_COUNT + 1, 0);

    fillThread = new Thread("I2S fill", [this]() { return fillThreadFunction(); }, OS_THREAD_PRIORITY_DEFAULT, 3072);
    receiveThread = new Thread("I2S receive", [this]() { return receiveThreadFunction(); }, OS_THREAD_PRIORITY_DEFAULT, 3072);
}

void I2SGen4_RK::loop() {
    // Put your code to run during the application thread loop here
}

os_thread_return_t I2SGen4_RK::fillThreadFunction(void) {
    while(true) {
        int err;
        void *unused;

        err = os_queue_take(fillQueue, &unused, 1000, 0);
        if (err == 0) {
            fillCallbackInternal();
        }
    }
}

os_thread_return_t I2SGen4_RK::receiveThreadFunction(void) {
    while(true) {
        int err;
        void *buf;

        err = os_queue_take(receiveQueue, &buf, 1000, 0);
        if (err == 0) {
            receiveCallbackInternal(buf);
        }
    }
}
// [static]
void I2SGen4_RK::fillCallbackStatic() {
    if (_instance) {
        _instance->fillCallback();
    }
   
}

void I2SGen4_RK::fillCallback() {
    if (userFillCallbackRunAsISR) {
        fillCallbackInternal();
    }
    else {
        void *unused = 0;
        os_queue_put(fillQueue, &unused, 0, 0);
    }
}


void I2SGen4_RK::fillCallbackInternal() {
    void *buf = g_rtl_i2s_api.getTxPage();
    if (!buf) {
        return;
    }

    if (userFillCallback) {
        
        size_t bytesPerSample = g_rtl_i2s_api.bits24 ? 4 : 2;
        size_t channelCount = g_rtl_i2s_api.stereo ? 2 : 1;
        size_t sampleCount = getDmaPageSize() / bytesPerSample / channelCount;
        
        userFillCallback(buf, getDmaPageSize(), sampleCount, bytesPerSample, channelCount);
    } 
    else {
        memset(buf, 0, getDmaPageSize());
    }
    g_rtl_i2s_api.sendTxPage(buf);
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
        size_t channelCount = g_rtl_i2s_api.stereo ? 2 : 1;
        size_t sampleCount = getDmaPageSize() / bytesPerSample / channelCount;

        userReceiveCallback(buf, getDmaPageSize(), sampleCount, bytesPerSample, channelCount);
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
