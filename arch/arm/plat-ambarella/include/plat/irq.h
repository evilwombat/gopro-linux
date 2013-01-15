/*
 * arch/arm/plat-ambarella/include/plat/irq.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef __PLAT_AMBARELLA_IRQ_H
#define __PLAT_AMBARELLA_IRQ_H

/* ==========================================================================*/
#define NR_VIC_IRQ_SIZE			(32)

#if defined(CONFIG_ARM_GIC)
#define VIC_IRQ(x)			((x) + 32)
#define NR_SPI_IRQS			(256)
#define SGI_INT_VEC(x)			(x)
#define PPI_INT_VEC(x)			(x)
#define SPI_INT_VEC(x)			(x)
#else
#define VIC_IRQ(x)			(x)
#endif

#define VIC_INT_VEC(x)			(VIC_IRQ(x) + NR_VIC_IRQ_SIZE * 0)
#define VIC2_INT_VEC(x)			(VIC_IRQ(x) + NR_VIC_IRQ_SIZE * 1)
#define VIC3_INT_VEC(x)			(VIC_IRQ(x) + NR_VIC_IRQ_SIZE * 2)
#define VIC4_INT_VEC(x)			(VIC_IRQ(x) + NR_VIC_IRQ_SIZE * 3)

#ifndef NR_SPI_IRQS
#define NR_VIC_IRQS			(VIC_INSTANCES * NR_VIC_IRQ_SIZE)
#define GPIO_INT_VEC(x)			(NR_VIC_IRQS + x)
#else
#define NR_VIC_IRQS			NR_SPI_IRQS
#define GPIO_INT_VEC(x)			(NR_SPI_IRQS + x)
#endif

#ifndef CONFIG_AMBARELLA_EXT_IRQ_NUM
#define CONFIG_AMBARELLA_EXT_IRQ_NUM	(64)
#endif
#define EXT_IRQ(x)			GPIO_INT_VEC(AMBGPIO_SIZE + x)

#define NR_IRQS				EXT_IRQ(CONFIG_AMBARELLA_EXT_IRQ_NUM)

/* ==========================================================================*/
#if (CHIP_REV == A2)

#define USBVBUS_IRQ			VIC_INT_VEC(0)
#define VOUT_IRQ			VIC_INT_VEC(1)
#define VIN_IRQ				VIC_INT_VEC(2)
#define VDSP_IRQ			VIC_INT_VEC(3)
#define USBC_IRQ			VIC_INT_VEC(4)
#define ETH_IRQ				VIC_INT_VEC(4)
#define HOSTTX_IRQ			VIC_INT_VEC(5)
#define HOSTRX_IRQ			VIC_INT_VEC(6)
#define I2STX_IRQ			VIC_INT_VEC(7)
#define I2SRX_IRQ			VIC_INT_VEC(8)
#define UART0_IRQ			VIC_INT_VEC(9)
#define GPIO0_IRQ			VIC_INT_VEC(10)
#define GPIO1_IRQ			VIC_INT_VEC(11)
#define GPIO2_IRQ			VIC_INT_VEC(11)	/* Shared with GPIO1_IRQ */
#define TIMER1_IRQ			VIC_INT_VEC(12)
#define TIMER2_IRQ			VIC_INT_VEC(13)
#define TIMER3_IRQ			VIC_INT_VEC(14)
#define DMA_IRQ				VIC_INT_VEC(15)
#define FIOCMD_IRQ			VIC_INT_VEC(16)
#define FIODMA_IRQ			VIC_INT_VEC(17)
#define SD_IRQ				VIC_INT_VEC(18)
#define SD2_IRQ				VIC_INT_VEC(18)	/* Shared with SD_IRQ */
#define IDC_IRQ				VIC_INT_VEC(19)
#define SSI_IRQ				VIC_INT_VEC(20)
#define WDT_IRQ				VIC_INT_VEC(21)
#define IRIF_IRQ			VIC_INT_VEC(22)
#define CFCD1_IRQ			VIC_INT_VEC(23)
#define SD1CD_IRQ			VIC_INT_VEC(24)

#elif (CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q)

