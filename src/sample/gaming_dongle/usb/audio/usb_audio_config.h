#ifndef _USB_AUDIO_CONFIG_H_
#define _USB_AUDIO_CONFIG_H_
#include "usb_audio_driver.h"

#define UAC_SPK_SUPPORT             1
#define UAC_MIC_SUPPORT             1
#define UAC_INTR_SUPPORT            0

#if F_APP_USB_HIGH_SPEED_0_5MS
#define USB_AUDIO_VERSION           USB_AUDIO_VERSION_2
#else
#define USB_AUDIO_VERSION           USB_AUDIO_VERSION_1
#endif

#if RTL8763ESE_PRODUCT_SPEAKER
#undef UAC_MIC_SUPPORT
#define UAC_MIC_SUPPORT             0
#endif

#define UAC_SAM_RATE_8000                  8000
#define UAC_SAM_RATE_16000                 16000
#define UAC_SAM_RATE_32000                 32000
#define UAC_SAM_RATE_44100                 44100
#define UAC_SAM_RATE_48000                 48000
#define UAC_SAM_RATE_96000                 96000
#define UAC_SAM_RATE_192000                192000

#if F_APP_USB_AUDIO_96K_24BIT
#define USB_AUDIO_DS_SAMPLE_RATE           UAC_SAM_RATE_96000
#else
#define USB_AUDIO_DS_SAMPLE_RATE           UAC_SAM_RATE_48000
#endif
#define USB_AUDIO_US_SAMPLE_RATE           UAC_SAM_RATE_48000

#define UAC_BIT_RES_16                      16
#define UAC_BIT_RES_24                      24

#define UAC_CHAN_MONO                       1
#define UAC_CHAN_STEREO                     2

#if F_APP_USB_HIGH_SPEED_0_5MS || F_APP_PCM_SPLIT_0_5MS
#define USB_AUDIO_DS_INTERVAL       1
#else
#define USB_AUDIO_DS_INTERVAL       2
#endif
#define USB_AUDIO_US_INTERVAL       4
#define USB_AUDIO_SEC_DS_INTERVAL   2

#if(USB_AUDIO_VERSION == USB_AUDIO_VERSION_2)

#define UAC2_SPK_BIT_RES                    UAC_BIT_RES_16
#define UAC2_MIC_BIT_RES                    UAC_BIT_RES_16

#define UAC2_SPK_CHAN_NUM                   UAC_CHAN_STEREO
#define UAC2_MIC_CHAN_NUM                   UAC_CHAN_MONO

#define UAC2_MIC_SAM_RATE_NUM               1
#define UAC2_SPK_SAM_RATE_NUM               1
#define UAC2_SPK_SAM_RATE_MAX               UAC_SAM_RATE_48000
#define UAC2_MIC_SAM_RATE_MAX               UAC_SAM_RATE_48000

#define UAC2_SPK_VOL_RANGE_NUM              1
#define UAC2_SPK_VOL_CUR                    0x0000
#define UAC2_SPK_VOL_MIN                    0xB300 // -77dB
#define UAC2_SPK_VOL_MAX                    0x0000 // 0 dB
#define UAC2_SPK_VOL_RES                    0x0001

#define UAC2_MIC_VOL_RANGE_NUM              1
#define UAC2_MIC_VOL_CUR                    0x0000
#define UAC2_MIC_VOL_MIN                    0xAD00 // -83dB
#define UAC2_MIC_VOL_MAX                    0x0000 // 0 dB
#define UAC2_MIC_VOL_RES                    0x0001

#else
#define UAC1_BIT_RES_16                      16
#define UAC1_BIT_RES_24                      24

#if F_APP_USB_AUDIO_96K_24BIT
#define UAC1_SPK_BIT_RES                    UAC1_BIT_RES_24
#else
#define UAC1_SPK_BIT_RES                    UAC1_BIT_RES_16
#endif

#if F_APP_USB_GIP_SUPPORT
#define UAC1_SPK_BIT_RES_GIP                UAC1_BIT_RES_16
#endif

#define UAC1_MIC_BIT_RES                    UAC1_BIT_RES_16

#define UAC1_SPK_CHAN_NUM                   UAC_CHAN_STEREO
#define UAC1_MIC_CHAN_NUM                   UAC_CHAN_MONO

// second uac configure
#if ENABLE_UAC2
#define UAC1_SPK_2_BIT_RES                    UAC1_SPK_BIT_RES
#define UAC1_SPK_2_CHAN_NUM                   UAC_CHAN_STEREO
#define UAC1_SPK_2_SAM_RATE_NUM               1
#define UAC1_SPK_2_SAM_RATE_MAX               UAC_SAM_RATE_48000
#define UAC1_SPK_2_UNIT_CTRL_NUM               2

#define UAC1_SPK_2_VOL_CUR                    0x0000
#define UAC1_SPK_2_VOL_MIN                    0xB300 // -77dB
#define UAC1_SPK_2_VOL_MAX                    0x0000 // 0 dB
#define UAC1_SPK_2_VOL_RES                    0x0001

#define UAC1_STREAM_2_INTF_NUM            1


