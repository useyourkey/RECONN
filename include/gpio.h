#ifndef __GPIOH__
#define __GPIOH__

typedef enum
{
    GPIO_56,    // Apple Auth chip SPI_NSS/Mode2
    GPIO_57,    // WiFi Enable
    GPIO_128,   // Enable 1.8V power for GPS antenna
    GPIO_137,   // DC Input from T5-8 Connectors
    GPIO_140,   // Enable 1.3V power for LNB
    GPIO_141,   // Enable 18V
    GPIO_144,   // LNB 10MHZ
    GPIO_145,   // Charging Disable
    GPIO_146,   // Spectrum Analyzer 10MHZ
    GPIO_147,   // Enable 3.3V GPS power
    GPIO_156,   // Enable 5V Power
    GPIO_157,   // Enable 12V power
    GPIO_158,   // Battery Thermo status
    GPIO_159,   // Enable 3.3V power
    GPIO_161,   // Enable 1.8V for Wifi Power and oscillator
    GPIO_162,   // Power button pressed
    GPIO_171,   // Red status LED
    GPIO_172,   // Main power status LED
    GPIO_173,   // Green status LED
    GPIO_174,   // DMM Power
    GPIO_175    // Apple Authentication chip reset
}GpioNames;

#define APPLE_AUTH_CHIP_GPIO    GPIO_56
#define WIFI_ENABLE_GPIO        GPIO_57
#define GPS_ANTANNAE_GPIO       GPIO_128
#define POWER_12V_INPUT_GPIO    GPIO_137
#define LNB_ENABLE_GPIO         GPIO_140
#define POWER_18V_GPIO          GPIO_141
#define LNB_10MHZ_GPIO          GPIO_144
#define CHARGE_DISABLE_GPIO     GPIO_145
#define SA_10MHZ_GPIO           GPIO_146
#define LNB_18VDC_GPIO          GPIO_147
#define POWER_5V_GPIO           GPIO_156
#define POWER_12V_GPIO          GPIO_157
#define BATTERY_THERMO_GPIO     GPIO_158
#define POWER_3_3V_GPIO         GPIO_159
#define POWER_1_8V_GPIO         GPIO_161
#define POWER_BUTTON_GPIO       GPIO_162
#define BATTERY_LED_RED_GPIO    GPIO_171
#define POWER_LED_GPIO          GPIO_172
#define BATTERY_LED_GREEN_GPIO  GPIO_173
#define DMM_POWER_GPIO          GPIO_174
#define APPLE_AUTH_RESET_GPIO   GPIO_175


typedef enum
{
    DISABLE,
    ENABLE,
    READ
}GpioAction;

extern ReconnErrCodes reconnGpioAction(GpioNames, GpioAction, short *);
#endif