#define USBVBUS_IRQ			VIC_INT_VEC(0)
#define VOUT_IRQ			VIC_INT_VEC(1)
#define VIN_IRQ				VIC_INT_VEC(2)
#define VDSP_IRQ			VIC_INT_VEC(3)
#define USBC_IRQ			VIC_INT_VEC(4)
#define MS_IRQ				VIC_INT_VEC(5)
#define HIF_ARM1_IRQ			VIC_INT_VEC(6)
#define I2STX_IRQ			VIC_INT_VEC(7)
#define I2SRX_IRQ			VIC_INT_VEC(8)
#define UART0_IRQ			VIC_INT_VEC(9)
#define GPIO0_IRQ			VIC_INT_VEC(10)
#define GPIO1_IRQ			VIC_INT_VEC(11)
#define TIMER1_IRQ			VIC_INT_VEC(12)
#define TIMER2_IRQ			VIC_INT_VEC(13)
#define TIMER3_IRQ			VIC_INT_VEC(14)
#define DMA_IRQ				VIC_INT_VEC(15)
#define FIOCMD_IRQ			VIC_INT_VEC(16)
#define FIODMA_IRQ			VIC_INT_VEC(17)
#define SD_IRQ				VIC_INT_VEC(18)
#define SD2_IRQ				VIC_INT_VEC(18)	/* Shared with SD_IRQ */
#define IDC_IRQ				VIC_INT_VEC(19)
#define SSI_IRQ				VIC_INT_VEC(20)
#define WDT_IRQ				VIC_INT_VEC(21)
#define IRIF_IRQ			VIC_INT_VEC(22)
#define CFCD1_IRQ			VIC_INT_VEC(23)
#define SD1CD_IRQ			VIC_INT_VEC(24)
#define MOTOR_IRQ			VIC_INT_VEC(25)
#define HDMI_IRQ			VIC_INT_VEC(26)
#define ETH_IRQ				VIC_INT_VEC(27)
#define IDSP_ERROR_IRQ			VIC_INT_VEC(28)
#define HIF_ARM2_IRQ			VIC_INT_VEC(29)
#define GPIO2_IRQ			VIC_INT_VEC(30)
#define CFCD2_IRQ			VIC_INT_VEC(31)

#elif (CHIP_REV == A5)

#define USBVBUS_IRQ			VIC_INT_VEC(0)
#define VOUT_IRQ			VIC_INT_VEC(1)
#define VIN_IRQ				VIC_INT_VEC(2)
#define VDSP_IRQ			VIC_INT_VEC(3)
#define USBC_IRQ			VIC_INT_VEC(4)
#define HOSTTX_IRQ			VIC_INT_VEC(5)
#define HOSTRX_IRQ			VIC_INT_VEC(6)
#define I2STX_IRQ			VIC_INT_VEC(7)
#define I2SRX_IRQ			VIC_INT_VEC(8)
#define UART0_IRQ			VIC_INT_VEC(9)
#define GPIO0_IRQ			VIC_INT_VEC(10)
#define GPIO1_IRQ			VIC_INT_VEC(11)
#define TIMER1_IRQ			VIC_INT_VEC(12)
#define TIMER2_IRQ			VIC_INT_VEC(13)
#define TIMER3_IRQ			VIC_INT_VEC(14)
#define DMA_IRQ				VIC_INT_VEC(15)
#define FIOCMD_IRQ			VIC_INT_VEC(16)
#define FIODMA_IRQ			VIC_INT_VEC(17)
#define SD_IRQ				VIC_INT_VEC(18)
#define IDC_IRQ				VIC_INT_VEC(19)
#define SSI_IRQ				VIC_INT_VEC(20)
#define WDT_IRQ				VIC_INT_VEC(21)
#define IRIF_IRQ			VIC_INT_VEC(22)
#define CFCD1_IRQ			VIC_INT_VEC(23)
#define SD1CD_IRQ			VIC_INT_VEC(24)
#define UART1_IRQ			VIC_INT_VEC(25)
#define GPIO3_IRQ			VIC_INT_VEC(26)
#define ETH_IRQ				VIC_INT_VEC(27)
#define IDSP_ERROR_IRQ			VIC_INT_VEC(28)
#define VOUT_SYNC_MISSED_IRQ		VIC_INT_VEC(29)
#define GPIO2_IRQ			VIC_INT_VEC(30)
#define CFCD2_IRQ			VIC_INT_VEC(31)

