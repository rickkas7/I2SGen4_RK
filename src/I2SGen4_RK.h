#ifndef __I2SGEN4_RK_H
#define __I2SGEN4_RK_H

#include "Particle.h"
#include "rtl_i2s.h"
#include <atomic>

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
     * @brief Container class for audio settings including sample rate, channels (stereo/mono), and bits per sample
     */
    class AudioSettings {
    public:
        /**
         * @brief Construct object with default settings
         * 
         * sampleRateHz: 16000
         * stereo: true
         * bits24: false
         */
        AudioSettings() {};

        /**
         * @brief Destructor. There is no additional storage used outside of the primitive class members (int, bool).
         */
        virtual ~AudioSettings() {};

        /**
         * @brief Set the sample rate in Hz. This value is stored but not validated by this method.
         * 
         * @param sampleRateHz 
         * @return AudioSettings& Reference to this object for chaining calls, fluent-style.
         */
        AudioSettings &withSampleRateHz(int sampleRateHz) { this->sampleRateHz = sampleRateHz; return *this; };
        
        /**
         * @brief Get the configured sample rate.
         * 
         * @return int 
         */
        int getSampleRateHz() const { return sampleRateHz; };

        /**
         * @brief Set mono (single channel)
         * 
         * @return AudioSettings& 
         */
        AudioSettings &withMono() { this->stereo = false; return *this; };

        /**
         * @brief Set stereo (two channels)
         * 
         * @param stereo true (default parameter) to set stereo
         * @return AudioSettings& 
         */
        AudioSettings &withStereo(bool stereo = true) { this->stereo = stereo; return *this; };

        /**
         * @brief Set the number of channels (1 or 2)
         * 
         * @param channels Must be 1 or 2; if not valid stereo is assumed.
         * @return AudioSettings& 
         */
        AudioSettings &withChannels(int channels) { this->stereo = (channels != 1); return *this; };

        /**
         * @brief Returns true if stereo (two channel mode)
         * 
         * @return true 
         * @return false 
         */
        bool getStereo() const { return stereo; };

        /**
         * @brief Returns the number of channels (1 or 2)
         * 
         * @return int 
         */
        int getChannelCount() const { return stereo ? 2 : 1; };

        /**
         * @brief Set 16-bit mode. The default is 16-bit.
         * 
         * @return AudioSettings& 
         */
        AudioSettings &withBits16() { this->bits24 = false; return *this; };

        /**
         * @brief Set 24-bit mode. The default is 16-bit.
         * 
         * @param bits24 true to set 24-bit mode or false to set 16-bit mode.
         * @return AudioSettings& 
         */
        AudioSettings &withBits24(bool bits24 = true) { this->bits24 = bits24; return *this; };
        
        /**
         * @brief Returns true if using 24-bit mode, false if using 16-bit mode (the default).
         * 
         * @return int
         */
        bool getBits24() const { return bits24; };

        /**
         * @brief Get the number of bytes per sample, 2 for 16-bit and 4 for 24-bit
         * 
         * @return size_t 
         */
        size_t getBytesPerSample() const { return bits24 ? 4 : 2; };

        /**
         * @brief Get the number of sample frames (all channels) per RTL_I2S_DMA_PAGE_SIZE buffer
         * 
         * @return size_t 
         */
        size_t getSamplesFramesPerBuffer() const { return RTL_I2S_DMA_PAGE_SIZE / getBytesPerSample() / getChannelCount();  };

        /**
         * @brief Copy constructor
         * 
         * @param src 
         */
        AudioSettings(const AudioSettings &src) {
            sampleRateHz = src.sampleRateHz;
            stereo = src.stereo;
            bits24 = src.bits24;
        }

        /**
         * @brief Copy operator
         * 
         * @param src 
         * @return AudioSettings& 
         */
        AudioSettings& operator=(const AudioSettings &src) {
            sampleRateHz = src.sampleRateHz;
            stereo = src.stereo;
            bits24 = src.bits24;
            return *this;
        }

        /**
         * @brief Equality operator - returns true if the sample rate, stereo, and bits24 settings are the same
         * 
         * @param other 
         * @return true 
         * @return false 
         */
        bool operator==(const AudioSettings &other) const {
            return (sampleRateHz == other.sampleRateHz) && (stereo == other.stereo) && (bits24 == other.bits24);
        }

    protected:
        /**
         * @brief Sample rate in Hz. Default is 16000
         * Valid values include: 8000, 16000, 24000, 32000, 48000, 96000, 44100.
         */
        int sampleRateHz = 16000;

        /**
         * @brief Stereo (true, the default), or mono (false)
         */
        bool stereo = true;

        /**
         * @brief 24-bit mode (true), or 16-bit mode (false, the default)
         */
        bool bits24 = false;
    };

    /**
     * @brief Class to hold a copy of a single DMA buffer of audio data
     * 
     * Avoid allocating the object on the stack because it contains the buffer as member data, which is
     * RTL_I2S_DMA_PAGE_SIZE bytes which can be large for a stack variable.
     */
    class Buffer {
    public:
        /**
         * @brief Construct a buffer object. The buffer is always RTL_I2S_DMA_PAGE_SIZE bytes.
         */
        Buffer() {};

        /**
         * @brief Destruct the buffer object. This object does not contain any heap allocated data outside of the object.
         */
        virtual ~Buffer() {};

        /**
         * @brief Set the value of every byte to 0
         */
        void clear() { memset(buffer, 0, Buffer::size); };

        /**
         * @brief Construct a Buffer object as a copy of another one.
         * 
         * @param src 
         */
        Buffer(const Buffer &src) { memcpy(buffer, src.buffer, Buffer::size); };

        /**
         * @brief Sets the contents of this buffer to be equal to the contents of another.
         * 
         * @param src 
         * @return Buffer& 
         * 
         * This method does not allocate any memory.
         */
        Buffer &operator=(const Buffer &src) { memcpy(buffer, src.buffer, Buffer::size); return *this; };

        /**
         * @brief The buffer of data bytes, always RTL_I2S_DMA_PAGE_SIZE
         */
        uint8_t buffer[RTL_I2S_DMA_PAGE_SIZE];

        /**
         * @brief Convenience static method Buffer::size() returns RTL_I2S_DMA_PAGE_SIZE, the size of the buffer.
         */
        static const size_t size = RTL_I2S_DMA_PAGE_SIZE;
    };

    /**
     * @brief Abstract base class for buffers that can be streamed, typically for playing audio
     * 
     * The BufferVector and BufferConst both are BufferStreamable.
     */
    class BufferStreamable {
    public:
        /**
         * @brief Set a function to call when all data has been sent
         * 
         * @param userStreamCompletion The function or C++ lambda to call
         * @return BufferStreamable& 
         */
        BufferStreamable &withUserStreamCompletion(std::function<void()> userStreamCompletion) { this->userStreamCompletion = userStreamCompletion; return *this; };
        
        /**
         * @brief Returns true of at the end of the stream
         * 
         * @return true 
         * @return false 
         */
        virtual bool atEOF() const = 0;

        /**
         * @brief Call to copy the next page of RTL_I2S_DMA_PAGE_SIZE bytes
         * 
         * @param dest 
         */
        virtual void copyPage(uint8_t *dest) = 0;

        /**
         * @brief Call to write a page. Supported on BufferVector, not supported on all streams
         * 
         * @param src The page to write, pointer to RTL_I2S_DMA_PAGE_SIZE bytes
         */
        virtual void writePage(const uint8_t *src) {};

        /**
         * @brief Clear any state
         */
        virtual void clear();

    protected:
        /**
         * @brief Called by subclasses when copyPage is called and at EOF
         * 
         * This checks to see if a userStreamCompletion function is available and has 
         * not been called yet, then calls it.
         */
        void handleUserStreamCompletion();

        /**
         * @brief Method called on first copyPage when at EOF
         */
        std::function<void()> userStreamCompletion = 0;

        /**
         * @brief Flag to 
         */
        bool userStreamCompletionCalled = false;
    };

    /**
     * @brief Class to hold a Vector of Buffer objects to hold a large audio sample
     * 
     * This object pre-allocates all of the memory to assure it can be done, and also because the library callback
     * can run as an ISR where it cannot allocate additional buffers.
     */
    class BufferVector : public BufferStreamable {
    public:
        /**
         * @brief Construct an object with no buffers allocated
         */
        BufferVector() { indexAtomic.store(0); };

        /**
         * @brief Destructor. This deletes the buffer pointers in the vector, as well.
         * 
         */
        virtual ~BufferVector();

        /**
         * @brief Frees the buffer pointers in the vector, and sets the vector length to 0.
         * 
         */
        void free();

        /**
         * @brief Zero out all buffers in the vector. This will produce silence if played.
         */
        void clear();

        /**
         * @brief Rewind the buffer so reading it using copyPage() will start over from the beginning
         * 
         * This method is safe to call from an ISR.
         */
        void rewind() { indexAtomic.store(0); };

        /**
         * @brief Allocate the specified number of buffers
         * 
         * @param numBuffers 
         * @return true 
         * @return false 
         * 
         * This deletes and previous buffers and rewinds to the beginning. If an out of memory condition
         * occurs, false will be returned. The vector will contain as many buffers as could be allocated,
         * but will still be valid.
         */
        bool allocate(size_t numBuffers);

        /**
         * @brief Get the current index into the vector to use next.
         * 
         * @return size_t 
         */
        size_t getIndex() const { return indexAtomic.load(); };

        /**
         * @brief Returns true if all of the buffers in the vector have been consumed.
         * 
         * @return true 
         * @return false 
         * 
         * This method is safe to call from an ISR. This method is part of the implementation of BufferStreamable.
         */
        virtual bool atEOF() const { return getIndex() == buffers.size(); };

        /**
         * @brief Copy a buffer out of the vector into dest. Always copies RTL_I2S_DMA_PAGE_SIZE bytes.
         * 
         * @param dest 
         * 
         * If you are atEOF() then dest will be filled will bytes with a 0 value.
         * 
         * This method is safe to call from an ISR. This method is part of the implementation of BufferStreamable.
         */
        virtual void copyPage(uint8_t *dest);

        /**
         * @brief Call to write a page. 
         * 
         * @param src The page to write, pointer to RTL_I2S_DMA_PAGE_SIZE bytes
         */
        virtual void writePage(const uint8_t *src);


    protected:
        /**
         * @brief Vector of Buffer objects. These objects are allocated during allocate() and are owned by this object.
         */
        std::vector<Buffer*> buffers;

        /**
         * @brief index (0-based) into the vector. This is a std::atomic atomic variable.
         */
        std::atomic<size_t> indexAtomic;
    };

    /**
     * @brief Class typically used to stream data out of a const uint8_t array
     */
    class BufferConst : public BufferStreamable {
    public:
        /**
         * @brief Construct a the object without setting the data; use set() to do that when using this constructor
         */
        BufferConst() { offsetAtomic.store(0); } ;

        /**
         * @brief Destructor. This does not delete the underlying data.
         */
        virtual ~BufferConst() {};

        /**
         * @brief Construct an object for the specified buffer. The buffer is not copied and must remain valid for the life of this object.
         * 
         * @param buf Pointer to uint8_t array
         * @param bufSize Size of uint8_t array in bytes
         * 
         * The buffer is not deleted when this object is deleted; you retain ownership of the buffer.
         */
        BufferConst(const uint8_t *buf, size_t bufSize) : buf(buf), bufSize(bufSize) { offsetAtomic.store(0); };

        /**
         * @brief Set the buffer to play continuously, looping back to the beginning after it has been played. Default is false.
         * 
         * @param continuousLoop 
         * @return BufferConst& 
         * 
         * Once you start playing continuously, you can set the flag to false it will stop after the current buffer is finished.
         */
        BufferConst &withContinuousLoop(bool continuousLoop = true) { this->continuousLoop = continuousLoop; return *this; };

        /**
         * @brief Use the specified buffer. The buffer is not copied and must remain valid for the life of this object.
         * 
         * @param buf Pointer to uint8_t array
         * @param bufSize Size of uint8_t array in bytes
         * 
         * The buffer is not deleted when this object is deleted; you retain ownership of the buffer.
         */
        void set(const uint8_t *buf, size_t bufSize) { this->buf = buf; this->bufSize = bufSize; offsetAtomic.store(0); };

        /**
         * @brief Rewind so copyPage will start copying from the beginning again.
         * 
         * This method is safe to call from an ISR.
         */
        void rewind() { offsetAtomic.store(0); };

        /**
         * @brief Get the current offset being read from for copyPage.
         * 
         * @return size_t 
         * 
         * This method is safe to call from an ISR.
         */
        size_t getOffset() const;

        /**
         * @brief Get the number of bytes left to be copied using copyPage.
         * 
         * @return size_t 
         * 
         * This method is safe to call from an ISR.
         */
        size_t getRemainder() const { return bufSize - getOffset(); };
        
        /**
         * @brief Returns true if all data has been copied.
         * 
         * @return true 
         * @return false 
         * 
         * This method is safe to call from an ISR. This method is part of the implementation of BufferStreamable.
         */
        virtual bool atEOF() const { return getOffset() >= bufSize; };

        /**
         * @brief Copies RTL_I2S_DMA_PAGE_SIZE bytes of data to dest
         * 
         * @param dest Filled in with data or zero bytes
         * 
         * If at EOF, dest is filled with bytes with the value 0. If the buf is not an multiple of 
         * RTL_I2S_DMA_PAGE_SIZE bytes, then it will be padded to 0 bytes.
         * 
         * This method is safe to call from an ISR. This method is part of the implementation of BufferStreamable.
         */
        virtual void copyPage(uint8_t *dest);

    protected:
        const uint8_t *buf = nullptr; //!< The buffer passed into the constructor or set() method (not a copy)
        size_t bufSize = 0; //!< THe size of the buffer passed int othe constructor or set() method

        bool continuousLoop = false; //!< True to start over from beginning after playing (default: false)

        /**
         * @brief Offset into the buffer using std::atomic
         * 
         * Note: The offset may be > bufSize; this will always occur if bufSize is not a multiple of Buffer::size
         * but getOffset() will limit the value to bufSize. This is because of the way the offset is atomically
         * incremented.
         */
        std::atomic<size_t> offsetAtomic;
    };


    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use I2SGen4_RK::instance() to instantiate the singleton.
     */
    static I2SGen4_RK &instance();

    /**
     * @brief Set the sample rate, number of channels, and number of bits. Must be called before start().
     * 
     * @param settings 
     * @return I2SGen4_RK& 
     */
    I2SGen4_RK &withAudioSettings(const AudioSettings &settings) { audioSettings = settings; return *this; };

    /**
     * @brief Set the sample rate in Hz. Default is 16000 Hz. Must be called before start().
     * 
     * @param sampleRateHz The sample rate in Hz 
     * @return I2SGen4_RK& 
     * 
     * Valid values include: 8000, 16000, 24000, 32000, 48000, 96000, 44100.
     * 
     */
    I2SGen4_RK &withSampleRateHz(int sampleRateHz) { audioSettings.withSampleRateHz(sampleRateHz); return *this; };

    /**
     * @brief Get the sample rate in Hz.
     * 
     * @return int 
     */
    int getSampleRateHz() const { return audioSettings.getSampleRateHz(); };

    /**
     * @brief Sets mono (monophonic, single channel) mode. Default is stereo. Must be called before start().
     * 
     * @return I2SGen4_RK& 
     * 
     * Note: Mono 24-bit mode is not supported by the hardware!
     */
    I2SGen4_RK &withMono() { audioSettings.withMono(); return *this; };

    /**
     * @brief Sets stereo (stereophonic, two channel) mode. Default is stereo. Must be called before start().
     *
     * @param stereo Set the stereo flag or not (defaults to true) 
     * 
     * @return I2SGen4_RK& 
     */
    I2SGen4_RK &withStereo(bool stereo = true) { audioSettings.withStereo(stereo); return *this; };

    /**
     * @brief Get the stereo flag (stereo = true, mono = false)
     * 
     * @return true 
     * @return false 
     */
    bool getStereo(void) const { return audioSettings.getStereo(); };

    /**
     * @brief Sets 16-bit mode (the default). Must be called before start().
     * 
     * @return I2SGen4_RK& 
     */
    I2SGen4_RK &withBits16() { audioSettings.withBits16(); return *this; };

    /**
     * @brief Sets 24- bit mode. Must be called before start().
     * 
     * @param bits24 Set to true for 24 bit mode (default parameter), or false for 16-bit
     * @return I2SGen4_RK& 
     * 
     * 32-bit mode is not supported.
     */
    I2SGen4_RK &withBits24(bool bits24 = true) { audioSettings.withBits24(bits24); return *this; };

    /**
     * @brief Get the 24 bit flag (true = 24 bits, false = 16 bits)
     * 
     * @return true 
     * @return false 
     */
    bool getBits24(void) const { return audioSettings.getBits24(); };

    /**
     * @brief Get the current audio settings (const, for reading only)
     * 
     * @return const I2SGen4_RK::AudioSettings& 
     */
    const I2SGen4_RK::AudioSettings &getAudioSettings() const { return audioSettings; };

    /**
     * @brief Get the current audio settings (modifiable)
     * 
     * @return I2SGen4_RK::AudioSettings& 
     */
    I2SGen4_RK::AudioSettings &getAudioSettings() { return audioSettings; };

    /**
     * @brief Set the direction (RX, TX, or both). Default is I2SGen4_RK::Direction::RX_TX. Must be called before start().
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
     * @brief Play audio from a BufferStreamable class (such as BufferVector, BufferConst, or I2SGen4_TestSine16_RK). Must be called before start().
     * 
     * @param stream The stream to readFrom using copyPage
     * @param runAsISR Run as ISR. Default value is true; BufferVector. BufferConst, and I2SGen4_TestSine16_RK are all ISR safe.
     * @return I2SGen4_RK& 
     */
    I2SGen4_RK &withFillFromBufferStreamable(BufferStreamable *stream, bool runAsISR = true);    

    /**
     * @brief Set function that is called to fill a buffer with I2S samples to send. Must be called before start().
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
     * @brief Set the fill callback to run at interrupt (ISR) level. Must be called before start().
     * 
     * @param runAsISR 
     * @return I2SGen4_RK& 
     * 
     * By default, the fill callback runs from a thread. By using this call, you can run the callback at the ISR level, which has lower
     * latency, but more restrictions on what you can do.
     */
    I2SGen4_RK &withFillCallbackRunAsISR(bool runAsISR = true) { userFillCallbackRunAsISR = runAsISR; return *this; };

    /**
     * @brief Store audio in a BufferStreamable class (such as BufferVector). Must be called before start().
     * 
     * @param stream The stream to write to using writePage
     * @param runAsISR Run as ISR. Default value is true; BufferVector is ISR safe.
     * @return I2SGen4_RK& 
     */
    I2SGen4_RK &withReceiveToBufferStreamable(BufferStreamable *stream, bool runAsISR = true);    

    /**
     * @brief Set the function to call when data is received by I2S. Must be called before start().
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
     * @brief Set the receive callback to run at interrupt (ISR) level. Must be called before start().
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

    /**
     * @brief Start I2S streaming.
     * 
     * It's best to start it and leave it running, and put any gating in the fill or receive callbacks, rather than try to
     * turn it on and off with any precision.
     */
    void start();

    /**
     * @brief Stop I2S streaming.
     * 
     */
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
     * @brief Worker thread function for transmit mode
     * 
     * There are two threads so each thread can wait on an os_queue, one for transmit and one for receive.
     */
    os_thread_return_t fillThreadFunction(void);

    /**
     * @brief Worker thread function for receive mode
     * 
     * There are two threads so each thread can wait on an os_queue, one for transmit and one for receive.
     */
    os_thread_return_t receiveThreadFunction(void);

    /**
     * @brief Function that is called to fill the buffer to send. This is called from rtl_i2s.c via the API table.
     */
    static void fillCallbackStatic();

    /**
     * @brief Member function called from fillCallbackStatic
     * 
     * This runs at ISR level and determines if the settings are to defer using the queue to the worker thread
     * or run immediately.
     */
    void fillCallback();

    /**
     * @brief Internal function to handle filling the buffer for transmit
     * 
     * This can be called from the ISR (if transmit from ISR is enabled), or from the thread (if deferring)
     */
    void fillCallbackInternal();

    /**
     * @brief Function that is called to process a buffer received
     * 
     * @param buf 
     */
    static void receiveCallbackStatic(void *buf);

    /**
     * @brief Member function called from receiveCallbackStatic
     * 
     * This runs at ISR level and determines if the settings are to defer using the queue to the worker thread
     * or run immediately.
     */
    void receiveCallback(void *buf);

    /**
     * @brief Internal function to handle saving the buffer for receive
     * 
     * This can be called from the ISR (if transmit from ISR is enabled), or from the thread (if deferring)
     */
    void receiveCallbackInternal(void *buf);

    /**
     * @brief User callback to fill a buffer to send
     */
    std::function<void(void *buf, size_t bufSize, size_t sampleCount, size_t bytesPerSample, size_t channelCount)> userFillCallback = 0;

    /**
     * @brief Whether to have the fill callback run at ISR level or not
     */
    bool userFillCallbackRunAsISR = false;

    /**
     * @brief User callback to process a buffer received
     * 
     */
    std::function<void(const void *buf, size_t bufSize, size_t sampleCount, size_t bytesPerSample, size_t channelCount)> userReceiveCallback = 0;

    /**
     * @brief Whether to have the receive callback run at ISR level or not
     */
    bool userReceiveCallbackRunAsISR = false;

    /**
     * @brief Mutex to protect shared resources
     * 
     * This is initialized in setup() so make sure you call the setup() method from the global application setup.
     */
    os_mutex_t mutex = 0;

    /**
     * @brief Currently selected audio settings, see methods like withSampleRate()
     */
    AudioSettings audioSettings; 

    /**
     * @brief Worker thread instance class
     */
    Thread *fillThread = 0;

    /**
     * @brief Queue for filling at non-ISR time
     */
    os_queue_t fillQueue;

    /**
     * @brief Worker thread instance class
     */
    Thread *receiveThread = 0;

    /**
     * @brief Queue for receive buffers received at ISR and deferred to the worker thread.
     */
    os_queue_t receiveQueue;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static I2SGen4_RK *_instance;

};

