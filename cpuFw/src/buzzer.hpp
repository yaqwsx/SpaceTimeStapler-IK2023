// Based on https://github.com/krzychb/dac-cosine/blob/master/main/dac-cosine.c

extern "C" {
    #include "soc/rtc_io_reg.h"
    #include "soc/rtc_cntl_reg.h"
    #include "soc/sens_reg.h"
    #include "soc/rtc.h"
};
#include <driver/dac.h>


struct Buzzer {

    void start() {

    }

    void beep( dac_channel_t channel, int frequency, int volume ) {
        _dacCosineEnable( channel );
        dac_output_enable( channel );
        _dacScaleSet( channel, volume );
        _dacFrequencySet( 0b111, frequency * 65535 / 1000000 );
    }

    void stop( dac_channel_t channel ) {
        dac_output_voltage( channel, 0 );
    }

    static void _dacCosineEnable( dac_channel_t channel ) {
        // Enable tone generator common to both channels
        SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL1_REG, SENS_SW_TONE_EN);
        switch(channel) {
            case DAC_CHANNEL_1:
                // Enable / connect tone tone generator on / to this channel
                SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN1_M);
                // Invert MSB, otherwise part of waveform will have inverted
                SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_INV1, 2, SENS_DAC_INV1_S);
                break;
            case DAC_CHANNEL_2:
                SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN2_M);
                SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_INV2, 2, SENS_DAC_INV2_S);
                break;
            default:
                break;
        }
    }

    static void _dacFrequencySet( int clk_8m_div, int frequency_step )  {
        REG_SET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_CK8M_DIV_SEL, clk_8m_div);
        SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL1_REG, SENS_SW_FSTEP, frequency_step, SENS_SW_FSTEP_S);
    }

    static void _dacScaleSet( dac_channel_t channel, int scale ) {
        switch(channel) {
            case DAC_CHANNEL_1:
                SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_SCALE1, scale, SENS_DAC_SCALE1_S);
                break;
            case DAC_CHANNEL_2:
                SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_SCALE2, scale, SENS_DAC_SCALE2_S);
                break;
            default:
                break;
        }
    }

    static void _dacOffsetSet( dac_channel_t channel, int offset )
    {
        switch(channel) {
            case DAC_CHANNEL_1:
                SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_DC1, offset, SENS_DAC_DC1_S);
                break;
            case DAC_CHANNEL_2:
                SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_DC2, offset, SENS_DAC_DC2_S);
                break;
            default:
                break;
        }
    }

    static void _dacInvertSet( dac_channel_t channel, int invert ) {
        switch(channel) {
            case DAC_CHANNEL_1:
                SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_INV1, invert, SENS_DAC_INV1_S);
                break;
            case DAC_CHANNEL_2:
                SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_INV2, invert, SENS_DAC_INV2_S);
                break;
            default:
                break;
        }
    }
};