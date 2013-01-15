/*
 * ambhw/gpio.h
 *
 * History:
 *	2006/12/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2006-2008, Ambarella, Inc.
 */

#ifndef __AMBHW__GPIO_H__
#define __AMBHW__GPIO_H__

#include <ambhw/chip.h>
#include <ambhw/busaddr.h>

/****************************************************/
/* Capabilities based on chip revision              */
/****************************************************/
#if 	(CHIP_REV == A5) || (CHIP_REV == A6) || (CHIP_REV == A7L)
#define GPIO_INSTANCES	4
#elif 	(CHIP_REV == A2) || (CHIP_REV == A3) || 		\
	(CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q) ||	\
	(CHIP_REV == A5S) || (CHIP_REV == A5L)
#define GPIO_INSTANCES	3
#elif 	(CHIP_REV == A7)
#define GPIO_INSTANCES	5
#elif 	(CHIP_REV == I1)
#define GPIO_INSTANCES	6
#else
#define GPIO_INSTANCES	2
#endif

#if 	(CHIP_REV == A1)
#define GPIO_MAX_LINES			58
#elif 	(CHIP_REV == A2) || (CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q)
#define GPIO_MAX_LINES			81
#elif 	(CHIP_REV == A3) || (CHIP_REV == A5S)
#define GPIO_MAX_LINES			96
#elif	(CHIP_REV == A5L)
#define GPIO_MAX_LINES			88
#elif 	(CHIP_REV == A5) || (CHIP_REV == A6)
#define GPIO_MAX_LINES			128
#elif 	(CHIP_REV == A7)
#define GPIO_MAX_LINES			144
#elif 	(CHIP_REV == I1)
#define GPIO_MAX_LINES			192
#else
#define GPIO_MAX_LINES			128
#endif

#if 	(CHIP_REV == A2)
#define GPIO_BWO_A2			1
#define GPIO_UNDEFINED_59_63		1
#else
#define GPIO_BWO_A2			0
#define GPIO_UNDEFINED_59_63		0
#endif

/* SW definitions */
#define GPIO_HIGH			1
#define GPIO_LOW			0

/* GPIO function selection */
/* Select SW or HW control and input/output direction of S/W function */
#define GPIO_FUNC_SW_INPUT		0
#define GPIO_FUNC_SW_OUTPUT		1
#define GPIO_FUNC_HW			2

/* GPIO 50~53 INTERNAL MUXER (A5S)*/
#define ENABLE_STEPPERC			1
#define ENABLE_SPI_SLAVE		2
#define ENABLE_UART1			3

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/

#define GPIO_DATA_OFFSET		0x00
#define GPIO_DIR_OFFSET			0x04
#define GPIO_IS_OFFSET			0x08
#define GPIO_IBE_OFFSET			0x0c
#define GPIO_IEV_OFFSET			0x10
#define GPIO_IE_OFFSET			0x14
#define GPIO_AFSEL_OFFSET		0x18
#define GPIO_RIS_OFFSET			0x1c
#define GPIO_MIS_OFFSET			0x20
#define GPIO_IC_OFFSET			0x24
#define GPIO_MASK_OFFSET		0x28
#define GPIO_ENABLE_OFFSET		0x2c

#define GPIO0_DATA_REG			GPIO0_REG(GPIO_DATA_OFFSET)
#define GPIO0_DIR_REG			GPIO0_REG(GPIO_DIR_OFFSET)
#define GPIO0_IS_REG			GPIO0_REG(GPIO_IS_OFFSET)
#define GPIO0_IBE_REG			GPIO0_REG(GPIO_IBE_OFFSET)
#define GPIO0_IEV_REG			GPIO0_REG(GPIO_IEV_OFFSET)
#define GPIO0_IE_REG			GPIO0_REG(GPIO_IE_OFFSET)
#define GPIO0_AFSEL_REG			GPIO0_REG(GPIO_AFSEL_OFFSET)
#define GPIO0_RIS_REG			GPIO0_REG(GPIO_RIS_OFFSET)
#define GPIO0_MIS_REG			GPIO0_REG(GPIO_MIS_OFFSET)
#define GPIO0_IC_REG			GPIO0_REG(GPIO_IC_OFFSET)
#define GPIO0_MASK_REG			GPIO0_REG(GPIO_MASK_OFFSET)
#define GPIO0_ENABLE_REG		GPIO0_REG(GPIO_ENABLE_OFFSET)

