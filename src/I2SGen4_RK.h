#ifndef __I2SGEN4_RK_H
#define __I2SGEN4_RK_H

#include "Particle.h"
#include "rtl_i2s.h"

/**
 * This class is a singleton; you do not create one as a global, on the stack, or with new.
 * 
 * From global application setup you must call:
 * I2SGen4_RK::instance().setup();
 * 
 * From global application loop you must call:
 * I2SGen4_RK::instance().loop();
 */
class I2SGen4_RK {
public:
    /**
     * @brief Direction
     * 
     * These correspond to I2S_DIR_RX (0), I2S_DIR_TX (1), I2S_DIR_TXRX (2) and must match!
     */
    enum class Direction : int {
        RX_ONLY = 0, //!< Receive data from I2S only
        TX_ONLY = 1, //!< Transmit data to I2S only
        RX_TX = 2    //!< Both transmit and receive
    };

    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use I2SGen4_RK::instance() to instantiate the singleton.
     */
    static I2SGen4_RK &instance();

    /**
     * @brief Set the sample rate in Hz. Default is 16000 Hz.
     * 
     * @param sampleRate 
     * @return I2SGen4_RK& 
     * 
     * Valid values include: 8000, 16000, 24000, 32000, 48000, 96000, 44100.
     * 
     */
    I2SGen4_RK &withSampleRate(int sampleRateHz) { g_rtl_i2s_api.sampleRateHz = sampleRateHz; return *this; };

    /**
     * @brief Get the sample rate in Hz.
     * 
     * @return int 
     */
    int getSampleRate() const { return g_rtl_i2s_api.sampleRateHz; };

    /**
     * @brief Sets mono (monophonic, single channel) mode. Default is stereo.
     * 
     * @return I2SGen4_RK& 
     * 
     * Note: Mono 24-bit mode is not supported by the hardware!
     */
    I2SGen4_RK &withMono() { g_rtl_i2s_api.stereo = false; return *this; };

    /**
     * @brief Sets stereo (stereophonic, two channel) mode. Default is stereo.
     *
     * @param stereo Set the stereo flag or not (defaults to true) 
     * 
     * @return I2SGen4_RK& 
     */
    I2SGen4_RK &withStereo(bool stereo = true) { g_rtl_i2s_api.stereo = stereo; return *this; };

    /**
     * @brief Get the stereo flag (stereo = true, mono = false)
     * 
     * @return true 
     * @return false 
     */
    bool getStereo(void) const { return g_rtl_i2s_api.stereo; };

    /**
     * @brief Sets 16-bit mode (the default)
     * 
     * @return I2SGen4_RK& 
     */
    I2SGen4_RK &withBits16() { g_rtl_i2s_api.bits24 = false; return *this; };

    /**
     * @brief Sets 24- bit mode
     * 
     * @param bits24 Set to true for 24 bit mode (default parameter), or false for 16-bit
     * @return I2SGen4_RK& 
     * 
     * 32-bit mode is not supported.
     */
    I2SGen4_RK &withBits24(bool bits24 = true) { g_rtl_i2s_api.bits24 = bits24; return *this; };

    /**
     * @brief Get the 24 bit flag (true = 24 bits, false = 16 bits)
     * 
     * @return true 
     * @return false 
     */
    bool getBits24(void) const { return g_rtl_i2s_api.bits24; };

    /**
     * @brief Set the direction (RX, TX, or both). Default is I2SGen4_RK::Direction::RX_TX.
     * 
     * @param dir 
     * @return I2SGen4_RK& 
     */
    I2SGen4_RK &withDirection(Direction dir) { g_rtl_i2s_api.direction = (int)dir; return *this; };

    /**
     * @brief Get the Direction objectGet 
     * 
     * @return Direction 
     */
    Direction getDirection(void) const { return (Direction)g_rtl_i2s_api.direction; };

    /**
     * @brief Get the DMA page size in bytes 2 <= pageSize <= 4096. Default is 768.
     * 
     * @return int 
     * 
     * You cannot set this value; it is compiled into the code.
     */
    int getDmaPageSize() const { return RTL_I2S_DMA_PAGE_SIZE; };

    /**
     * @brief Get the number of DMA pages 2 <= pageCount <= 4. Default is 4.
     * 
     * @return int 
     * 
     * You cannot set this value; it is compiled into the code.
     */
    int getDmaPageCount() const { return RTL_I2S_DMA_PAGE_COUNT; };
    

    /**
     * @brief Set function that is called to fill a buffer with I2S samples to send
     * 
     * @param fillCallback 
     * @return I2SGen4_RK& 
     * 
     * The fill callback or lambda has the prototype:
     * 
     * void callback(const void *buf, size_t bufSize, size_t sampleCount, size_t bytesPerSample, size_t channelCount)
     * 
     * - buf The buffer where the data is stored.
     * - bufSize The size of the buffer in bytes. This is getDmaPageSize().
     * - sampleCount The number of samples in the buffer, taking into account the size in bits (16 or 24) and the number of channels (1 or 2, mono or stereo).
     * - bytesPerSample 2 for 16-bit, or 4 for 24-bit, For 24 bit, samples are aligned on the LSB side (mask 0x00ffffff).
     * - channelCount 1 for mono or 2 for stereo.
     */
    I2SGen4_RK &withFillCallback(std::function<void(void *buf, size_t bufSize, size_t sampleCount, size_t bytesPerSample, size_t channelCount)> fillCallback) { userFillCallback = fillCallback; return *this; };


