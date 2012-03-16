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
    GPIO_147,   // Enable 3.3V GPS power
    GPIO_146,   // Spectrum Analyzer 10MHZ
    GPIO_156,   // Enable 5V Power
    GPIO_157,   // Enable 12V power
    GPIO_158,   // Battery Thermo status
    GPIO_159,   // Enable 3.3V power
    GPIO_161,   // Enable 1.8V for Wifi Power and oscillator
    GPIO_162,   // Power button pressed
    GPIO_171,   // Green status LED
    GPIO_172,   // Main power status LED
    GPIO_173,   // Red status LED
    GPIO_174,   // DMM Power
    GPIO_175    // Apple Authentication chip reset
}GpioNames;

typedef enum
{
    DISABLE,
    ENABLE
}GpioAction;

extern ReconnErrCodes reconnGpioAction(GpioNames, GpioAction);
#endif
