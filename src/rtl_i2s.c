#include "rtl_i2s.h"

// The I2S API headers conflict with Particle.h headers!
// This file can only contain interface code to the RTL SDK and not anything that interfaces
// to the Particle platform.
#include "i2s_api.h" 
// #include "alc5651.h"


rtl_i2s_api g_rtl_i2s_api = {
	16000, // sampleRateHz
	TRUE, // stereo
	FALSE, // bits24
	I2S_DIR_TXRX, // direction (2)
	FALSE, // use_mclk 
	35, // platform PLATFORM_P2 (32) or PLATFORM_MSOM (35)
	NULL, // fillCallback
	NULL, // receiveCallback
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

// #define SAMPLE_FILE
#define SAMPLE_FILE_RATE 8000//44100
#define SAMPLE_FILE_CHNUM 2




#if defined(SAMPLE_FILE)
// no sample
//	SR_96KHZ,
//	SR_7p35KHZ,
//	SR_29p4KHZ,
//	SR_88p2KHZ
#if SAMPLE_FILE_RATE==8000
	#if SAMPLE_FILE_CHNUM==2
		// #include "birds_8000_2ch_16b.c"
        extern short sample[];
        extern int sample_size;
		#undef SAMPLE_FILE_RATE
		#define SAMPLE_FILE_RATE SR_8KHZ
	#endif
#elif SAMPLE_FILE_RATE==11025
	#if SAMPLE_FILE_CHNUM==2
		#include "birds_11025_2ch_16b.c"
		#undef SAMPLE_FILE_RATE
		#define SAMPLE_FILE_RATE SR_14p7KHZ
	#endif
#elif SAMPLE_FILE_RATE==16000
	#if SAMPLE_FILE_CHNUM==2
		#include "birds_16000_2ch_16b.c"
		#undef SAMPLE_FILE_RATE
		#define SAMPLE_FILE_RATE SR_16KHZ
	#endif
#elif SAMPLE_FILE_RATE==22050
	#if SAMPLE_FILE_CHNUM==2
		#include "birds_22050_2ch_16b.c"
		#undef SAMPLE_FILE_RATE
		#define SAMPLE_FILE_RATE SR_22p05KHZ
	#endif
#elif SAMPLE_FILE_RATE==24000
	#if SAMPLE_FILE_CHNUM==2
		#include "birds_24000_2ch_16b.c"
		#undef SAMPLE_FILE_RATE
		#define SAMPLE_FILE_RATE SR_24KHZ
	#endif
#elif SAMPLE_FILE_RATE==32000
	#if SAMPLE_FILE_CHNUM==2
		#include "birds_32000_2ch_16b.c"
		#undef SAMPLE_FILE_RATE
		#define SAMPLE_FILE_RATE SR_32KHZ
	#endif
#elif SAMPLE_FILE_RATE==44100
	#if SAMPLE_FILE_CHNUM==2
		#include "birds_44100_2ch_16b.c"
		#undef SAMPLE_FILE_RATE
		#define SAMPLE_FILE_RATE SR_44p1KHZ
	#endif
#elif SAMPLE_FILE_RATE==48000
	#if SAMPLE_FILE_CHNUM==2
		#include "birds_48000_2ch_16b.c"
		#undef SAMPLE_FILE_RATE
		#define SAMPLE_FILE_RATE SR_48KHZ
	#endif
#endif

#if SAMPLE_FILE_CHNUM==2
	#undef SAMPLE_FILE_CHNUM
	#define SAMPLE_FILE_CHNUM CH_STEREO
#endif

int curr_cnt=0;
#else

short test_sine16[16]={0, 12539/4, 23170/4, 30273/4, 32767/4, 30273/4, 23170/4, 12539/4,
                  0, -12539/4, -23170/4, -30273/4, -32767/4, -30273/4, -23170/4, -12539/4};
int test_sine24[16]={0, 12539*256/4, 23170*256/4, 30273*256/4, 32767*256/4, 30273*256/4, 23170*256/4, 12539*256/4,
                  0, -12539*256/4, -23170*256/4, -30273*256/4, -32767*256/4, -30273*256/4, -23170*256/4, -12539*256/4};

extern void wait_ms(long);

#include <math.h>
short remap_level_to_signed_16_bit(float val)
{
	val*=32767;
	if(val>32767)	val=32767;
	if(val<-32768)	val=-32768;
	
	return val;
}

void generate_freq_16bit(short *buffer, int count, float freq, float sampling_rate)
{
  int pos; // sample number we're on

  for (pos = 0; pos < count; pos++) {
    float a = 2 * 3.14159f * freq * pos / sampling_rate;
    // convert from [-1.0,1.0] to [-32767,32767]:
    buffer[pos] = remap_level_to_signed_16_bit(a);
  }
}

void gen_sound_sample16(short *buf, int buf_size, int channel_num)
{
	int i;
	for (i = 0 ; i < buf_size ; i+=channel_num){
		buf[i] = test_sine16[(i/channel_num)%16];
		if(channel_num>=2)
        	buf[i+1] = test_sine16[(i/channel_num)%16];
    }
}

void gen_sound_sample24(int *buf, int buf_size, int channel_num)
{
	int i;
	for (i = 0 ; i < buf_size ; i+=channel_num){
		buf[i] = test_sine24[(i/channel_num)%16]&0xFFFFFF;
		if(channel_num>=2)
        	//buf[i+1] = test_sine24[(i/channel_num)%16]&0xFFFFFF;
			buf[i+1] = test_sine24[(i/channel_num)%16]&0xFFFFFF;
    }
}


int test_rate_list[19] = {
	SR_8KHZ,
	SR_12KHZ,
	SR_16KHZ,
	SR_24KHZ,
	SR_32KHZ,
	SR_48KHZ,
	SR_64KHZ,
	SR_96KHZ,
	SR_192KHZ,
	SR_384KHZ,
	SR_7p35KHZ,
	SR_11p025KHZ,
	SR_14p7KHZ,
	SR_22p05KHZ,
	SR_29p4KHZ,
	SR_44p1KHZ,
	SR_58p8KHZ,
	SR_88p2KHZ,
	SR_176p4KHZ
};
#endif

void test_tx_complete(void *data, char *pbuf)
{
    int *ptx_buf;
    
    i2s_t *obj = (i2s_t *)data;

	// int* i2s_get_tx_page(i2s_t *obj);
    ptx_buf = i2s_get_tx_page(obj);
    //ptx_buf = (int*)pbuf;
#if defined(SAMPLE_FILE)	
    _memcpy((void*)ptx_buf, (void*)&sample[curr_cnt], RTL_I2S_DMA_PAGE_SIZE);
	curr_cnt+=(RTL_I2S_DMA_PAGE_SIZE/sizeof(short));
	if(curr_cnt >= sample_size*(obj->channel_num==CH_MONO?1:2)) {
		curr_cnt = 0;
    }
#else
	if(obj->word_length == WL_16b){
		gen_sound_sample16((short*)ptx_buf, RTL_I2S_DMA_PAGE_SIZE/sizeof(short), obj->channel_num==CH_MONO?1:2);
	}else{
		gen_sound_sample24((int*)ptx_buf, RTL_I2S_DMA_PAGE_SIZE/sizeof(int), obj->channel_num==CH_MONO?1:2);
	}
#endif

	// void i2s_send_page(i2s_t *obj, uint32_t *pbuf);
    i2s_send_page(obj, (uint32_t*)ptx_buf);
}

void test_rx_complete(void *data, char* pbuf)
{
    i2s_t *obj = (i2s_t *)data;
    int *ptx_buf;

    //ptx_buf = i2s_get_tx_page(obj);
    //_memcpy((void*)ptx_buf, (void*)pbuf, RTL_I2S_DMA_PAGE_SIZE);

	// void i2s_recv_page(i2s_t *obj);
    i2s_recv_page(obj);    // submit a new page for receive
    //i2s_send_page(obj, (uint32_t*)ptx_buf);    // loopback
}

void runTest(void)
{
    int *ptx_buf;
    int i,j;
	PinName sckPin, wsPin, txPin, rxPin, mckPin;
    
	// alc5651_init();
	// alc5651_init_interface2();	// connect to ALC interface 2
	
	// dump register 
	//alc5651_reg_dump();
	//alc5651_index_dump();

	// I2S init

	// Valid values: CH_MONO, CH_STEREO
	i2s_obj.channel_num = CH_MONO;

	// Valid values: SR_8KHZ, SR_16KHZ, SR_24KHZ, SR_32KHZ, SR_48KHZ, SR_96KHZ, SR_7p35KHZ, SR_14p7KHZ, SR_22p05KHZ, SR_29p4KHZ, SR_44p1KHZ, SR_88p2KHZ
	i2s_obj.sampling_rate = SR_44p1KHZ;

	// Valid values: WL_16b, WL_24b
	i2s_obj.word_length = WL_16b;

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
    i2s_tx_irq_handler(&i2s_obj, (i2s_irq_handler)test_tx_complete, (uint32_t)&i2s_obj);

	// void i2s_rx_irq_handler(i2s_t *obj, i2s_irq_handler handler, uint32_t id);
    i2s_rx_irq_handler(&i2s_obj, (i2s_irq_handler)test_rx_complete, (uint32_t)&i2s_obj);
    
#if defined(SAMPLE_FILE)	
	i2s_set_param(&i2s_obj,SAMPLE_FILE_CHNUM,SAMPLE_FILE_RATE,WL_16b);
    for (i=0;i<RTL_I2S_DMA_PAGE_COUNT;i++) {
        ptx_buf = i2s_get_tx_page(&i2s_obj);
        if (ptx_buf) {
            _memcpy((void*)ptx_buf, (void*)&sample[curr_cnt], RTL_I2S_DMA_PAGE_SIZE);
            i2s_send_page(&i2s_obj, (uint32_t*)ptx_buf);
            curr_cnt+=(RTL_I2S_DMA_PAGE_SIZE/sizeof(short));
            if(curr_cnt >= sample_size*(i2s_obj.channel_num==CH_MONO?1:2)) {
                curr_cnt = 0;
            }
        }
    }
#else
	// output freq, @ sampling rate
	// 6kHz 		@ 96kHz
	// 3kHz 		@ 48kHz
	// 2kHz			@ 32kHz
	// 1.5kHz 		@ 24kHz
	// 1kHz 		@ 16kHz
	// 500Hz 		@ 8kHz
	// 5512.5 Hz 	@ 88200Hz
	// 2756.25 Hz	@ 44100Hz
	// 1837.5 Hz	@ 29400Hz
	// 1378.125 Hz	@ 22050Hz
	// 459.375 Hz	@ 7350Hz

	// Stereo, 16bit
	for(i=0;i<19;i++){
		i2s_set_param(&i2s_obj,CH_STEREO,test_rate_list[i],WL_16b);
        // Start with fill all pages of DMA buffer
        for (j=0;j<RTL_I2S_DMA_PAGE_COUNT;j++) {
            ptx_buf = i2s_get_tx_page(&i2s_obj);
            if (ptx_buf) {
                gen_sound_sample16((short*)ptx_buf, RTL_I2S_DMA_PAGE_SIZE/sizeof(short), 2);
                i2s_send_page(&i2s_obj, (uint32_t*)ptx_buf);
            }
        }
        wait_ms(5000);  // delay 5 sec.
	}

	// Mono, 16bit
	for(i=0;i<19;i++){
		i2s_set_param(&i2s_obj,CH_MONO,test_rate_list[i],WL_16b);
        for (j=0;j<RTL_I2S_DMA_PAGE_COUNT;j++) {
            ptx_buf = i2s_get_tx_page(&i2s_obj);
            if (ptx_buf) {
                gen_sound_sample16((short*)ptx_buf, RTL_I2S_DMA_PAGE_SIZE/sizeof(short), 1);
                i2s_send_page(&i2s_obj, (uint32_t*)ptx_buf);
            }
        }
        wait_ms(5000);  // delay 5 sec.
	}

//	i2s_deinit(&i2s_obj);
	i2s_disable(&i2s_obj);

	// alc5651_set_word_len(2);	
	// alc5651_reg_dump();
    
	i2s_enable(&i2s_obj);
	// Stereo, 24bit
	for(i=0;i<19;i++){
		i2s_set_param(&i2s_obj,CH_STEREO,test_rate_list[i],WL_24b);
        for (j=0;j<RTL_I2S_DMA_PAGE_COUNT;j++) {
            ptx_buf = i2s_get_tx_page(&i2s_obj);
            if (ptx_buf) {
                gen_sound_sample24((int*)ptx_buf, RTL_I2S_DMA_PAGE_SIZE/sizeof(int), 2);
                i2s_send_page(&i2s_obj, (uint32_t*)ptx_buf);
            }
        }
        wait_ms(5000);  // delay 5 sec.
	}
	
	// Not Support Mono, 24bit
	i2s_deinit(&i2s_obj);
#endif
	
	
	// while(1);
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