#define GPIO1_DATA_REG			GPIO1_REG(GPIO_DATA_OFFSET)
#define GPIO1_DIR_REG			GPIO1_REG(GPIO_DIR_OFFSET)
#define GPIO1_IS_REG			GPIO1_REG(GPIO_IS_OFFSET)
#define GPIO1_IBE_REG			GPIO1_REG(GPIO_IBE_OFFSET)
#define GPIO1_IEV_REG			GPIO1_REG(GPIO_IEV_OFFSET)
#define GPIO1_IE_REG			GPIO1_REG(GPIO_IE_OFFSET)
#define GPIO1_AFSEL_REG			GPIO1_REG(GPIO_AFSEL_OFFSET)
#define GPIO1_RIS_REG			GPIO1_REG(GPIO_RIS_OFFSET)
#define GPIO1_MIS_REG			GPIO1_REG(GPIO_MIS_OFFSET)
#define GPIO1_IC_REG			GPIO1_REG(GPIO_IC_OFFSET)
#define GPIO1_MASK_REG			GPIO1_REG(GPIO_MASK_OFFSET)
#define GPIO1_ENABLE_REG		GPIO1_REG(GPIO_ENABLE_OFFSET)

#if	(GPIO_INSTANCES >= 3)
#define GPIO2_DATA_REG			GPIO2_REG(GPIO_DATA_OFFSET)
#define GPIO2_DIR_REG			GPIO2_REG(GPIO_DIR_OFFSET)
#define GPIO2_IS_REG			GPIO2_REG(GPIO_IS_OFFSET)
#define GPIO2_IBE_REG			GPIO2_REG(GPIO_IBE_OFFSET)
#define GPIO2_IEV_REG			GPIO2_REG(GPIO_IEV_OFFSET)
#define GPIO2_IE_REG			GPIO2_REG(GPIO_IE_OFFSET)
#define GPIO2_AFSEL_REG			GPIO2_REG(GPIO_AFSEL_OFFSET)
#define GPIO2_RIS_REG			GPIO2_REG(GPIO_RIS_OFFSET)
#define GPIO2_MIS_REG			GPIO2_REG(GPIO_MIS_OFFSET)
#define GPIO2_IC_REG			GPIO2_REG(GPIO_IC_OFFSET)
#define GPIO2_MASK_REG			GPIO2_REG(GPIO_MASK_OFFSET)
#define GPIO2_ENABLE_REG		GPIO2_REG(GPIO_ENABLE_OFFSET)
#endif

#if 	(GPIO_INSTANCES >= 4)
#define GPIO3_DATA_REG			GPIO3_REG(GPIO_DATA_OFFSET)
#define GPIO3_DIR_REG			GPIO3_REG(GPIO_DIR_OFFSET)
#define GPIO3_IS_REG			GPIO3_REG(GPIO_IS_OFFSET)
#define GPIO3_IBE_REG			GPIO3_REG(GPIO_IBE_OFFSET)
#define GPIO3_IEV_REG			GPIO3_REG(GPIO_IEV_OFFSET)
#define GPIO3_IE_REG			GPIO3_REG(GPIO_IE_OFFSET)
#define GPIO3_AFSEL_REG			GPIO3_REG(GPIO_AFSEL_OFFSET)
#define GPIO3_RIS_REG			GPIO3_REG(GPIO_RIS_OFFSET)
#define GPIO3_MIS_REG			GPIO3_REG(GPIO_MIS_OFFSET)
#define GPIO3_IC_REG			GPIO3_REG(GPIO_IC_OFFSET)
#define GPIO3_MASK_REG			GPIO3_REG(GPIO_MASK_OFFSET)
#define GPIO3_ENABLE_REG		GPIO3_REG(GPIO_ENABLE_OFFSET)
#endif