#define AUDIO_ORC_IRQ			VIC2_INT_VEC(0)
#define DMA_FIOS_IRQ			VIC2_INT_VEC(1)
#define ADC_LEVEL_IRQ			VIC2_INT_VEC(2)
#define VOUT1_SYNC_MISSED_IRQ		VIC2_INT_VEC(3)
#define IDC2_IRQ			VIC2_INT_VEC(4)
#define IDSP_LAST_PIXEL_IRQ		VIC2_INT_VEC(5)
#define IDSP_VSYNC_IRQ			VIC2_INT_VEC(6)
#define IDSP_SENSOR_VSYNC_IRQ		VIC2_INT_VEC(7)
#define HDMI_IRQ			VIC2_INT_VEC(8)
#define SSI2_IRQ			VIC2_INT_VEC(9)
#define VOUT_TV_SYNC_IRQ		VIC2_INT_VEC(10)
#define VOUT_LCD_SYNC_IRQ		VIC2_INT_VEC(11)

#elif (CHIP_REV == A5S)

#define USBVBUS_IRQ			VIC_INT_VEC(0)
#define VOUT_IRQ			VIC_INT_VEC(1)
#define VIN_IRQ				VIC_INT_VEC(2)
#define VDSP_IRQ			VIC_INT_VEC(3)
#define USBC_IRQ			VIC_INT_VEC(4)
#define HIF_ARM1_IRQ			VIC_INT_VEC(5)
#define HIF_ARM2_IRQ			VIC_INT_VEC(6)
#define I2STX_IRQ			VIC_INT_VEC(7)
#define I2SRX_IRQ			VIC_INT_VEC(8)
#define UART0_IRQ			VIC_INT_VEC(9)
#define GPIO0_IRQ			VIC_INT_VEC(10)
#define GPIO1_IRQ			VIC_INT_VEC(11)
#define TIMER1_IRQ			VIC_INT_VEC(12)
#define TIMER2_IRQ			VIC_INT_VEC(13)
#define TIMER3_IRQ			VIC_INT_VEC(14)
#define DMA_IRQ				VIC_INT_VEC(15)
#define FIOCMD_IRQ			VIC_INT_VEC(16)
#define FIODMA_IRQ			VIC_INT_VEC(17)
#define SD_IRQ				VIC_INT_VEC(18)
#define IDC_IRQ				VIC_INT_VEC(19)
#define SSI_IRQ				VIC_INT_VEC(20)
#define WDT_IRQ				VIC_INT_VEC(21)
#define IRIF_IRQ			VIC_INT_VEC(22)
#define CFCD1_IRQ			VIC_INT_VEC(23)
#define SD1CD_IRQ			VIC_INT_VEC(24)
#define UART1_IRQ			VIC_INT_VEC(25)
#define GPIO3_IRQ			VIC_INT_VEC(26)
#define ETH_IRQ				VIC_INT_VEC(27)
#define IDSP_ERROR_IRQ			VIC_INT_VEC(28)
#define VOUT_SYNC_MISSED_IRQ		VIC_INT_VEC(29)
#define GPIO2_IRQ			VIC_INT_VEC(30)
#define CFCD2_IRQ			VIC_INT_VEC(31)

#define AUDIO_ORC_IRQ			VIC2_INT_VEC(0)
#define DMA_FIOS_IRQ			VIC2_INT_VEC(1)
#define ADC_LEVEL_IRQ			VIC2_INT_VEC(2)
#define VOUT1_SYNC_MISSED_IRQ		VIC2_INT_VEC(3)
#define IDC2_IRQ			VIC2_INT_VEC(4)
#define IDSP_LAST_PIXEL_IRQ		VIC2_INT_VEC(5)
#define IDSP_VSYNC_IRQ			VIC2_INT_VEC(6)
#define IDSP_SENSOR_VSYNC_IRQ		VIC2_INT_VEC(7)
#define HDMI_IRQ			VIC2_INT_VEC(8)
#define SSI2_IRQ			VIC2_INT_VEC(9)
#define VOUT_TV_SYNC_IRQ		VIC2_INT_VEC(10)
#define VOUT_LCD_SYNC_IRQ		VIC2_INT_VEC(11)
#define ORC_VOUT0_IRQ			VIC2_INT_VEC(12)
#define AES_IRQ				VIC2_INT_VEC(13)
#define DES_IRQ				VIC2_INT_VEC(14)
#define MS_IRQ				VIC2_INT_VEC(15)
#define GDMA_IRQ			VIC2_INT_VEC(16)
#define MOTOR_IRQ			VIC2_INT_VEC(17)

