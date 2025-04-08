#include "rtl_i2s.h"

// The I2S API headers conflict with Particle.h headers!
// This file can only contain interface code to the RTL SDK and not anything that interfaces
// to the Particle platform.
#include "i2s_api.h" 
// #include "alc5651.h"


static void rtl_i2s_init();
static void rtl_i2s_deinit();
static void *rtl_i2s_getTxPage();
static void rtl_i2s_sendTxPage(void *buf);
static void rtl_i2s_returnRecvPage();


rtl_i2s_api g_rtl_i2s_api = {
	16000, // sampleRateHz
	TRUE, // stereo
	FALSE, // bits24
	I2S_DIR_TXRX, // direction (2)
	FALSE, // use_mclk 
	35, // platform PLATFORM_P2 (32) or PLATFORM_MSOM (35)
	NULL, // fillCallback
	NULL, // receiveCallback
	rtl_i2s_init,
	rtl_i2s_deinit,
	rtl_i2s_getTxPage,
	rtl_i2s_sendTxPage,
	rtl_i2s_returnRecvPage,
};


i2s_t i2s_obj;


//The size of this buffer should be multiples of 32 and its head address should align to 32 
//to prevent problems that may occur when CPU and DMA access this area simultaneously. 
// I wonder if this needs to be SRAM_NOCACHE_DATA_SECTION? It is in some examples, but I get an overflow when I use it:
// /Users/rickk/.particle/toolchains/gcc-arm/10.2.1/bin/../lib/gcc/arm-none-eabi/10.2.1/../../../../arm-none-eabi/bin/ld: /Users/rickk/Documents/src/Mine/I2SGen4_RK/target/6.3.0/msom/I2SGen4_RK_fi.elf section `.bdsram.data' will not fit in region `SRAM'
// /Users/rickk/.particle/toolchains/gcc-arm/10.2.1/bin/../lib/gcc/arm-none-eabi/10.2.1/../../../../arm-none-eabi/bin/ld: section .backup VMA [000000001007b400,000000001007b403] overlaps section .bdsram.data VMA [000000001007ad40,000000001007b53f]
// /Users/rickk/.particle/toolchains/gcc-arm/10.2.1/bin/../lib/gcc/arm-none-eabi/10.2.1/../../../../arm-none-eabi/bin/ld: region `SRAM' overflowed by 2064 bytes
static u8 i2s_tx_buf[RTL_I2S_DMA_PAGE_SIZE*RTL_I2S_DMA_PAGE_COUNT]__attribute__((aligned(32)));
static u8 i2s_rx_buf[RTL_I2S_DMA_PAGE_SIZE*RTL_I2S_DMA_PAGE_COUNT]__attribute__((aligned(32)));


static void rtl_i2s_fill_buffer(void *data, char *pbuf)
{
	g_rtl_i2s_api.fillCallback();
}

static void rtl_i2s_receive_buffer(void *data, char* pbuf)
{
    // i2s_t *obj = (i2s_t *)data;

	g_rtl_i2s_api.receiveCallback(pbuf);
}

void rtl_i2s_init() {
    int i;
	PinName sckPin, wsPin, txPin, rxPin, mckPin;
    
	// Valid values: CH_MONO, CH_STEREO
	i2s_obj.channel_num = g_rtl_i2s_api.stereo ? CH_STEREO : CH_MONO;

	// Valid values: SR_8KHZ, SR_16KHZ, SR_24KHZ, SR_32KHZ, SR_48KHZ, SR_96KHZ, SR_7p35KHZ, SR_14p7KHZ, SR_22p05KHZ, SR_29p4KHZ, SR_44p1KHZ, SR_88p2KHZ
	i2s_obj.sampling_rate = rtl_i2s_mapSampleRate(g_rtl_i2s_api.sampleRateHz);

	// Valid values: WL_16b, WL_24b
	i2s_obj.word_length = g_rtl_i2s_api.bits24 ? WL_24b : WL_16b;

	// Valid directions:
	// I2S_DIR_RX (0), I2S_DIR_TX (1), I2S_DIR_TXRX (2)
	i2s_obj.direction = g_rtl_i2s_api.direction;    

	if (g_rtl_i2s_api.platform == 32) {
		// PLATFORM_P2 (32)

		// P2 only (these are not exposed on Photon 2)
		sckPin = PB_29;
		wsPin = PB_31;
		txPin = PB_26;
		rxPin = PA_0;
		mckPin = PA_12;
	}
	else {
		// PLATFORM_MSOM (35)
		sckPin = PB_20;
		wsPin = PA_4;
		txPin = PA_1;
		rxPin = PA_0;
		mckPin = PA_12;
	}

	if (g_rtl_i2s_api.direction == I2S_DIR_TX) {
		// TX only
		rxPin = NC;
	}
	if (g_rtl_i2s_api.direction == I2S_DIR_RX) {
		// RX only
		txPin = NC;
	}
	if (!g_rtl_i2s_api.use_mclk) {
		mckPin = NC;
	}


	// void i2s_init(i2s_t *obj, PinName sck, PinName ws, PinName sd_tx, PinName sd_rx, PinName mck);
	// PinName is an enum of constants like PA_0
	i2s_init(&i2s_obj, sckPin, wsPin, txPin, rxPin, mckPin);

	// void i2s_set_dma_buffer(i2s_t *obj, char *tx_buf, char *rx_buf, uint32_t page_num, uint32_t page_size);
    i2s_set_dma_buffer(&i2s_obj, (char*)i2s_tx_buf, (char*)i2s_rx_buf, \
        RTL_I2S_DMA_PAGE_COUNT, RTL_I2S_DMA_PAGE_SIZE);

	// void i2s_tx_irq_handler(i2s_t *obj, i2s_irq_handler handler, uint32_t id);
    i2s_tx_irq_handler(&i2s_obj, (i2s_irq_handler)rtl_i2s_fill_buffer, (uint32_t)&i2s_obj);

	// void i2s_rx_irq_handler(i2s_t *obj, i2s_irq_handler handler, uint32_t id);
    i2s_rx_irq_handler(&i2s_obj, (i2s_irq_handler)rtl_i2s_receive_buffer, (uint32_t)&i2s_obj);
    
    for (i=0;i<RTL_I2S_DMA_PAGE_COUNT;i++) {
		g_rtl_i2s_api.fillCallback();
	}

}
void rtl_i2s_deinit() {
	i2s_deinit(&i2s_obj);
}

void *rtl_i2s_getTxPage() {
	// int* i2s_get_tx_page(i2s_t *obj);
    return (void *) i2s_get_tx_page(&i2s_obj);
}

void rtl_i2s_sendTxPage(void *buf) {
	i2s_send_page(&i2s_obj, (uint32_t *)buf);
}

void rtl_i2s_returnRecvPage() {
	// void i2s_recv_page(i2s_t *obj);
	i2s_recv_page(&i2s_obj);
}


int rtl_i2s_mapSampleRate(int rateHz) {
	switch(rateHz) {
		case 8000:
			return SR_8KHZ;

		case 16000:
			return SR_16KHZ;
		
		case 24000:
			return SR_24KHZ;

		case 32000:
			return SR_32KHZ;

		case 48000:
			return SR_48KHZ;

		case 96000:
			return SR_96KHZ;

		case 44100:
			return SR_44p1KHZ;			

		default:
			return -1;
	}

}