#if 	(GPIO_INSTANCES >= 5)
#define GPIO4_DATA_REG			GPIO4_REG(GPIO_DATA_OFFSET)
#define GPIO4_DIR_REG			GPIO4_REG(GPIO_DIR_OFFSET)
#define GPIO4_IS_REG			GPIO4_REG(GPIO_IS_OFFSET)
#define GPIO4_IBE_REG			GPIO4_REG(GPIO_IBE_OFFSET)
#define GPIO4_IEV_REG			GPIO4_REG(GPIO_IEV_OFFSET)
#define GPIO4_IE_REG			GPIO4_REG(GPIO_IE_OFFSET)
#define GPIO4_AFSEL_REG			GPIO4_REG(GPIO_AFSEL_OFFSET)
#define GPIO4_RIS_REG			GPIO4_REG(GPIO_RIS_OFFSET)
#define GPIO4_MIS_REG			GPIO4_REG(GPIO_MIS_OFFSET)
#define GPIO4_IC_REG			GPIO4_REG(GPIO_IC_OFFSET)
#define GPIO4_MASK_REG			GPIO4_REG(GPIO_MASK_OFFSET)
#define GPIO4_ENABLE_REG		GPIO4_REG(GPIO_ENABLE_OFFSET)
#endif

#if 	(GPIO_INSTANCES >= 6)
#define GPIO5_DATA_REG			GPIO5_REG(GPIO_DATA_OFFSET)
#define GPIO5_DIR_REG			GPIO5_REG(GPIO_DIR_OFFSET)
#define GPIO5_IS_REG			GPIO5_REG(GPIO_IS_OFFSET)
#define GPIO5_IBE_REG			GPIO5_REG(GPIO_IBE_OFFSET)
#define GPIO5_IEV_REG			GPIO5_REG(GPIO_IEV_OFFSET)
#define GPIO5_IE_REG			GPIO5_REG(GPIO_IE_OFFSET)
#define GPIO5_AFSEL_REG			GPIO5_REG(GPIO_AFSEL_OFFSET)
#define GPIO5_RIS_REG			GPIO5_REG(GPIO_RIS_OFFSET)
#define GPIO5_MIS_REG			GPIO5_REG(GPIO_MIS_OFFSET)
#define GPIO5_IC_REG			GPIO5_REG(GPIO_IC_OFFSET)
#define GPIO5_MASK_REG			GPIO5_REG(GPIO_MASK_OFFSET)
#define GPIO5_ENABLE_REG		GPIO5_REG(GPIO_ENABLE_OFFSET)
#endif

/************************/
/* GPIO pins definition */
/************************/
#define GPIO(x)		(x)
#define IDC_CLK		GPIO(0)
#define IDC_DATA	GPIO(1)
#define SSI0_CLK	GPIO(2)
#define SSI0_MOSI	GPIO(3)
#define SSI0_MISO	GPIO(4)
#define SSI0_EN0	GPIO(5)
#define SSI0_EN1	GPIO(6)
#define VD_HVLD		GPIO(7)
#define VD_VVLD		GPIO(8)
#define STRIG0		GPIO(9)
#define STRIG1		GPIO(10)
#define TIMER0		GPIO(11)
#define TIMER1		GPIO(12)
#define TIMER2		GPIO(13)
#define UART0_TX	GPIO(14)
#define UART0_RX	GPIO(15)
#define VD_PWM0		GPIO(16)
/* There is no GPIO(17) */
#define VD_OUT6		GPIO(18)
#define VD_OUT7		GPIO(19)
#define VD_OUT8		GPIO(20)
#define VD_OUT9		GPIO(21)
#define VD_OUT10	GPIO(22)
#define VD_OUT11	GPIO(23)
#define VD_OUT12	GPIO(24)
#define VD_OUT13	GPIO(25)
#define VD_OUT14	GPIO(26)
#define VD_OUT15	GPIO(27)
#define SD12		GPIO(28)
#define SD13		GPIO(29)
#define SD14		GPIO(30)
#define SD15		GPIO(31)
#define CF_CD2		GPIO(32)
#define VD_SPL		GPIO(33)
#define VD_VCOMAC	GPIO(34)
#define IR_IN		GPIO(35)
#define STSCHG		GPIO(36)
#define CF_PULL_CTL	GPIO(37)
#define CF_PWRCYC	GPIO(38)
#define FL_WP		GPIO(39)
#define SC_A0		GPIO(40)
#define SC_A1		GPIO(41)
#define SC_A2		GPIO(42)
#define SC_A3		GPIO(43)
#define SC_A4		GPIO(44)
#define SC_B0		GPIO(45)
#define SC_B1		GPIO(46)
#define SC_B2		GPIO(47)
#define SC_B3		GPIO(48)
#define SC_B4		GPIO(49)
#define SC_C0		GPIO(50)
#define SC_C1		GPIO(51)
#define SC_C2		GPIO(52)
#define SC_C3		GPIO(53)
#define SC_C4		GPIO(54)
#define SD16		GPIO(55)
#define SD17		GPIO(56)
#define SD18		GPIO(57)
#define SD19		GPIO(58)