#elif (CHIP_REV == A7)

#define USBVBUS_IRQ			VIC_INT_VEC(0)
#define VOUT_IRQ			VIC_INT_VEC(1)
#define VIN_IRQ				VIC_INT_VEC(2)
#define VDSP_IRQ			VIC_INT_VEC(3)
#define USBC_IRQ			VIC_INT_VEC(4)
#define HIF_ARM1_IRQ			VIC_INT_VEC(5)
#define HIF_ARM2_IRQ			VIC_INT_VEC(6)
#define I2STX_IRQ			VIC_INT_VEC(7)
#define I2SRX_IRQ			VIC_INT_VEC(8)
#define UART0_IRQ			VIC_INT_VEC(9)
#define GPIO0_IRQ			VIC_INT_VEC(10)
#define GPIO1_IRQ			VIC_INT_VEC(11)
#define TIMER1_IRQ			VIC_INT_VEC(12)
#define TIMER2_IRQ			VIC_INT_VEC(13)
#define TIMER3_IRQ			VIC_INT_VEC(14)
#define DMA_IRQ				VIC_INT_VEC(15)
#define FIOCMD_IRQ			VIC_INT_VEC(16)
#define FIODMA_IRQ			VIC_INT_VEC(17)
#define SD_IRQ				VIC_INT_VEC(18)
#define IDC_IRQ				VIC_INT_VEC(19)
#define SSI_IRQ				VIC_INT_VEC(20)
#define WDT_IRQ				VIC_INT_VEC(21)
#define IRIF_IRQ			VIC_INT_VEC(22)
#define CFCD1_IRQ			VIC_INT_VEC(23)
#define SD1CD_IRQ			VIC_INT_VEC(24)
#define UART1_IRQ			VIC_INT_VEC(25)
#define SSI_SLAVE_IRQ			VIC_INT_VEC(26)
#define ETH_IRQ				VIC_INT_VEC(27)
#define IDSP_ERROR_IRQ			VIC_INT_VEC(28)
#define GPIO3_IRQ			VIC_INT_VEC(29)
#define GPIO2_IRQ			VIC_INT_VEC(30)
#define CFCD2_IRQ			VIC_INT_VEC(31)

#define DMA_FIOS_IRQ			VIC2_INT_VEC(1)
#define ADC_LEVEL_IRQ			VIC2_INT_VEC(2)
#define IDC2_IRQ			VIC2_INT_VEC(4)
#define IDSP_LAST_PIXEL_IRQ		VIC2_INT_VEC(5)
#define IDSP_VSYNC_IRQ			VIC2_INT_VEC(6)
#define IDSP_SENSOR_VSYNC_IRQ		VIC2_INT_VEC(7)
#define HDMI_IRQ			VIC2_INT_VEC(8)
#define SSI2_IRQ			VIC2_INT_VEC(9)
#define VOUT_TV_SYNC_IRQ		VIC2_INT_VEC(10)
#define VOUT_LCD_SYNC_IRQ		VIC2_INT_VEC(11)
#define ORC_VOUT0_IRQ			VIC2_INT_VEC(12)
#define AES_IRQ				VIC2_INT_VEC(13)
#define DES_IRQ				VIC2_INT_VEC(14)
#define MS_IRQ				VIC2_INT_VEC(15)
#define GPIO4_IRQ			VIC2_INT_VEC(16)
#define MOTOR_IRQ			VIC2_INT_VEC(17)
#define GDMA_IRQ			VIC2_INT_VEC(18)
#define FDET_IRQ			VIC2_INT_VEC(19)
#define ETH2_IRQ			VIC2_INT_VEC(20)
#define SSI3_IRQ			VIC2_INT_VEC(21)
#define CODE_VDSP_3_IRQ			VIC2_INT_VEC(22)
#define CODE_VDSP_2_IRQ			VIC2_INT_VEC(23)
#define CODE_VDSP_1_IRQ			VIC2_INT_VEC(24)
#define MD5_SHA1_IRQ			VIC2_INT_VEC(25)
#define ROLLERING_SHUTTER_IRQ		VIC2_INT_VEC(26)