/**
 * @brief Generate 16-bit sine wave data
 * 
 */
class I2SGen4_TestSine16_RK : public I2SGen4_RK::BufferStreamable {
public:
    I2SGen4_TestSine16_RK();

    virtual ~I2SGen4_TestSine16_RK();

    /**
     * @brief Set the settings from an AudioSettings object (recommended). Call before allocate(),
     * 
     * @param settings 
     * @return I2SGen4_TestSine16_RK& 
     * 
     * You should create an AudioSettings object with your settings, and pass it to both this class and the I2SGen4_RK
     * class so they have the same settings. 
     */
    I2SGen4_TestSine16_RK &withAudioSettings(const I2SGen4_RK::AudioSettings &settings);

    /**
     * @brief Set the number of sample frames. Call before allocate()
     * 
     * @param samplesFramesPerBuffer 
     * @return I2SGen4_TestSine16_RK& 
     * 
     * It's often easier to use withAudioSettings() instead of manually setting the same frames per buffer and withChannelCount.
     * 
     * The formula for sample frames per buffer is:alignas
     * 
     * RTL_I2S_DMA_PAGE_SIZE / bytes per sample / number of channels
     * 
     * Since this class only supports 16-bit mode, bytes per sample is always 2.
     */
    I2SGen4_TestSine16_RK &withSamplesFramesPerBuffer(size_t samplesFramesPerBuffer) { this->samplesFramesPerBuffer = samplesFramesPerBuffer; return *this; };