/* Tertiary function of GPIO pins */
#define A17		GPIO(18)
#define A18		GPIO(19)
#define A19		GPIO(20)
#define A20		GPIO(21)
#define A21		GPIO(22)
#define A22		GPIO(23)
#define I2S_SO_2	GPIO(40)
#define I2S_SI_2	GPIO(41)
#define I2S_WS_2	GPIO(42)
#define I2S_CLK_2	GPIO(43)
#define PWM1		GPIO(45)
#define PWM2		GPIO(46)
#define NAND_CE1	GPIO(47)
#define SSIO_EN2	GPIO(48)
#define SSIO_EN3	GPIO(49)
#if (CHIP_REV == A5L)
#define PWM3		GPIO(40)
#define PWM4		GPIO(41)
#else
#define PWM3		GPIO(50)
#define PWM4		GPIO(51)
#endif
#define NAND_CE2	GPIO(52)
#define NAND_CE3	GPIO(53)

/*------------------------------------------------------------------------*/
/* (CHIP_REV == A3) and later */
#define SD_0		GPIO(55)
#define SD_1		GPIO(56)
#define SD_2		GPIO(57)
#define SD_3		GPIO(58)
#define SD_4		GPIO(59)
#define SD_5		GPIO(60)
#define SD_6		GPIO(61)
#define SD_7		GPIO(62)

/*------------------------------------------------------------------------*/
/* (CHIP_REV == A2) and later */
#define SMIO_2		GPIO(64)
#define SMIO_3		GPIO(65)
#define SMIO_4		GPIO(66)
#define SMIO_5		GPIO(67)
#define SD1_CD		GPIO(67)
#define SMIO_6		GPIO(68)
#define SMIO_38		GPIO(69)
#define SMIO_39		GPIO(70)
#define SMIO_40		GPIO(71)
#define SMIO_41		GPIO(72)
#define SMIO_42		GPIO(73)
#define SMIO_43		GPIO(74)
#define SMIO_44		GPIO(75)
#define SMIO_45		GPIO(76)
#define I2S_WS		GPIO(77)
#define I2S_CLK		GPIO(78)
#define I2S_SO		GPIO(79)
#define I2S_SI		GPIO(80)
#define CLK_AU		GPIO(81)

#if (CHIP_REV == A7L)
#define SSI_4_N		GPIO(88)
#define SSI_5_N		GPIO(89)
#define SSI_6_N		GPIO(90)
#define SSI_7_N		GPIO(91)
#else
/* Tertiary function of GPIO pins */
#define SSI_4_N		GPIO(77)
#define SSI_5_N		GPIO(78)
#define SSI_6_N		GPIO(79)
#define SSI_7_N		GPIO(80)
#endif

/*------------------------------------------------------------------------*/
/* (CHIP_REV == A3) and later */
#define I2S1_SO		GPIO(82)
#define I2S1_SI		GPIO(83)
#define I2S2_SO		GPIO(84)
#define I2S2_SI		GPIO(85)
#define IDC2CLK		GPIO(86)
#define IDC2DATA	GPIO(87)
#define SSI2CLK		GPIO(88)
#define SSI2MOSI	GPIO(89)
#define SSI2MISO	GPIO(90)
#define SSI2_0EN	GPIO(91)
#define SD_8		GPIO(92)
#define SD_9		GPIO(93)
#define SD_10		GPIO(94)
#define SD_11		GPIO(95)