#elif (CHIP_REV == A7L)

#define USBVBUS_IRQ			VIC_INT_VEC(0)
#define VOUT_IRQ			VIC_INT_VEC(1)
#define VIN_IRQ				VIC_INT_VEC(2)
#define GDMA_IRQ			VIC_INT_VEC(3)
#define USBC_IRQ			VIC_INT_VEC(4)
#define SDIO_CD_IRQ			VIC_INT_VEC(5)
#define VDSP_IRQ			VIC_INT_VEC(6)
#define I2STX_IRQ			VIC_INT_VEC(7)
#define I2SRX_IRQ			VIC_INT_VEC(8)
#define UART0_IRQ			VIC_INT_VEC(9)
#define GPIO0_IRQ       		VIC_INT_VEC(10)
#define GPIO1_IRQ       		VIC_INT_VEC(11)
#define TIMER1_IRQ       		VIC_INT_VEC(12)
#define TIMER2_IRQ       		VIC_INT_VEC(13)
#define TIMER3_IRQ       		VIC_INT_VEC(14)
#define DMA_IRQ				VIC_INT_VEC(15)
#define FIOCMD_IRQ			VIC_INT_VEC(16)
#define FIODMA_IRQ			VIC_INT_VEC(17)
#define SD_IRQ				VIC_INT_VEC(18)
#define IDC_IRQ				VIC_INT_VEC(19)
#define SSI_IRQ				VIC_INT_VEC(20)
#define WDT_IRQ				VIC_INT_VEC(21)
#define IRIF_IRQ			VIC_INT_VEC(22)
#define SD1CD_IRQ			VIC_INT_VEC(24)
#define UART1_IRQ			VIC_INT_VEC(25)
#define IDC2_IRQ			VIC_INT_VEC(26)
#define IDSP_ERROR_IRQ			VIC_INT_VEC(28)
#define GPIO2_IRQ			VIC_INT_VEC(30)

#define DMA_FIOS_IRQ			VIC2_INT_VEC(1)
#define ADC_LEVEL_IRQ			VIC2_INT_VEC(2)
#define IDSP_LAST_PIXEL_IRQ		VIC2_INT_VEC(5)
#define IDSP_VSYNC_IRQ			VIC2_INT_VEC(6)
#define IDSP_SENSOR_VSYNC_IRQ		VIC2_INT_VEC(7)
#define HDMI_IRQ			VIC2_INT_VEC(8)
#define SSI2_IRQ			VIC2_INT_VEC(9)
#define VOUT_TV_SYNC_IRQ		VIC2_INT_VEC(10)
#define VOUT_LCD_SYNC_IRQ		VIC2_INT_VEC(11)
#define ORC_VOUT0_IRQ			VIC2_INT_VEC(12)
#define XINT_IRQ			VIC2_INT_VEC(15)
#define MOTOR_IRQ			VIC2_INT_VEC(17)
#define GPIO3_IRQ			VIC2_INT_VEC(19)
#define DRAM_ERROR_IRQ			VIC2_INT_VEC(23)
#define SD2_IRQ				VIC2_INT_VEC(24)
#define TIMER4_IRQ       		VIC2_INT_VEC(27)
#define TIMER5_IRQ       		VIC2_INT_VEC(28)
#define TIMER6_IRQ       		VIC2_INT_VEC(29)
#define TIMER7_IRQ       		VIC2_INT_VEC(30)
#define TIMER8_IRQ       		VIC2_INT_VEC(31)

#elif (CHIP_REV == I1)

