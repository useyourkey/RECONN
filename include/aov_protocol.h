#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_


struct aov_header {
	unsigned short			protocol_id;
	unsigned short			protocol_ver;
	unsigned char			header_len;
	unsigned char			flags;
	unsigned short			data_len;
	unsigned short			header_crc;
	unsigned short			data_crc;
};


#define		AVCOM_PROTOCOL_ID		0x1234
#define		AVCOM_PROTOCOL_VER_MAJ	0x00
#define		AVCOM_PROTOCOL_VER_MIN	0x01
#define		AVCOM_PROTOCOL_VERSION	((AVCOM_PROTOCOL_VER_MAJ << 8) | AVCOM_PROTOCOL_VER_MIN)
#define		AVCOM_HEADER_LENGTH		12

#define		API_ACK					0x0001
#define		API_FLASH_MAIN_START	0x0002
#define		API_FILE_BLOCK			0x0003
#define		API_RECV_START			0x0004
#define		API_ENG_SET_DAC			0x0005
#define		API_ENG_SET_20DB		0x0006
#define		API_ENG_SET_DSA			0x0007
#define		API_ENG_SET_IF			0x0008
#define		API_ENG_SET_RBW_MUX		0x0009
#define		API_ENG_SET_VBW			0x000A
#define		API_ENG_SET_UTUNE_LO	0x000B
#define		API_ENG_SET_PLLSSS		0x000C
#define		API_ENG_SET_PLL_WAIT	0x000D
#define		API_ENG_SET_DDSSSS		0x000E
#define		API_ENG_SET_DDS_WAIT	0x000F
#define		API_ENG_TRG_PLL_SWEEP	0x0010
#define		API_ENG_TRG_DDS_SWEEP	0x0011
#define		API_ENG_GET_SWEEP_DATA	0x0012
#define		API_ENG_SET_MAC_HOST	0x0013
#define		API_ENG_GET_TEMP		0x0014
#define		API_FLASH_BOOT_START	0x0015
#define		API_FLASH_INIT_START	0x0016
#define		API_DISCOVERY_HELLO		0x0017
#define		API_DISCOVERY_ECHO		0x0018
#define		API_CHANGE_HOSTNAME		0x0019
#define		API_CHANGE_NETWORK		0x001A
#define		API_CHANGE_PORT			0x001B
#define		API_BOOT_BOOTLOADER		0x001C
#define		API_REBOOT				0x001D

#define		API_ENG_SET_HW_DESC		0x0020
#define		API_ENG_GET_HW_DESC		0x0021
#define		API_SET_INPUT_LIST		0x0022
#define		API_GET_INPUT_LIST		0x0023
#define		API_SET_BAND_LIST		0x0024
#define		API_GET_BAND_INFO		0x0025
#define		API_SET_RBW_LIST		0x0026
#define		API_GET_RBW_LIST		0x0027
#define		API_SET_VBW_LIST		0x0028
#define		API_GET_VBW_LIST		0x0029
#define		API_SET_FREQ_SSS		0x0030
#define		API_SET_FREQ_CSS		0x0031
#define		API_SET_INPUT			0x0032

#define		API_BUILD_DATETIME		0x0033
#define		API_UARTLOADER_VERSION	0x0034
#define		API_BOOTLOADER_VERSION	0x0035
#define		API_BASE_VERSION		0x0036

#define		API_SET_BAND			0x0037
#define		API_SET_REF_LEVEL		0x0038
#define		API_SET_RBW				0x0039
#define		API_SET_VBW				0x003A
#define		API_GET_SWEEP_DATA		0x003B
#define		API_GET_ACTIVE_RBW		0x003C
#define		API_GET_ACTIVE_VBW		0x003D

#define		API_SET_NUM_BANDS		0x003F
#define		API_SET_BAND_INFO		0x0040
#define		API_GET_ACTIVE_BAND		0x0041

#define		API_TRG_SWEEP			0x0042
#define		API_TRG_GET_SWEEP		0x0043
#define		API_GET_PROD_DESC		0x0044
#define		API_GET_NETWORK			0x0045
#define		API_GET_TRG_SWEEP_ARRAY	0x0046
#define		API_GET_SWEEP_ARRAY		0x0047
#define		API_PING				0x0048
#define		API_SET_FREQ_CSD		0x0049
#define		API_ENG_SET_PLLFREQ		0x004A
#define		API_ENG_SET_DDSFREQ		0x004B
#define		API_ENG_GET_ADC			0x004C
#define		API_ENG_SET_PLLLOCK		0x004D
#define		API_ENG_CAL_RBW			0x004E

#define		API_ENG_PLL_LDP			0x004F
#define		API_ENG_GLBL_WAIT		0x0050
#define		API_ENG_PLL_EN_LD		0x0051
#define		API_ENG_GET_FREQ		0x0052
#define		API_SET_REFLVL			0x0053
#define		API_GET_REFLVL			0x0054
#define		API_ENG_CAL_20DB		0x0055
#define		API_SAVE_BANDSETTINGS	0x0056
#define		API_SET_BAND_NAME		0x0057
#define		API_SET_ANALYZER_NAME	0x0058

#define		API_ENG_SET_MODEL		0x0059
#define		API_ENG_SET_PROD_ID		0x005A
#define		API_ENG_SET_SERIAL		0x005B

#define		API_TRIG_ZERO_SPAN		0x005C
#define		API_TRIGGET_ZERO_SPAN	0x005D
#define		API_SET_ZERO_SPAN		0x005E
#define		API_GET_ZERO_SPAN		0x005F

#define		API_ENG_CAL_ATTN		0x0060
#define		API_ENG_CAL_DSA			0x0061
#define		API_ENG_GET_CAL_20DB	0x0062
#define		API_ENG_GET_CAL_DSA		0x0063

#endif /* _PROTOCOL_H_ */