/*------------------------------------------------------------------------*/
/* (CHIP_REV == A5) and later */
#if (CHIP_REV == A5) || (CHIP_REV == A5S) || (CHIP_REV == A6)
#define GMII_MDC_O	GPIO(96)
#define GMII_MDIO	GPIO(97)
#define PHY_TXEN_O 	GPIO(98)
#define PJY_TXER_O  	GPIO(99)
#define PHY_TXD_O_0	GPIO(100)
#define PHY_TXD_O_1   	GPIO(101)
#define CLK_TX_I	GPIO(104)
#define CLK_RX_I	GPIO(105)
#define NAND_FLASH_CE1 	GPIO(114)
#define NAND_FLASH_CE2	GPIO(115)
#define NAND_FLASH_CE3	GPIO(116)
#endif


/*------------------------------------------------------------------------*/
#if (CHIP_REV == A5L)
#define IDC_BUS_HDMI	GPIO(95)
#else
/* (CHIP_REV == A2S/A2M) */
#define IDC_BUS_HDMI	GPIO(87)
#endif

/*------------------------------------------------------------------------*/
#if (CHIP_REV == A5S) 
/* (CHIP_REV == A5S)*/
#define IDC3_BUS_MUX	GPIO(36)

#define IDC3_DATA	GPIO(36)
#define IDC3_CLK	GPIO(37)

#define PHY_TXD_O_2	GPIO(18)
#define PHY_TXD_O_3	GPIO(19)
#define PHY_RXDV_I	GPIO(40)
#define PHY_RXER_I	GPIO(41)
#define PHY_CRS_I	GPIO(42)
#define PHY_COL_I	GPIO(43)
#define PHY_RXD_I_0	GPIO(44)
#define PHY_RXD_I_1	GPIO(47)
#define PHY_RXD_I_2	GPIO(48)
#define PHY_RXD_I_3	GPIO(49)
#define UART1_TX	GPIO(50)
#define UART1_RTS	GPIO(51)
#define UART1_RX	GPIO(52)
#define UART1_CTS	GPIO(53)
#define SSI_SLAVE_MISO	GPIO(50)
#define SSI_SLAVE_EN	GPIO(51)
#define SSI_SLAVE_MOSI	GPIO(52)
#define SSI_SLAVE_CLK	GPIO(53)

#elif (CHIP_REV == A7L)
/* (CHIP_REV == A7L)*/
#define SD_HS		GPIO(22)
#define IDC3_BUS_MUX	GPIO(36)

#define IDC3_DATA	GPIO(36)
#define IDC3_CLK	GPIO(37)

#define UART1_TX	GPIO(50)
#define UART1_RTS	GPIO(51)
#define UART1_RX	GPIO(52)
#define UART1_CTS	GPIO(53)
#define SSI_SLAVE_MISO	GPIO(50)
#define SSI_SLAVE_EN	GPIO(51)
#define SSI_SLAVE_MOSI	GPIO(52)
#define SSI_SLAVE_CLK	GPIO(53)

#elif (CHIP_REV == A7) 

#define VD1_OUT20	GPIO(28)
#define VD1_OUT21	GPIO(29)
#define VD1_OUT22	GPIO(30)
#define VD1_OUT23	GPIO(31)

#define IDC3_BUS_MUX	GPIO(17)
#define IDC3_DATA	GPIO(36)
#define IDC3_CLK	GPIO(37)

#define UART1_TX	GPIO(50)
#define UART1_RTS	GPIO(51)
#define UART1_RX	GPIO(52)
#define UART1_CTS	GPIO(53)
#define SSI_SLAVE_MISO	GPIO(50)
#define SSI_SLAVE_EN	GPIO(51)
#define SSI_SLAVE_MOSI	GPIO(52)
#define SSI_SLAVE_CLK	GPIO(53)

