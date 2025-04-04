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
    os_thread_return_t threadFunction(void);

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
    Thread *thread = 0;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static I2SGen4_RK *_instance;

};
#endif  /* __I2SGEN4_RK_H */