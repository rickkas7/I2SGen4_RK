#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// 768 1536 3072
// This is the size in bytes, and must be an integral number of samples (which is why this is 768 or 1536 instead of 512 in case you are using 24 bit samples)
#define RTL_I2S_DMA_PAGE_SIZE	1536   // 2 ~ 4096
#define RTL_I2S_DMA_PAGE_COUNT    4   // Vaild number is 2~4

typedef void (*rtl_i2s_callback)(void *buf);

/**
 * @brief Structure to pass data between rtl_i2s.c (C) and I2SGen4_RK (C++)
 * 
 * The rtl_i2s file can't include Particle.h, and I2SGen4_RK can't include
 * RTL I2S APIs, so this structure allows data to be passed between the two
 * using generic C data types.
 */
typedef struct {
    /**
     * @brief Sample rate in Hz. Default is 16000
     * Valid values include: 8000, 16000, 24000, 32000, 48000, 96000, 44100.
     */
    int sampleRateHz;

    /**
     * @brief Stereo (true, the default), or mono (false)
     */
    int stereo;

    /**
     * @brief 24-bit mode (true), or 16-bit mode (false, the default)
     */
    int bits24;

    /**
     * @brief Direction
     * 
     * Value values include: I2S_DIR_RX (0), I2S_DIR_TX (1), I2S_DIR_TXRX (2)
     */
    int direction;

    /**
     * @brief Use the MCLK pin. Default is false. This is normally not needed.
     */
    int use_mclk;

    /**
     * @brief PLATFORM_P2 (32) or PLATFORM_MSOM (35)
     */
    int platform;

    /**
     * @brief Callback to fill a buffer to transmit
     * 
     */
    void (*fillCallback)();
    
    /**
     * @brief Callback to process a received buffer
     * 
     */
    void (*receiveCallback)(void *buf);


    /**
     * @brief Function to initialize with the current settings
     */
    void (*init)();

    /**
     * @brief Function to deinitialize
     */
    void (*deinit)();

    /**
     * @brief Get a page buffer to fill in prior to calling sendTxPage
     */
    void * (*getTxPage)();

    /**
     * @brief Send a page that you fill in after calling getTxPage
     * 
     * @param buf The buffer, must come from getTxPage, not an arbitrary buffer
     */
    void (*sendTxPage)(void *buf);

    /**
     * @brief Returns the buffer passed to the receive callback. Required!
     */
    void (*returnRecvPage)();

} rtl_i2s_api;

extern rtl_i2s_api g_rtl_i2s_api;


/**
 * @brief Map a floating point value like 8000 or 44100 into a RTL SDK sample rate code (SR_8KHZ, SR_44p1KHZ)
 * 
 * @param rate 
 * @return int Sample rate code or -1 for unknown sample rate
 * 
 * Value values include: 8000, 16000, 24000, 32000, 48000, 96000, 44100
 */
int rtl_i2s_mapSampleRate(int rateHz);




void runTest(void);

#ifdef __cplusplus
}
#endif