#define PHY_RXDV_I	GPIO(40)
#define PHY_RXER_I	GPIO(41)
#define PHY_CRS_I	GPIO(42)
#define PHY_COL_I	GPIO(43)
#define PHY_RXD_I_0	GPIO(44)
#define PHY_RXD_I_1	GPIO(47)
#define PHY_RXD_I_2	GPIO(48)
#define PHY_RXD_I_3	GPIO(49)
#define PHY_CLK_TX_I	GPIO(96)
#define PHY_CLK_RX_I	GPIO(97)
#define PHY_TXEN_O 	GPIO(98)
#define PHY_TXER_O  	GPIO(99)
#define PHY_GMII_MDO_O	GPIO(100)
#define PHY_GMII_MDC_O	GPIO(101)
#define PHY_TXD_O_0	GPIO(102)
#define PHY_TXD_O_1   	GPIO(103)
#define PHY_TXD_O_2	GPIO(104)
#define PHY_TXD_O_3	GPIO(105)
#define PHY2_CLK_TX_I	GPIO(106)
#define PHY2_CLK_RX_I	GPIO(107)
#define PHY2_TXEN_O 	GPIO(108)
#define PHY2_TXER_O  	GPIO(109)
#define PHY2_GMII_MDO_O	GPIO(110)
#define PHY2_GMII_MDC_O	GPIO(111)
#define PHY2_TXD_O_0	GPIO(112)
#define PHY2_TXD_O_1   	GPIO(113)
#define PHY2_TXD_O_2	GPIO(114)
#define PHY2_TXD_O_3	GPIO(115)
#define PHY2_RXD_I_0	GPIO(116)
#define PHY2_RXD_I_1	GPIO(117)
#define PHY2_RXD_I_2	GPIO(118)
#define PHY2_RXD_I_3	GPIO(119)
#define PHY2_COL_I	GPIO(120)
#define PHY2_CRS_I	GPIO(121)
#define PHY2_RXER_I	GPIO(122)
#define PHY2_RXDV_I	GPIO(123)

#define SSI3_EN0	GPIO(128)
#define SSI3_EN1	GPIO(129)
#define SSI3_EN2	GPIO(130)
#define SSI3_EN3	GPIO(131)
#define SSI3_EN4	GPIO(132)
#define SSI3_EN5	GPIO(133)
#define SSI3_EN6	GPIO(134)
#define SSI3_EN7	GPIO(135)
#define SSI3_MOSI	GPIO(136)
#define SSI3_CLK	GPIO(137)
#define SSI3_MISO	GPIO(138)

#define SC_D0		GPIO(139)
#define SC_D1		GPIO(140)
#define SC_D2		GPIO(141)
#define SC_D3		GPIO(142)
#define SC_D4		GPIO(143)

#elif (CHIP_REV == A5L)

#define IDC3_BUS_MUX	GPIO(63)

#elif (CHIP_REV == I1) /* iONE */

#define APP_PRT_OVCURR_I0	GPIO(7)
#define APP_PRT_OVCURR_I1	GPIO(8)

#define ECHI_PRT_PWR_O0		GPIO(9)
#define ECHI_PRT_PWR_O1		GPIO(10)

#define SSI3_MISO		GPIO(18)
#define SSI4_MISO		GPIO(19)
#define SSI_AHB_CLK_OUT		GPIO(17)
#define SSI_AHB_MISO		GPIO(20)
#define SSI_AHB_MOSI		GPIO(158)
#define SSI_AHB_EN0		GPIO(159)

#define SPDIF_OUT		GPIO(21)
#define SD_HS			GPIO(22)
#define SDXC_HS			GPIO(23)

#define VD1_OUT20		GPIO(28)
#define VD1_OUT21		GPIO(29)
#define VD1_OUT22		GPIO(30)
#define VD1_OUT23		GPIO(31)
#define VD1_OUT15		GPIO(92)
#define VD1_OUT16		GPIO(93)
#define VD1_OUT17		GPIO(94)
#define VD1_OUT18		GPIO(95)

