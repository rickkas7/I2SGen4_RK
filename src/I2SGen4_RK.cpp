#include "I2SGen4_RK.h"

#include <cmath>

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

void I2SGen4_RK::start() { 
    g_rtl_i2s_api.sampleRateHz = audioSettings.getSampleRateHz();
    g_rtl_i2s_api.stereo = (int) audioSettings.getStereo();
    g_rtl_i2s_api.bits24 = (int) audioSettings.getBits24();

    g_rtl_i2s_api.init(); 
}

I2SGen4_RK &I2SGen4_RK::withFillFromBufferStreamable(BufferStreamable *stream, bool runAsISR) {
    withFillCallback([stream](void *buf, size_t bufSize, size_t sampleCount, size_t bytesPerSample, size_t channelCount) {
        stream->copyPage((uint8_t *) buf); 
    });
    if (runAsISR) {
        withFillCallbackRunAsISR();
    }
    return *this;
}

I2SGen4_RK &I2SGen4_RK::withReceiveToBufferStreamable(BufferStreamable *stream, bool runAsISR) {
    withReceiveCallback([stream](const void *buf, size_t bufSize, size_t sampleCount, size_t bytesPerSample, size_t channelCount) {
        stream->writePage((const uint8_t *) buf); 
    });
    if (runAsISR) {
        withReceiveCallbackRunAsISR();
    }
    return *this;
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
        _instance->fillCallback( );
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
// I2SGen4_RK::BufferStreamable
//

void I2SGen4_RK::BufferStreamable::clear() {
    userStreamCompletionCalled = false;

}

void I2SGen4_RK::BufferStreamable::handleUserStreamCompletion() {
    if (userStreamCompletion && !userStreamCompletionCalled) {
        userStreamCompletionCalled = true;
        userStreamCompletion();
    }
}

//
// I2SGen4_RK::BufferVector
// 
I2SGen4_RK::BufferVector::~BufferVector() {
    free();
}

void I2SGen4_RK::BufferVector::free() {
    for(size_t ii = 0; ii < buffers.size(); ii++) {
        delete buffers[ii];
    }
    buffers.clear();
}


void I2SGen4_RK::BufferVector::clear() {
    for(size_t ii = 0; ii < buffers.size(); ii++) {
        buffers[ii]->clear();
    }
    BufferStreamable::clear();
}

bool I2SGen4_RK::BufferVector::allocate(size_t numBuffers) {
    bool result = true;

    free();
    buffers.reserve(numBuffers);
    
    for(size_t ii = 0; ii < numBuffers; ii++) {
        Buffer *b = new Buffer();
        if (!b) {
            result = false;
            break;
        }
        buffers.push_back(b);
    }

    return result;
}

void I2SGen4_RK::BufferVector::copyPage(uint8_t *dest) {
    if (!atEOF()) {
        size_t tempIndex = indexAtomic.fetch_add(1);

        memcpy(dest, buffers[tempIndex]->buffer, Buffer::size);    
    }
    else {
        BufferStreamable::handleUserStreamCompletion();
        memset(dest, 0, Buffer::size);
    }
}

void I2SGen4_RK::BufferVector::writePage(const uint8_t *src) {
    if (!atEOF()) {
        size_t tempIndex = indexAtomic.fetch_add(1);

        memcpy(buffers[tempIndex]->buffer, src, Buffer::size);    
    }
}


//
// I2SGen4_RK::BufferConst
// 

size_t I2SGen4_RK::BufferConst::getOffset() const { 
    size_t offset = offsetAtomic.load(); 
    if (offset > bufSize) {
        offset = bufSize;
    }
    return offset;
}

void I2SGen4_RK::BufferConst::copyPage(uint8_t *dest) {
    if (!atEOF()) {
        size_t destOffset = 0;

        while(true) {
            // RTL_I2S_DMA_PAGE_SIZE is in bytes
            size_t offset = offsetAtomic.fetch_add(RTL_I2S_DMA_PAGE_SIZE);

            if (offset < bufSize) {
                size_t count = bufSize - offset;        
                if (count > (RTL_I2S_DMA_PAGE_SIZE - destOffset)) {
                    count = (RTL_I2S_DMA_PAGE_SIZE - destOffset);
                }
                memcpy(&dest[destOffset], &buf[offset], count);  
                destOffset += count;
                
                if (!continuousLoop) {
                    if (destOffset < RTL_I2S_DMA_PAGE_SIZE) {
                        memset(&dest[destOffset], 0, RTL_I2S_DMA_PAGE_SIZE - destOffset);
                    }        
                    break;
                }
                else
                if (offsetAtomic.load() >= bufSize) {
                    offsetAtomic.store(0);
                }
                else {
                    break;
                }
        
            }
            else {
                memset(&dest[destOffset], 0, RTL_I2S_DMA_PAGE_SIZE - destOffset);
                break;
            }
        }
    }
    else {
        BufferStreamable::handleUserStreamCompletion();
        memset(dest, 0, Buffer::size);
    }
}


//
// I2SGen4_TestSine16_RK
//
I2SGen4_TestSine16_RK::I2SGen4_TestSine16_RK() {
}

I2SGen4_TestSine16_RK::~I2SGen4_TestSine16_RK() {
    if (samples) {
        delete[] samples;
        samples = nullptr;
    }
}

I2SGen4_TestSine16_RK &I2SGen4_TestSine16_RK::withAudioSettings(const I2SGen4_RK::AudioSettings &settings) {
    channelCount = settings.getChannelCount();
    samplesFramesPerBuffer = settings.getSamplesFramesPerBuffer();
    return *this;
}

bool I2SGen4_TestSine16_RK::allocate(int frequencyHz, int samplesPerSecond) {
    const double pi = 3.14159265358979323846;

    double sinePeriodSec = 1.0 / (double)frequencyHz;

    double samplePeriodSec = 1.0 / (double)samplesPerSecond;

    sampleCount = (size_t)ceil(sinePeriodSec * (float)samplesPerSecond);
    if (sampleCount < 2) {
        return false;
    }

    if (samples) {
        delete[] samples;
        samples = nullptr;
    }
    samples = new int16_t[sampleCount];
    if (!samples) {
        return false;
    }

    double t = 0;
    for(size_t ii = 0; ii < sampleCount; ii++, t += samplePeriodSec) {
        double val = 32767.0 * sin(2 * pi * (double)frequencyHz * t);

        samples[ii] = (int16_t)val;
    }

    indexAtomic.store(0);

    return true;
}

int16_t I2SGen4_TestSine16_RK::getSample() {
    size_t tempIndex = indexAtomic.fetch_add(1);

    int16_t result = samples[tempIndex % sampleCount];

    return result;
}


void I2SGen4_TestSine16_RK::copyPage(uint8_t *dest) {
    size_t index = 0;
    for(size_t ii = 0; ii < samplesFramesPerBuffer; ii++) {
        int16_t value = getSample();
        for(size_t jj = 0; jj < channelCount; jj++) {
            ((int16_t *)dest)[index++] = value;
        }
    }

}