#define UAC_SPK_2_ID_INPUT_TERMINAL               0x04
#define UAC_SPK_2_ID_OUTPUT_TERMINAL              0x07
#define UAC_SPK_2_ID_FEATURE_UNIT                 0x0B
#endif

#if TARGET_RTL8763ESE | TARGET_RTL8763EWM
#define UAC1_SPK_SAM_RATE_NUM               1
#elif TARGET_RTL8763EAU | TARGET_RTL8773DO
#define UAC1_SPK_SAM_RATE_NUM               1
#else
#define UAC1_SPK_SAM_RATE_NUM               3
#endif
#define UAC1_MIC_SAM_RATE_NUM               1

#if F_APP_USB_AUDIO_96K_24BIT
#define UAC1_SPK_SAM_RATE_MAX               UAC_SAM_RATE_96000
#else
#define UAC1_SPK_SAM_RATE_MAX               UAC_SAM_RATE_48000
#endif
#define UAC1_MIC_SAM_RATE_MAX               UAC_SAM_RATE_48000

#if TARGET_RTL8763EAU
#define UAC1_MIC_UNIT_CTRL_NUM               2 // 3
#else
#define UAC1_MIC_UNIT_CTRL_NUM               2
#endif
#define UAC1_SPK_UNIT_CTRL_NUM               2

#define UAC1_SPK_VOL_CUR                    0x0000
#define UAC1_SPK_VOL_MIN                    0xB300 // -77dB
#define UAC1_SPK_VOL_MAX                    0x0000 // 0 dB
#define UAC1_SPK_VOL_RES                    0x0001

#define UAC1_MIC_VOL_CUR                    0x0000
#define UAC1_MIC_VOL_MIN                    0xAD00 // -83dB
#define UAC1_MIC_VOL_MAX                    0x0000 // 0 dB
#define UAC1_MIC_VOL_RES                    0x0100

#if UAC_SPK_SUPPORT & UAC_MIC_SUPPORT
#define UAC1_STREAM_INTF_NUM            2
#else
#define UAC1_STREAM_INTF_NUM            1
#endif
#endif

#if (USB_AUDIO_VERSION == USB_AUDIO_VERSION_2)
#define UAC_SPK_BIT_RES         UAC2_SPK_BIT_RES
#define UAC_MIC_BIT_RES         UAC2_MIC_BIT_RES

#define UAC_SPK_CHAN_NUM        UAC2_SPK_CHAN_NUM
#define UAC_MIC_CHAN_NUM        UAC2_MIC_CHAN_NUM

#define UAC_SPK_VOL_CUR         UAC2_SPK_VOL_CUR
#define UAC_SPK_VOL_MIN         UAC2_SPK_VOL_MIN
#define UAC_SPK_VOL_MAX         UAC2_SPK_VOL_MAX
#define UAC_SPK_VOL_RES         UAC2_SPK_VOL_RES

#define UAC_MIC_VOL_CUR         UAC2_MIC_VOL_CUR
#define UAC_MIC_VOL_MIN         UAC2_MIC_VOL_MIN
#define UAC_MIC_VOL_MAX         UAC2_MIC_VOL_MAX
#define UAC_MIC_VOL_RES         UAC2_MIC_VOL_RES
#else
#define UAC_SPK_BIT_RES         UAC1_SPK_BIT_RES
#define UAC_MIC_BIT_RES         UAC1_MIC_BIT_RES

#define UAC_SPK_CHAN_NUM        UAC1_SPK_CHAN_NUM
#define UAC_MIC_CHAN_NUM        UAC1_MIC_CHAN_NUM

#define UAC_SPK_VOL_CUR         UAC1_SPK_VOL_CUR
#define UAC_SPK_VOL_MIN         UAC1_SPK_VOL_MIN
#define UAC_SPK_VOL_MAX         UAC1_SPK_VOL_MAX
#define UAC_SPK_VOL_RES         UAC1_SPK_VOL_RES

#define UAC_MIC_VOL_CUR         UAC1_MIC_VOL_CUR
#define UAC_MIC_VOL_MIN         UAC1_MIC_VOL_MIN
#define UAC_MIC_VOL_MAX         UAC1_MIC_VOL_MAX
#define UAC_MIC_VOL_RES         UAC1_MIC_VOL_RES
#endif

#if F_APP_USB_AUDIO_FEEDBACK_SUPPORT
#define USB_AUDIO_EXTRA_FEEDBACK_SIZE   80
#else
#define USB_AUDIO_EXTRA_FEEDBACK_SIZE   0
#endif

#define UAC_SPK1_MAX_PACKET_SIZE (((UAC1_SPK_SAM_RATE_MAX / 1000) * (UAC1_SPK_BIT_RES / 8) * UAC1_SPK_CHAN_NUM) + USB_AUDIO_EXTRA_FEEDBACK_SIZE)
#define UAC_SPK2_MAX_PACKET_SIZE (((UAC1_SPK_2_SAM_RATE_MAX / 1000) * (UAC1_SPK_2_BIT_RES / 8) * UAC1_SPK_2_CHAN_NUM) + USB_AUDIO_EXTRA_FEEDBACK_SIZE)


#endif