#define TS_CH1_RX_DATA0		GPIO(96)
#define TS_CH1_RX_DATA1		GPIO(97)
#define TS_CH1_RX_DATA2 	GPIO(98)
#define TS_CH1_RX_DATA3  	GPIO(99)
#define TS_CH1_RX_DATA4		GPIO(100)
#define TS_CH1_RX_DATA5		GPIO(101)
#define TS_CH1_RX_DATA6		GPIO(102)
#define TS_CH1_RX_DATA7  	GPIO(103)
#define TS_CH0_RX_DATA0		GPIO(104)
#define TS_CH0_RX_DATA1		GPIO(105)
#define TS_CH0_RX_DATA2		GPIO(106)
#define TS_CH0_RX_DATA3		GPIO(107)
#define TS_CH0_RX_DATA4		GPIO(108)
#define TS_CH0_RX_DATA5  	GPIO(109)
#define TS_CH0_RX_DATA6		GPIO(110)
#define TS_CH0_RX_DATA7		GPIO(111)
#define TS_CH1_TX_DATA0		GPIO(112)
#define TS_CH1_TX_DATA1		GPIO(113)
#define TS_CH1_TX_DATA2 	GPIO(114)
#define TS_CH1_TX_DATA3  	GPIO(115)
#define TS_CH1_TX_DATA4		GPIO(116)
#define TS_CH1_TX_DATA5		GPIO(117)
#define TS_CH1_TX_DATA6		GPIO(118)
#define TS_CH1_TX_DATA7  	GPIO(119)
#define TS_CH0_TX_DATA0		GPIO(120)
#define TS_CH0_TX_DATA1		GPIO(121)
#define TS_CH0_TX_DATA2		GPIO(122)
#define TS_CH0_TX_DATA3		GPIO(123)
#define TS_CH0_TX_DATA4		GPIO(124)
#define TS_CH0_TX_DATA5  	GPIO(125)
#define TS_CH0_TX_DATA6		GPIO(126)
#define TS_CH0_TX_DATA7		GPIO(127)

#define TS_CH0_RX_TS_CLK	GPIO(151)
#define TS_CH1_TX_VAL		GPIO(152)
#define TS_CH1_TX_CLK		GPIO(153)
#define TS_CH1_TX_PSYNC		GPIO(154)
#define TS_CH0_TX_VAL		GPIO(155)
#define TS_CH0_TX_CLK		GPIO(156)
#define TS_CH0_TX_PSYNC		GPIO(157)
#define TS_CH1_RX_VAL		GPIO(187)
#define TS_CH1_RX_PSYNC		GPIO(188)
#define TS_CH0_RX_VAL		GPIO(189)
#define TS_CH0_RX_PSYNC		GPIO(190)
#define TS_CH1_RX_TS_CLK	GPIO(191)

#define SDXC_WP			GPIO(128)
#define SDXC_CD			GPIO(129)

#define UART2_RX		GPIO(130)
#define UART2_CTS		GPIO(131)
#define UART3_RX		GPIO(132)
#define UART3_CTS		GPIO(133)
#define UART2_RTS		GPIO(135)
#define UART2_TX		GPIO(136)
#define UART3_RTS		GPIO(137)
#define UART3_TX		GPIO(138)

#define SSI3_EN0		GPIO(145)
#define SSI3_EN1		GPIO(144)
#define SSI3_EN2		GPIO(143)
#define SSI3_EN3		GPIO(142)
#define SSI3_EN4		GPIO(141)
#define SSI3_EN5		GPIO(140)
#define SSI3_EN6		GPIO(139)
#define SSI3_EN7		GPIO(138)
#define SSI3_CLK		GPIO(146)
#define SSI3_MOSI		GPIO(147)
#define SSI4_MOSI		GPIO(148)
#define SSI4_EN0		GPIO(149)
#define SSI4_CLK		GPIO(150)

/* Tertiary function of GPIO pins */
#define UART1_TX		GPIO(50)
#define UART1_RTS		GPIO(51)
#define UART1_RX		GPIO(52)
#define UART1_CTS		GPIO(53)

#define IDC3_BUS_MUX		GPIO(36)
#define IDC3_DATA		GPIO(36)
#define IDC3_CLK		GPIO(37)

#define UART1_TX		GPIO(50)
#define UART1_RTS		GPIO(51)
#define UART1_RX		GPIO(52)
#define UART1_CTS		GPIO(53)
#define SSI_SLAVE_MISO		GPIO(50)
#define SSI_SLAVE_EN		GPIO(51)
#define SSI_SLAVE_MOSI		GPIO(52)
#define SSI_SLAVE_CLK		GPIO(53)