#define USBVBUS_IRQ			VIC_INT_VEC(0)
#define ROLLING_SHUTTER_IRQ		VIC_INT_VEC(3)
#define USBC_IRQ			VIC_INT_VEC(4)
#define HIF_ARM1_IRQ			VIC_INT_VEC(5)
#define HIF_ARM2_IRQ			VIC_INT_VEC(6)
#define I2STX_IRQ			VIC_INT_VEC(7)
#define I2SRX_IRQ			VIC_INT_VEC(8)
#define UART0_IRQ			VIC_INT_VEC(9)
#define GPIO0_IRQ			VIC_INT_VEC(10)
#define GPIO1_IRQ			VIC_INT_VEC(11)
#define DMA_IRQ				VIC_INT_VEC(15)
#define FIOCMD_IRQ			VIC_INT_VEC(16)
#define FIODMA_IRQ			VIC_INT_VEC(17)
#define SD_IRQ				VIC_INT_VEC(18)
#define IDC_IRQ				VIC_INT_VEC(19)
#define SSI_IRQ				VIC_INT_VEC(20)
#define IRIF_IRQ			VIC_INT_VEC(22)
#define CFCD1_IRQ			VIC_INT_VEC(23)
#define SD1CD_IRQ			VIC_INT_VEC(24)
#define UART1_IRQ			VIC_INT_VEC(25)
#define SSI_SLAVE_IRQ			VIC_INT_VEC(26)
#define ETH_IRQ				VIC_INT_VEC(27)
#define IDSP_SOFT_IRQ			VIC_INT_VEC(28)
#define ETH_POWER_IRQ			VIC_INT_VEC(29)
#define GPIO2_IRQ			VIC_INT_VEC(30)
#define CFCD2_IRQ			VIC_INT_VEC(31)

#define DRAM_AXI_ERROR_IRQ		VIC2_INT_VEC(0)
#define DMA_FIOS_IRQ			VIC2_INT_VEC(1)
#define ADC_LEVEL_IRQ			VIC2_INT_VEC(2)
#define SSI3_IRQ			VIC2_INT_VEC(3)
#define IDC2_IRQ			VIC2_INT_VEC(4)
#define IDSP_LAST_PIXEL_IRQ		VIC2_INT_VEC(5)
#define IDSP_VSYNC_IRQ			VIC2_INT_VEC(6)
#define IDSP_SENSOR_VSYNC_IRQ		VIC2_INT_VEC(7)
#define HDMI_IRQ			VIC2_INT_VEC(8)
#define SSI2_IRQ			VIC2_INT_VEC(9)
#define AES_IRQ				VIC2_INT_VEC(13)
#define DES_IRQ				VIC2_INT_VEC(14)
#define MS_IRQ				VIC2_INT_VEC(15)
#define USB_EHCI_IRQ			VIC2_INT_VEC(16)
#define MOTOR_IRQ			VIC2_INT_VEC(17)
#define MD5_SHA1_IRQ			VIC2_INT_VEC(18)
#define GPIO3_IRQ			VIC2_INT_VEC(19)
#define GPIO4_IRQ			VIC2_INT_VEC(20)
#define GPIO5_IRQ			VIC2_INT_VEC(21)
#define SATA_IRQ			VIC2_INT_VEC(22)
#define DRAM_ERROR_IRQ			VIC2_INT_VEC(23)
#define SD2_IRQ				VIC2_INT_VEC(24)
#define UART2_IRQ			VIC2_INT_VEC(25)
#define UART3_IRQ			VIC2_INT_VEC(26)

#define GSSI_IRQ			VIC3_INT_VEC(0)
#define SSI4_IRQ			VIC3_INT_VEC(1)
#define RNG_IRQ				VIC3_INT_VEC(2)
#define SD2CD_IRQ			VIC3_INT_VEC(3)
#define CORTEX_CORE0_IRQ		VIC3_INT_VEC(4)
#define CORTEX_CORE1_IRQ		VIC3_INT_VEC(5)
#define TS_CH1_RX_IRQ			VIC3_INT_VEC(6)
#define TS_CH0_RX_IRQ			VIC3_INT_VEC(7)
#define TS_CH1_TX_IRQ			VIC3_INT_VEC(8)
#define TS_CH0_TX_IRQ			VIC3_INT_VEC(9)
#define AXI_SOFT_IRQ(x)			VIC3_INT_VEC((x) + 10)	/* 0 <= x <= 13 */
#define CORTEX_WDT_IRQ			VIC3_INT_VEC(24)
#define USB_OHCI_IRQ			VIC3_INT_VEC(25)
#define SPDIF_IRQ			VIC3_INT_VEC(26)
#define SSI_AHB_IRQ			VIC3_INT_VEC(27)