    /**
     * @brief Sets the channel count. Call before allocate().
     * 
     * @param channelCount 
     * @return I2SGen4_TestSine16_RK& 
     * 
     * It's often easier to use withAudioSettings() instead of manually setting the same frames per buffer and withChannelCount.
     * 
     * Only supported channel counts are 1 (mono) and 2 (stereo).
     */
    I2SGen4_TestSine16_RK &withChannelCount(size_t channelCount) { this->channelCount = channelCount; return *this; };


    /**
     * @brief Allocate a new sine wave sample
     * 
     * @param frequencyHz Frequency in Hz for the sine wave.
     * @param samplesPerSecond Sampling frequency for I2S.
     * @return true for successful allocation or false for an error.
     * 
     * This method should be called to set up the sample and the frequency. It is not
     * ISR safe as it allocates a buffer with the sine wave data in it.
     */
    bool allocate(int frequencyHz, int samplesPerSecond);

    /**
     * @brief Get the next sample. This method is ISR safe.
     * 
     * @return int16_t The sample value
     * 
     * This object's index value is updated so the next time you call this method on an instance
     * of this object, you'll get the next value.
     */
    int16_t getSample();

    /**
     * @brief Get the number of samples in a full cycle of this sine wave
     * 
     * @return size_t Number of samples
     * 
     * This will vary depending on the frequencyHz (the frequency of the sine wave) and
     * samplesPerSecond (the sampling rate for I2S).
     */
    size_t getSampleCount() const { return sampleCount; };

    /**
     * @brief Always returns true so the sine wave is played continuously.
     * 
     * @return true 
     * 
     * This method is safe to call from an ISR. This method is part of the implementation of BufferStreamable.
     */
    virtual bool atEOF() const { return false; };

    /**
     * @brief Copies RTL_I2S_DMA_PAGE_SIZE bytes of data to dest
     * 
     * @param dest Filled in with data 
     * 
     * This method is safe to call from an ISR. This method is part of the implementation of BufferStreamable.
     */
    virtual void copyPage(uint8_t *dest);

protected:
    int16_t *samples = 0; //!< Array of samples, allocated in allocate(), deleted in destructor
    size_t sampleCount = 0; //!< Number of samples in the samples array
    size_t samplesFramesPerBuffer = 0; //!< Number of sample frames per buffer (configuration parameter)
    size_t channelCount = 1; //!< Number of channels (configuration parameter)
    std::atomic<size_t> indexAtomic; //!< Current index into samples, updated on getSample()
};

#endif  /* __I2SGEN4_RK_H */