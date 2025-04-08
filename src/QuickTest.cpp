#include "Particle.h"

#include "I2SGen4_RK.h"
#include "rtl_i2s.h"
#include "SparkFun_WM8960_Arduino_Library.h"

SYSTEM_MODE(SEMI_AUTOMATIC);

// System thread defaults to on in 6.2.0 and later and this line is not required
#ifndef SYSTEM_VERSION_v620
SYSTEM_THREAD(ENABLED);
#endif

SerialLogHandler logHandler(LOG_LEVEL_INFO);

WM8960 codec;

void setup()
{
    waitFor(Serial.isConnected, 10000);
    delay(2000);

    Wire.begin();

    if (codec.begin())
    {
        Log.info("WM8960 initialized");

        // General setup needed
        codec.enableVREF();
        codec.enableVMID();

#if 1
        // DAC -> Speakers (modified version of Example 09)
        // This sort of works with the Waveshare WM8960 on Muon

        // General setup needed
        codec.enableVREF();
        codec.enableVMID();

        // Connect from DAC outputs to output mixer
        codec.enableLD2LO();
        codec.enableRD2RO();

        // Set gainstage between booster mixer and output mixer
        // For this loopback example, we are going to keep these as low as they go
        codec.setLB2LOVOL(WM8960_OUTPUT_MIXER_GAIN_NEG_21DB);
        codec.setRB2ROVOL(WM8960_OUTPUT_MIXER_GAIN_NEG_21DB);

        // Enable output mixers
        codec.enableLOMIX();
        codec.enableROMIX();

        // CLOCK STUFF, These settings will get you 44.1KHz sample rate, and class-d
        // freq at 705.6kHz
        codec.enablePLL(); // Needed for class-d amp clock
        codec.setPLLPRESCALE(WM8960_PLLPRESCALE_DIV_2);
        codec.setSMD(WM8960_PLL_MODE_FRACTIONAL);
        codec.setCLKSEL(WM8960_CLKSEL_PLL);
        codec.setSYSCLKDIV(WM8960_SYSCLK_DIV_BY_2);
        codec.setBCLKDIV(4);
        codec.setDCLKDIV(WM8960_DCLKDIV_16);
        codec.setPLLN(7);
        codec.setPLLK(0x86, 0xC2, 0x26); // PLLK=86C226h
        // codec.setADCDIV(0); // Default is 000 (what we need for 44.1KHz)
        // codec.setDACDIV(0); // Default is 000 (what we need for 44.1KHz)
        codec.setWL(WM8960_WL_16BIT);

        codec.enablePeripheralMode();
        // codec.enableMasterMode();
        // codec.setALRCGPIO(); // Note, should not be changed while ADC is enabled.

        // Enable DACs
        codec.enableDacLeft();
        codec.enableDacRight();

        // codec.enableLoopBack(); // Loopback sends ADC data directly into DAC
        codec.disableLoopBack();

        // Default is "soft mute" on, so we must disable mute to make channels active
        codec.disableDacMute();

        // Volume 0 = 0dB, more negative is lower volume, lowest is -74.00 dB
        codec.enableSpeakers();
        codec.setSpeakerVolumeDB(-20.00);
#endif

#if 0
        // Input 1 -> Speakers (from Example_04)
        // Setup signal flow through the analog audio bypass connections

        codec.enableLMIC();
        codec.enableRMIC();

        // Connect from INPUT1 to "n" (aka inverting) inputs of PGAs.
        codec.connectLMN1();
        codec.connectRMN1();

        // Disable mutes on PGA inputs (aka INTPUT1)
        codec.disableLINMUTE();
        codec.disableRINMUTE();

        // Set input boosts to get inputs 1 to the boost mixers
        codec.setLMICBOOST(WM8960_MIC_BOOST_GAIN_0DB);
        codec.setRMICBOOST(WM8960_MIC_BOOST_GAIN_0DB);

        codec.connectLMIC2B();
        codec.connectRMIC2B();

        // Enable boost mixers
        codec.enableAINL();
        codec.enableAINR();

        // Connect LB2LO (booster to output mixer (analog bypass)
        codec.enableLB2LO();
        codec.enableRB2RO();

        // Set gainstage between booster mixer and output mixer
        codec.setLB2LOVOL(WM8960_OUTPUT_MIXER_GAIN_0DB);
        codec.setRB2ROVOL(WM8960_OUTPUT_MIXER_GAIN_0DB);

        // Enable output mixers
        codec.enableLOMIX();
        codec.enableROMIX();

        // CLOCK STUFF, These settings will get you 44.1KHz sample rate, and class-d
        // freq at 705.6kHz
        codec.enablePLL(); // Needed for class-d amp clock
        codec.setPLLPRESCALE(WM8960_PLLPRESCALE_DIV_2);
        codec.setSMD(WM8960_PLL_MODE_FRACTIONAL);
        codec.setCLKSEL(WM8960_CLKSEL_PLL);
        codec.setSYSCLKDIV(WM8960_SYSCLK_DIV_BY_2);
        codec.setDCLKDIV(WM8960_DCLKDIV_16);
        codec.setPLLN(7);
        codec.setPLLK(0x86, 0xC2, 0x26); // PLLK=86C226h

        codec.enableSpeakers();
        codec.setSpeakerVolumeDB(0.00);
#endif

#if 0
        // Input 2 -> Headphones (from Example_02)
        // Set input boosts to get INPUT2 (both left and right) to the boost mixers
        codec.setLIN2BOOST(WM8960_BOOST_MIXER_GAIN_0DB);
        codec.setRIN2BOOST(WM8960_BOOST_MIXER_GAIN_0DB);

        // Enable input boost mixers
        codec.enableAINL();
        codec.enableAINR();

        // Connect LB2LO (booster to output mixer [aka analog bypass])
        codec.enableLB2LO();
        codec.enableRB2RO();

        // Set gainstage between boost mixer and output mixers (analog bypass)
        codec.setLB2LOVOL(WM8960_OUTPUT_MIXER_GAIN_0DB); 
        codec.setRB2ROVOL(WM8960_OUTPUT_MIXER_GAIN_0DB); 

        // Enable output mixers
        codec.enableLOMIX();
        codec.enableROMIX();

        codec.enableHeadphones();
        codec.enableOUT3MIX(); // Provides VMID as buffer for headphone ground

        Serial.println("Volume set to +0dB");
        codec.setHeadphoneVolumeDB(0.00);

#endif
#if 0
        // Input 2 -> Speakers (combination of examples 02 and 04)
        // Set input boosts to get INPUT2 (both left and right) to the boost mixers
        codec.setLIN2BOOST(WM8 960_BOOST_MIXER_GAIN_0DB);
        codec.setRIN2BOOST(WM8960_BOOST_MIXER_GAIN_0DB);

        // Enable input boost mixers
        codec.enableAINL();
        codec.enableAINR();

        // Connect LB2LO (booster to output mixer [aka analog bypass])
        codec.enableLB2LO();
        codec.enableRB2RO();

        // Set gainstage between boost mixer and output mixers (analog bypass)
        codec.setLB2LOVOL(WM8960_OUTPUT_MIXER_GAIN_0DB); 
        codec.setRB2ROVOL(WM8960_OUTPUT_MIXER_GAIN_0DB); 

        // Enable output mixers
        codec.enableLOMIX();
        codec.enableROMIX();

        codec.enableSpeakers();
        codec.setSpeakerVolumeDB(0.00);
#endif
    }
    else
    {
        Log.error("The device did not respond. Please check wiring.");
    }

    I2SGen4_RK::instance()
        .withSampleRate(16000)
        .withStereo()
        .withDirection(I2SGen4_RK::Direction::TX_ONLY)
        .withBits16()
        .withFillCallback([](void *buf, size_t bufSize, size_t sampleCount, size_t bytesPerSample, size_t channelCount) {
            I2SGen4_Test_RK::generateSample16((int16_t*)buf, sampleCount, channelCount);
        })
        .withFillCallbackRunAsISR()
        .withReceiveCallback([](const void *buf, size_t bufSize, size_t sampleCount, size_t bytesPerSample, size_t channelCount) {
        })
        .setup();

    I2SGen4_RK::instance().start();
}

void loop()
{
    I2SGen4_RK::instance().loop();
}