#if defined(CONFIG_ARM_GIC)
#define VOUT_IRQ			VIC4_INT_VEC(0)
#define VIN_IRQ				VIC4_INT_VEC(1)
#define TIMER1_IRQ			VIC4_INT_VEC(2)
#define TIMER2_IRQ			VIC4_INT_VEC(3)
#define TIMER3_IRQ			VIC4_INT_VEC(4)
#define WDT_IRQ				VIC4_INT_VEC(5)
#define VOUT_TV_SYNC_IRQ		VIC4_INT_VEC(6)
#define VOUT_LCD_SYNC_IRQ		VIC4_INT_VEC(7)
#define ORC_VOUT0_IRQ			VIC4_INT_VEC(8)
#define TIMER4_IRQ			VIC4_INT_VEC(9)
#define TIMER5_IRQ			VIC4_INT_VEC(10)
#define TIMER6_IRQ			VIC4_INT_VEC(11)
#define TIMER7_IRQ			VIC4_INT_VEC(12)
#define TIMER8_IRQ			VIC4_INT_VEC(13)
#define CODING_ORC0_IRQ			VIC4_INT_VEC(14)
#define CODING_ORC1_IRQ			VIC4_INT_VEC(15)
#define CODING_ORC2_IRQ			VIC4_INT_VEC(16)
#define CODING_ORC3_IRQ			VIC4_INT_VEC(17)
#else
#define VOUT_IRQ			VIC_INT_VEC(1)
#define VIN_IRQ				VIC_INT_VEC(2)
#define TIMER1_IRQ			VIC_INT_VEC(12)
#define TIMER2_IRQ			VIC_INT_VEC(13)
#define TIMER3_IRQ			VIC_INT_VEC(14)
#define WDT_IRQ				VIC_INT_VEC(21)
#define VOUT_TV_SYNC_IRQ		VIC2_INT_VEC(10)
#define VOUT_LCD_SYNC_IRQ		VIC2_INT_VEC(11)
#define ORC_VOUT0_IRQ			VIC2_INT_VEC(12)
#define TIMER4_IRQ			VIC2_INT_VEC(27)
#define TIMER5_IRQ			VIC2_INT_VEC(28)
#define TIMER6_IRQ			VIC2_INT_VEC(29)
#define TIMER7_IRQ			VIC2_INT_VEC(30)
#define TIMER8_IRQ			VIC2_INT_VEC(31)
#define CODING_ORC0_IRQ			VIC3_INT_VEC(28)
#define CODING_ORC1_IRQ			VIC3_INT_VEC(29)
#define CODING_ORC2_IRQ			VIC3_INT_VEC(30)
#define CODING_ORC3_IRQ			VIC3_INT_VEC(31)
#endif

#define GLOBAL_TIMER_IRQ		PPI_INT_VEC(27)
#define LEGACY_FIQ			PPI_INT_VEC(28)
#define LOCAL_TIMER_IRQ			PPI_INT_VEC(29)
#define LOCAL_WDOG_IRQ			PPI_INT_VEC(30)
#define LEGACY_IRQ			PPI_INT_VEC(31)

#define DDD_IRQ				SPI_INT_VEC(153)
#define DECODE_ERROR_IRQ		SPI_INT_VEC(154)
#define SLAVE_ERROR_IRQ			SPI_INT_VEC(155)
#define FMEM_READ_ERROR_IRQ		SPI_INT_VEC(156)
#define FMEM_WRITE_ERROR_IRQ		SPI_INT_VEC(157)
#define L2CC_ECNTR_IRQ			SPI_INT_VEC(158)
#define L2CC_COMBINED_IRQ		SPI_INT_VEC(159)

#endif

/* ==========================================================================*/
#ifndef __ASSEMBLER__

/* ==========================================================================*/

/* ==========================================================================*/
static inline int gpio_to_irq(unsigned gpio)
{
	if (gpio < GPIO_MAX_LINES)
		return GPIO_INT_VEC(gpio);

	return -EINVAL;
}

static inline int irq_to_gpio(unsigned irq)
{
	if ((irq > GPIO_INT_VEC(0)) && (irq < NR_IRQS))
		return irq - GPIO_INT_VEC(0);

	return -EINVAL;
}

extern void ambarella_init_irq(void);

extern u32 ambarella_irq_suspend(u32 level);
extern u32 ambarella_irq_resume(u32 level);
extern void ambarella_swvic_set(u32 irq);
extern void ambarella_swvic_clr(u32 irq);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