    /**
     * @brief Set the fill callback to run at interrupt (ISR) level
     * 
     * @param runAsISR 
     * @return I2SGen4_RK& 
     * 
     * By default, the fill callback runs from a thread. By using this call, you can run the callback at the ISR level, which has lower
     * latency, but more restrictions on what you can do.
     */
    I2SGen4_RK &withFillCallbackRunAsISR(bool runAsISR = true) { userFillCallbackRunAsISR = runAsISR; return *this; };


    /**
     * @brief Set the function to call when data is received by I2S.
     * 
     * @param receiveCallback 
     * @return I2SGen4_RK& 
     * 
     * The receive callback or lambda has the prototype:
     * 
     * void callback(const void *buf, size_t bufSize, size_t sampleCount, size_t bytesPerSample, size_t channelCount)
     * 
     * - buf The buffer where the data is stored.
     * - bufSize The size of the buffer in bytes. This is getDmaPageSize().
     * - sampleCount The number of samples in the buffer, taking into account the size in bits (16 or 24) and the number of channels (1 or 2, mono or stereo).
     * - bytesPerSample 2 for 16-bit, or 4 for 24-bit, For 24 bit, samples are aligned on the LSB side (mask 0x00ffffff).
     * - channelCount 1 for mono or 2 for stereo.
     */
    I2SGen4_RK &withReceiveCallback(std::function<void(const void *buf, size_t bufSize, size_t sampleCount, size_t bytesPerSample, size_t channelCount)> receiveCallback) { userReceiveCallback = receiveCallback; return *this; };


    /**
     * @brief Set the receive callback to run at interrupt (ISR) level
     * 
     * @param runAsISR 
     * @return I2SGen4_RK& 
     * 
     * By default, the receive callback runs from a thread. By using this call, you can run the callback at the ISR level, which has lower
     * latency, but more restrictions on what you can do.
     */
    I2SGen4_RK &withReceiveCallbackRunAsISR(bool runAsISR = true) { userReceiveCallbackRunAsISR = runAsISR; return *this; };

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use I2SGen4_RK::instance().setup();
     */
    void setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use I2SGen4_RK::instance().loop();
     */
    void loop();

    void start() { g_rtl_i2s_api.init(); };

    void stop() { g_rtl_i2s_api.deinit(); };

    /**
     * @brief Locks the mutex that protects shared resources
     * 
     * This is compatible with `WITH_LOCK(*this)`.
     * 
     * The mutex is not recursive so do not lock it within a locked section.
     */
    void lock() { os_mutex_lock(mutex); };

    /**
     * @brief Attempts to lock the mutex that protects shared resources
     * 
     * @return true if the mutex was locked or false if it was busy already.
     */
    bool tryLock() { return os_mutex_trylock(mutex); };

    /**
     * @brief Unlocks the mutex that protects shared resources
     */
    void unlock() { os_mutex_unlock(mutex); };


protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use I2SGen4_RK::instance() to instantiate the singleton.
     */
    I2SGen4_RK();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~I2SGen4_RK();

    /**
     * This class is a singleton and cannot be copied
     */
    I2SGen4_RK(const I2SGen4_RK&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    I2SGen4_RK& operator=(const I2SGen4_RK&) = delete;

    /**
     * @brief Worker thread function
     * 
     * This method is called to perform operations in the worker thread.
     * 
     * You generally will not return from this method.
     */
    os_thread_return_t fillThreadFunction(void);

    os_thread_return_t receiveThreadFunction(void);

    /**
     * @brief Function that is called to fill the buffer to send
     * 
     * @param buf 
     * @param bufSize 
     * @return int 
     */
    static void fillCallbackStatic();

    void fillCallback();

    void fillCallbackInternal();

    /**
     * @brief Function that is called to process a buffer received
     * 
     * @param buf 
     * @param bufSize 
     * @return int 
     */
    static void receiveCallbackStatic(void *buf);

    void receiveCallback(void *buf);

    void receiveCallbackInternal(void *buf);

    /**
     * @brief User callback to fill a buffer to send
     */
    std::function<void(void *buf, size_t bufSize, size_t sampleCount, size_t bytesPerSample, size_t channelCount)> userFillCallback = 0;

    bool userFillCallbackRunAsISR = false;

    /**
     * @brief User callback to process a buffer received
     * 
     */
    std::function<void(const void *buf, size_t bufSize, size_t sampleCount, size_t bytesPerSample, size_t channelCount)> userReceiveCallback = 0;

    bool userReceiveCallbackRunAsISR = false;

    /**
     * @brief Mutex to protect shared resources
     * 
     * This is initialized in setup() so make sure you call the setup() method from the global application setup.
     */
    os_mutex_t mutex = 0;

    /**
     * @brief Worker thread instance class
     * 
     * This is initialized in setup() so make sure you call the setup() method from the global application setup.
     */
    Thread *fillThread = 0;

    Thread *receiveThread = 0;

    os_queue_t fillQueue;

    os_queue_t receiveQueue;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static I2SGen4_RK *_instance;

};


/**
 * @brief Testing functions
 */
class I2SGen4_Test_RK {
public:
    static void generateSample16(int16_t *buf, size_t sampleCount, size_t channelCount);
    static void generateSample24(int *buf, size_t sampleCount, size_t channelCount);

    static const int16_t sine16[]; // 16 elements
    static const int32_t sine24[]; // 16 elements
};
    
#endif  /* __I2SGEN4_RK_H */