#define PHY_GMII_MDO_O		GPIO(160)
#define PHY_RXD_I_0		GPIO(161)
#define PHY_RXD_I_1		GPIO(162)
#define PHY_RXD_I_2		GPIO(163)
#define PHY_RXD_I_3		GPIO(164)
#define PHY_RXD_I_4		GPIO(165)
#define PHY_RXD_I_5		GPIO(166)
#define PHY_RXD_I_6		GPIO(167)
#define PHY_RXD_I_7		GPIO(168)
#define PHY_COL_I		GPIO(169)
#define PHY_CRS_I		GPIO(170)
#define PHY_RXER_I		GPIO(171)
#define PHY_RXDV_I		GPIO(172)
#define PHY_CLK_RX_I		GPIO(173)
#define PHY_CLK_TX_I		GPIO(174)
#define PHY_TXD_I_0		GPIO(175)
#define PHY_TXD_I_1		GPIO(176)
#define PHY_TXD_I_2		GPIO(177)
#define PHY_TXD_I_3		GPIO(178)
#define PHY_TXD_I_4		GPIO(179)
#define PHY_TXD_I_5		GPIO(180)
#define PHY_TXD_I_6		GPIO(181)
#define PHY_TXD_I_7		GPIO(182)
#define PHY_TXER_O  		GPIO(183)
#define PHY_TXEN_O 		GPIO(184)
#define PHY_GMII_MDC_O		GPIO(185)

#define P0_ACT_LED		GPIO(186)

#define SC_D0		GPIO(139)
#define SC_D1		GPIO(140)
#define SC_D2		GPIO(141)
#define SC_D3		GPIO(142)
#define SC_D4		GPIO(143)

#else

/* (CHIP_REV == A5) and later */
#define PHY_TXD_O_2	GPIO(102)
#define PHY_TXD_O_3	GPIO(103)
#define PHY_RXDV_I	GPIO(106)
#define PHY_RXER_I	GPIO(107)
#define PHY_CRS_I	GPIO(108)
#define PHY_COL_I	GPIO(109)
#define PHY_RXD_I_0	GPIO(110)
#define PHY_RXD_I_1	GPIO(111)
#define PHY_RXD_I_2	GPIO(112)
#define PHY_RXD_I_3	GPIO(113)
#endif


#if (CHIP_REV == A7L)
/* (CHIP_REV == A7L)*/
#define VD1_OUT00	GPIO(96)
#define VD1_OUT01	GPIO(97)
#define VD1_OUT02	GPIO(98)
#define VD1_OUT03	GPIO(99)
#define VD1_OUT04	GPIO(100)
#define VD1_OUT05	GPIO(101)
#define VD1_OUT06	GPIO(102)
#define VD1_OUT07	GPIO(103)
#define VD1_OUT08	GPIO(104)
#define VD1_OUT09	GPIO(105)
#define VD1_OUT10	GPIO(106)
#define VD1_OUT11	GPIO(107)
#define VD1_OUT12	GPIO(108)
#define VD1_OUT13	GPIO(109)
#define VD1_OUT14	GPIO(110)
#define VD1_OUT15	GPIO(111)
#define VD1_OUT16	GPIO(112)
#define VD1_OUT17	GPIO(113)
#define VD1_OUT18	GPIO(114)
#define VD1_OUT19	GPIO(115)
#define VD1_OUT20	GPIO(116)
#define VD1_OUT21	GPIO(117)
#define VD1_OUT22	GPIO(118)
#define VD1_OUT23	GPIO(119)
#define VD1_VSYNC	GPIO(120)
#define VD1_HSYNC	GPIO(121)
#define VD1_HVLD	GPIO(122)
#define VD1_CLK		GPIO(123)
#else
#define VD1_OUT20	GPIO(28)
#define VD1_OUT21	GPIO(29)
#define VD1_OUT22	GPIO(30)
#define VD1_OUT23	GPIO(31)
#define VD1_OUT15	GPIO(92)
#define VD1_OUT16	GPIO(93)
#define VD1_OUT17	GPIO(94)
#define VD1_OUT18	GPIO(95)
#endif

#endif

