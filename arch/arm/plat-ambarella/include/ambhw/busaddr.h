/*
 * ambhw/busaddr.h
 *
 * History:
 *	2006/12/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2006-2008, Ambarella, Inc.
 */

#ifndef __AMBHW__BUSADDR_H__
#define __AMBHW__BUSADDR_H__

#if !defined(__KERNEL__)
/*
 * The AHB physical address is at 0x60000000 but is always mapped to
 * virtual address 0xd8000000
 * The APB physical address is at 0x70000000 but is always mapped to
 * virtual address 0xd9000000
 */
#define AHB_BASE		0xd8000000
#define APB_BASE		0xd9000000
#define CRYPT_BASE		0xdb000000
#define DRAM_VIRT_BASE		0xfece0000

/* The physical address of AHB & APB - they SHOULD NOT be used directly */
#if (CHIP_REV == I1)
#define AHB_PHYS_BASE		0xe0000000
#define APB_PHYS_BASE		0xe8000000
#define DRAM_PHYS_BASE		0xdffe0000
#define AXI_PHYS_BASE		0xf0000000
#define CRYPT_PHYS_BASE		0xfffef000
#define PHY_BUS_MAP_TYPE	1
#elif (CHIP_REV == A7L)
#define AHB_PHYS_BASE		0x60000000
#define APB_PHYS_BASE		0x70000000
#define PHY_BUS_MAP_TYPE	2
#define DRAM_PHYS_BASE		0xfffe0000
#else
#define AHB_PHYS_BASE		0x60000000
#define APB_PHYS_BASE		0x70000000
#define PHY_BUS_MAP_TYPE	0
#define DRAM_PHYS_BASE		AHB_PHYS_BASE
#endif

#if defined(__BUILD_AMBOOT__) || defined(__AMBOOT__)
/* The boot-loader still deals with physical address */
#undef AHB_BASE
#define AHB_BASE		AHB_PHYS_BASE
#undef APB_BASE
#define APB_BASE		APB_PHYS_BASE
#undef CRYPT_BASE
#define CRYPT_BASE		CRYPT_PHYS_BASE
#undef DRAM_VIRT_BASE
#define DRAM_VIRT_BASE		DRAM_PHYS_BASE
#endif
#endif

/* DRAM slave offsets */

#if (CHIP_REV == I1) || (CHIP_REV == A7L)
#define DRAM_DRAM_OFFSET 		0x0000
#define DRAM_DDRC_OFFSET  		0x0800
#else
#define DRAM_DRAM_OFFSET 		0x4000
#define DRAM_DDRC_OFFSET  		DRAM_DRAM_OFFSET
#endif

/* AHB slave offsets */
#define FIO_FIFO_OFFSET			0x0000
#define FIO_OFFSET			0x1000
#define SD_OFFSET			0x2000
#define VIC_OFFSET			0x3000
#define DRAM_OFFSET			DRAM_DDRC_OFFSET
#define DMA_OFFSET			0x5000
#define USBDC_OFFSET			0x6000
#define VOUT_OFFSET			0x8000
#define VIN_OFFSET			0x9000
#define I2S_OFFSET			0xa000
#define HOST_OFFSET			0xb000
#define SDIO_OFFSET			0xc000

#if (CHIP_REV == I1)
#define SD2_OFFSET			0x1b000
#else
#define SD2_OFFSET			0xc000
#endif

#if (CHIP_REV == A6)
#define MS_OFFSET			0x17000
#elif (CHIP_REV == A2) || (CHIP_REV == A2S) 
#define MS_OFFSET			0xc000
#else
#define MS_OFFSET			0x16000
#endif

#if (CHIP_REV == A6)
#define HIF_OFFSET			0x14000
#elif (CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == I1)
#define HIF_OFFSET			0xb000
#else
#define HIF_OFFSET			0xd000
#endif

#define SSI_DMA_OFFSET			0xd000 /* iONE */

#define ETH_OFFSET			0xe000
#define ETH_DMA_OFFSET			0xf000
#define VIC2_OFFSET			0x10000
#define VOUT2_OFFSET			0x11000
#define VOUT_CLUT_OFFSET		0x11000
#define DMA_FIOS_OFFSET			0x12000
#define DMA2_OFFSET			0x12000

#if (CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q) || \
    (CHIP_REV == A5L)
#define HDMI_OFFSET			0xb000
#else
#define HDMI_OFFSET			0x13000
#endif

#define CRYPT_UNIT_OFFSET		0x14000
#define AHB_SECURE_SCRATCHPAD_OFFSET	0x14000 /* iONE */
#define GRAPHICS_DMA_OFFSET		0x15000

#if (CHIP_REV == I1)
#define TS_OFFSET			0x1d000
#else
#define TS_OFFSET			0x15000
#endif

#define SPIB_OFFSET			0x16000
#define FACE_DETECTION_OFFSET		0x17000 /* A7 */
#define ETH2_OFFSET			0x18000
#define ETH2_DMA_OFFSET			0x19000

#define USB_HOST_CTRL_EHCI_OFFSET	0x17000 /* iONE */
#define USB_HOST_CTRL_OHCI_OFFSET	0x18000 /* iONE */
#define AHB_SCRATCHPAD_OFFSET		0x19000 /* iONE */
#define SATA_OFFSET			0x1a000 /* iONE */

#if (CHIP_REV == I1)
#define SDXC_OFFSET			0x1b000 /* iONE */
#else
#define SDXC_OFFSET			0xc000
#endif
#define VIC3_OFFSET			0x1c000 /* iONE */
#define SPDIF_OFFSET			0x1e000 /* iONE */
#define AHB_SECURE_OFFSET		0x1f000 /* iONE */

/* AHB slave base addresses */
#define FIO_FIFO_BASE			(AHB_BASE + FIO_FIFO_OFFSET)
#define FIO_BASE			(AHB_BASE + FIO_OFFSET)
#define SD_BASE				(AHB_BASE + SD_OFFSET)
#define VIC_BASE			(AHB_BASE + VIC_OFFSET)
#define DRAM_BASE			(DRAM_PHYS_BASE + DRAM_OFFSET)
#define DMA_BASE			(AHB_BASE + DMA_OFFSET)
#define USBDC_BASE			(AHB_BASE + USBDC_OFFSET)
#define VOUT_BASE			(AHB_BASE + VOUT_OFFSET)
#define VIN_BASE			(AHB_BASE + VIN_OFFSET)
#define I2S_BASE			(AHB_BASE + I2S_OFFSET)
#define I2S_BASE_PHYS 			(AHB_PHYS_BASE + I2S_OFFSET)
#define SD2_BASE			(AHB_BASE + SD2_OFFSET)
#define MS_BASE				(AHB_BASE + MS_OFFSET)
#define HOST_BASE			(AHB_BASE + HOST_OFFSET)
#define HIF_BASE			(AHB_BASE + HIF_OFFSET)
#define ETH_BASE			(AHB_BASE + ETH_OFFSET)
#define ETH_DMA_BASE			(AHB_BASE + ETH_DMA_OFFSET)
#define VIC2_BASE			(AHB_BASE + VIC2_OFFSET)
#define VOUT2_BASE			(AHB_BASE + VOUT2_OFFSET)
#define VOUT_CLUT_BASE			(AHB_BASE + VOUT_CLUT_OFFSET)
#define DMA_FIOS_BASE			(AHB_BASE + DMA_FIOS_OFFSET)
#define DMA2_BASE			(AHB_BASE + DMA2_OFFSET)
#define HDMI_BASE			(AHB_BASE + HDMI_OFFSET)
#define TS_BASE				(AHB_BASE + TS_OFFSET)
#define SPIB_BASE			(AHB_BASE + SPIB_OFFSET)
#define GRAPHICS_DMA_BASE		(AHB_BASE + GRAPHICS_DMA_OFFSET)
#define ETH2_BASE			(AHB_BASE + ETH2_OFFSET)
#define ETH2_DMA_BASE			(AHB_BASE + ETH2_DMA_OFFSET)
#define FACE_DETECTION_BASE		(AHB_BASE + FACE_DETECTION_OFFSET)
#define SSI_DMA_BASE			(AHB_BASE + SSI_DMA_OFFSET)
#define AHB_SECURE_SCRATCHPAD_BASE	(AHB_BASE + AHB_SECURE_SCRATCHPAD_OFFSET)
#define USB_HOST_CTRL_EHCI_BASE		(AHB_BASE + USB_HOST_CTRL_EHCI_OFFSET)
#define USB_HOST_CTRL_OHCI_BASE		(AHB_BASE + USB_HOST_CTRL_OHCI_OFFSET)
#define AHB_SCRATCHPAD_BASE		(AHB_BASE + AHB_SCRATCHPAD_OFFSET)
#define SATA_BASE			(AHB_BASE + SATA_OFFSET)
#define SDXC_BASE			(AHB_BASE + SDXC_OFFSET)
#define VIC3_BASE			(AHB_BASE + VIC3_OFFSET)
#define SPDIF_BASE			(AHB_BASE + SPDIF_OFFSET)
#define AHB_SECURE_BASE			(AHB_BASE + AHB_SECURE_OFFSET)
#if !defined(__KERNEL__)
#if defined(CRYPT_PHYS_BASE)
#define CRYPT_UNIT_BASE			(CRYPT_PHYS_BASE)
#else
#define CRYPT_UNIT_BASE			(AHB_BASE + CRYPT_UNIT_OFFSET)
#endif
#else
#if defined(CRYPT_BASE)
#define CRYPT_UNIT_BASE			(CRYPT_BASE)
#else
#define CRYPT_UNIT_BASE			(AHB_BASE + CRYPT_UNIT_OFFSET)
#endif
#endif

/* AHB slave registers */
#define FIO_REG(x)			(FIO_BASE + (x))
#define XD_REG(x)			(FIO_BASE + (x))
#define CF_REG(x)			(FIO_BASE + (x))
#define SD_REG(x)			(SD_BASE + (x))
#define VIC_REG(x)			(VIC_BASE + (x))
#define DRAM_REG(x)			(DRAM_BASE + (x))
#define DMA_REG(x)			(DMA_BASE + (x))
#define USBDC_REG(x)			(USBDC_BASE + (x))
#define VOUT_REG(x)			(VOUT_BASE + (x))
#define VIN_REG(x)			(VIN_BASE + (x))
#define I2S_REG(x)			(I2S_BASE + (x))
#define I2S_REG_PHYS(x)  		(I2S_BASE_PHYS + (x))
#define SD2_REG(x)			(SD2_BASE + (x))
#define MS_REG(x)			(MS_BASE + (x))
#define HOST_REG(x)			(HOST_BASE + (x))
#define HIF_REG(x)			(HIF_BASE + (x))
#define ETH_REG(x)			(ETH_BASE + (x))
#define ETH_DMA_REG(x)			(ETH_DMA_BASE + (x))
#define VIC2_REG(x)			(VIC2_BASE + (x))
#define VOUT2_REG(x)			(VOUT2_BASE + (x))
#define VOUT_CLUT_REG(x)		(VOUT_CLUT_BASE + (x))
#define DMA_FIOS_REG(x)			(DMA_FIOS_BASE + (x))
#define DMA2_REG(x)			(DMA2_BASE + (x))
#define HDMI_REG(x)			(HDMI_BASE + (x))
#define TS_REG(x)			(TS_BASE + (x))
#define SPIB_REG(x)			(SPIB_BASE + (x))
#define CRYPT_UNIT_REG(x)		(CRYPT_UNIT_BASE + (x))
#define GRAPHICS_DMA_REG(x)		(GRAPHICS_DMA_BASE + (x))
#define ETH2_REG(x)			(ETH2_BASE + (x))
#define ETH2_DMA_REG(x)			(ETH2_DMA_BASE + (x))
#define FACE_DETECTION_REG(x)		(FACE_DETECTION_BASE + (x))
#define SSI_DMA_REG(x)			(SSI_DMA_BASE + (x))
#define AHB_SECURE_SCRATCHPAD_REG(x)	(AHB_SECURE_SCRATCHPAD_BASE + (x))
#define USB_HOST_CTRL_EHCI_REG(x)	(USB_HOST_CTRL_EHCI_BASE + (x))
#define USB_HOST_CTRL_OHCI_REG(x)	(USB_HOST_CTRL_OHCI_BASE + (x))
#define AHB_SCRATCHPAD_REG(x)		(AHB_SCRATCHPAD_BASE + (x))
#define SATA_REG(x)			(SATA_BASE + (x))
#define SDXC_REG(x)			(SDXC_BASE + (x))
#define VIC3_REG(x)			(VIC3_BASE + (x))
#define SPDIF_REG(x)			(SPDIF_BASE + (x))
#define AHB_SECURE_REG(x)		(AHB_SECURE_BASE + (x))

/*----------------------------------------------------------------------------*/

/* APB slave offsets */

#define TSSI_OFFSET			0x1000
#define SSI_OFFSET			0x2000
#define SPI_OFFSET			0x2000
#define IDC_OFFSET			0x3000
#define STEPPER_OFFSET			0x4000
#define UART_OFFSET			0x5000
#define IR_OFFSET			0x6000
#define IDC2_OFFSET			0x7000
#define PWM_OFFSET			0x8000
#define GPIO0_OFFSET			0x9000
#define GPIO1_OFFSET			0xa000
#define TIMER_OFFSET			0xb000
#define WDOG_OFFSET			0xc000
#define ADC_OFFSET			0xd000
#define RTC_OFFSET			0xd000
#define GPIO2_OFFSET			0xe000
#define SPI2_OFFSET			0xf000

#if (CHIP_REV == A7) || (CHIP_REV == I1)
#define GPIO3_OFFSET			0x10000
#elif (CHIP_REV == A7L)
#define GPIO3_OFFSET			0x1e000
#else
#define GPIO3_OFFSET			0x1f000
#endif

#define GPIO4_OFFSET			0x11000
#define GPIO5_OFFSET			0x12000 /* iONE */
#define UART2_OFFSET			0x13000 /* iONE */
#define UART3_OFFSET			0x14000 /* iONE */

#if (CHIP_REV == I1)
#define SPI3_OFFSET			0x15000
#else
#define SPI3_OFFSET			0x12000
#endif

#define SPI4_OFFSET			0x16000 /* iONE */
#define SATA_PHY_OFFSET			0x19000 /* iONE */

#if (CHIP_REV == A7L)
#define SPI_SLAVE_OFFSET		0x1000
#else
#define SPI_SLAVE_OFFSET		0x1e000
#endif

#define UART1_OFFSET			0x1f000
#define IDCS_OFFSET			0x12000

#if	defined(__A1_FPGA__)
#define RCT_OFFSET			0xf000
#else
#define RCT_OFFSET			0x170000
#endif
#define AUC_OFFSET			0x180000

/* APB slave base addresses */

#define TSSI_BASE			(APB_BASE + TSSI_OFFSET)
#define SSI_BASE			(APB_BASE + SPI_OFFSET)
#define SPI_BASE			(APB_BASE + SPI_OFFSET)
#define IDC_BASE			(APB_BASE + IDC_OFFSET)
#define STEPPER_BASE			(APB_BASE + STEPPER_OFFSET)
#define UART_BASE			(APB_BASE + UART_OFFSET)
#define IR_BASE				(APB_BASE + IR_OFFSET)
#define IDC2_BASE			(APB_BASE + IDC2_OFFSET)
#define PWM_BASE			(APB_BASE + PWM_OFFSET)
#define GPIO0_BASE			(APB_BASE + GPIO0_OFFSET)
#define GPIO1_BASE			(APB_BASE + GPIO1_OFFSET)
#define TIMER_BASE			(APB_BASE + TIMER_OFFSET)
#define WDOG_BASE			(APB_BASE + WDOG_OFFSET)
#define ADC_BASE			(APB_BASE + ADC_OFFSET)
#define RTC_BASE			(APB_BASE + RTC_OFFSET)
#define GPIO2_BASE			(APB_BASE + GPIO2_OFFSET)
#define SPI2_BASE			(APB_BASE + SPI2_OFFSET)
#define GPIO3_BASE			(APB_BASE + GPIO3_OFFSET)
#define RCT_BASE			(APB_BASE + RCT_OFFSET)
#define AUC_BASE			(APB_BASE + AUC_OFFSET)
#define SPI_SLAVE_BASE			(APB_BASE + SPI_SLAVE_OFFSET)
#define UART1_BASE			(APB_BASE + UART1_OFFSET)
#define GPIO4_BASE			(APB_BASE + GPIO4_OFFSET)
#define SPI3_BASE			(APB_BASE + SPI3_OFFSET)
#define IDCS_BASE			(APB_BASE + IDCS_OFFSET)
#define GPIO5_BASE			(APB_BASE + GPIO5_OFFSET)
#define UART2_BASE			(APB_BASE + UART2_OFFSET)
#define UART3_BASE			(APB_BASE + UART3_OFFSET)
#define SPI4_BASE			(APB_BASE + SPI4_OFFSET)
#define SATA_PHY_BASE			(APB_BASE + SATA_PHY_OFFSET)

/* APB slave registers */

#define TSSI_REG(x)			(TSSI_BASE + (x))
#define SPI_REG(x)			(SPI_BASE + (x))
#define SSI_REG(x)			(SPI_BASE + (x))
#define IDC_REG(x)			(IDC_BASE + (x))
#define ST_REG(x)			(STEPPER_BASE + (x))
#define UART_REG(x)			(UART_BASE + (x))
#define UART0_REG(x)			(UART_BASE + (x))
#define IR_REG(x)			(IR_BASE + (x))
#define IDC2_REG(x)			(IDC2_BASE + (x))
#define PWM_REG(x)			(PWM_BASE + (x))
#define GPIO0_REG(x)			(GPIO0_BASE + (x))
#define GPIO1_REG(x)			(GPIO1_BASE + (x))
#define TIMER_REG(x)			(TIMER_BASE + (x))
#define WDOG_REG(x)			(WDOG_BASE + (x))
#define ADC_REG(x)			(ADC_BASE + (x))
#define RTC_REG(x)			(RTC_BASE + (x))
#define GPIO2_REG(x)			(GPIO2_BASE + (x))
#define SPI2_REG(x)			(SPI2_BASE + (x))
#define GPIO3_REG(x)			(GPIO3_BASE + (x))
#define RCT_REG(x)			(RCT_BASE + (x))
#define AUC_REG(x)			(AUC_BASE + (x))
#define SPI_SLAVE_REG(x)		(SPI_SLAVE_BASE + (x))
#define UART1_REG(x)			(UART1_BASE + (x))
#define GPIO4_REG(x)			(GPIO4_BASE + (x))
#define SPI3_REG(x)			(SPI3_BASE + (x))
#define IDCS_REG(x)			(IDCS_BASE + (x))
#define GPIO5_REG(x)			(GPIO5_BASE + (x))
#define UART2_REG(x)			(UART2_BASE + (x))
#define UART3_REG(x)			(UART3_BASE + (x))
#define SPI4_REG(x)			(SPI4_BASE + (x))
#define SATA_PHY_REG(x)			(SATA_PHY_BASE + (x))


/* DSP Debug ports */

#define DSP_DEBUG0_OFFSET               0x100000
#define DSP_DEBUG1_OFFSET               0x110000
#define DSP_DEBUG2_OFFSET               0x120000
#define DSP_DEBUG3_OFFSET               0x130000
#define DSP_DEBUG4_OFFSET               0x140000
#define DSP_DEBUG5_OFFSET               0x150000
#define DSP_DEBUG6_OFFSET               0x160000
#define DSP_DEBUG7_OFFSET               0x170000

#define DSP_DEBUG0_BASE                 (APB_BASE + DSP_DEBUG0_OFFSET)
#define DSP_DEBUG1_BASE                 (APB_BASE + DSP_DEBUG1_OFFSET)
#define DSP_DEBUG2_BASE                 (APB_BASE + DSP_DEBUG2_OFFSET)
#define DSP_DEBUG3_BASE                 (APB_BASE + DSP_DEBUG3_OFFSET)
#define DSP_DEBUG4_BASE                 (APB_BASE + DSP_DEBUG4_OFFSET)
#define DSP_DEBUG5_BASE                 (APB_BASE + DSP_DEBUG5_OFFSET)
#define DSP_DEBUG6_BASE                 (APB_BASE + DSP_DEBUG6_OFFSET)
#define DSP_DEBUG7_BASE                 (APB_BASE + DSP_DEBUG7_OFFSET)


#if (CHIP_REV == A3) || (CHIP_REV == A5) || (CHIP_REV == A5S) || \
    (CHIP_REV == A6) || (CHIP_REV == A7) || (CHIP_REV == I1)  || \
    (CHIP_REV == A7L)
#define DSP_VIN_DEBUG_OFFSET		DSP_DEBUG1_OFFSET
#define DSP_VIN_DEBUG_BASE		DSP_DEBUG1_BASE
#define DSP_VIN_DEBUG_REG(x)		(DSP_VIN_DEBUG_BASE + (x))

#else

#define DSP_VIN_DEBUG_OFFSET		VIN_OFFSET
#define DSP_VIN_DEBUG_BASE		VIN_BASE
#define DSP_VIN_DEBUG_REG(x)		(DSP_VIN_DEBUG_BASE + (x))
#endif

/*----------------------------------------------------------------------------*/

/*******************/
/* DSP related ... */
/*******************/

#if (CHIP_REV == A6) || (CHIP_REV == A7) || (CHIP_REV == I1)

#define ME_OFFSET			0x140000
#define MDXF_OFFSET			0x150000
#define CODE_OFFSET			0x160000
#define ME_BASE				(APB_BASE + ME_OFFSET)
#define MDXF_BASE			(APB_BASE + MDXF_OFFSET)
#define CODE_BASE			(APB_BASE + CODE_OFFSET)

#define DSP_DRAM_MAIN_OFFSET		0x0008
#define DSP_DRAM_SUB0_OFFSET		0x0008
#define DSP_DRAM_SUB1_OFFSET		0x0008
#define DSP_CONFIG_MAIN_OFFSET		0x0000
#define DSP_CONFIG_SUB0_OFFSET		0x0000
#define DSP_CONFIG_SUB1_OFFSET		0x0000

#define DSP_DRAM_MAIN_REG		(CODE_BASE + DSP_DRAM_MAIN_OFFSET) /* CODE */
#define DSP_DRAM_SUB0_REG		(ME_BASE   + DSP_DRAM_SUB0_OFFSET) /* ME */
#define DSP_DRAM_SUB1_REG		(MDXF_BASE + DSP_DRAM_SUB1_OFFSET) /* MD */
#define DSP_CONFIG_MAIN_REG		(CODE_BASE + DSP_CONFIG_MAIN_OFFSET)
#define DSP_CONFIG_SUB0_REG		(ME_BASE   + DSP_CONFIG_SUB0_OFFSET)
#define DSP_CONFIG_SUB1_REG		(MDXF_BASE + DSP_CONFIG_SUB1_OFFSET)

#else

#define MEMD_OFFSET			0x150000
#define CODE_OFFSET			0x160000
#define MEMD_BASE			(APB_BASE + MEMD_OFFSET)
#define CODE_BASE			(APB_BASE + CODE_OFFSET)

#define DSP_DRAM_MAIN_OFFSET		0x0008
#define DSP_DRAM_SUB0_OFFSET		0x0008
#define DSP_DRAM_SUB1_OFFSET		0x8008
#define DSP_CONFIG_MAIN_OFFSET		0x0000
#define DSP_CONFIG_SUB0_OFFSET		0x0000
#define DSP_CONFIG_SUB1_OFFSET		0x8000

#define DSP_DRAM_MAIN_REG		(CODE_BASE + DSP_DRAM_MAIN_OFFSET) /* CODE */
#define DSP_DRAM_SUB0_REG		(MEMD_BASE + DSP_DRAM_SUB0_OFFSET) /* ME */
#define DSP_DRAM_SUB1_REG		(MEMD_BASE + DSP_DRAM_SUB1_OFFSET) /* MD */
#define DSP_CONFIG_MAIN_REG		(CODE_BASE + DSP_CONFIG_MAIN_OFFSET)
#define DSP_CONFIG_SUB0_REG		(MEMD_BASE + DSP_CONFIG_SUB0_OFFSET)
#define DSP_CONFIG_SUB1_REG		(MEMD_BASE + DSP_CONFIG_SUB1_OFFSET)

#endif


#if ((CHIP_REV == A1) || (CHIP_REV == A2) || (CHIP_REV == A2S) || \
     (CHIP_REV == A2M) || (CHIP_REV == A2Q) || (CHIP_REV == A5L))

#define DSP_TEXT_BASE_ADDR_CORE		0xf0000000
#define DSP_TEXT_BASE_ADDR_MDXF		0xf0000000
#define DSP_TEXT_BASE_ADDR_MEMD		0xf0000000

#elif (CHIP_REV == I1)

#define DSP_TEXT_BASE_ADDR_CORE		0x1900000
#define DSP_TEXT_BASE_ADDR_MDXF		0x1600000
#define DSP_TEXT_BASE_ADDR_MEMD		0x1300000

#else

#define DSP_TEXT_BASE_ADDR_CORE		0x900000
#define DSP_TEXT_BASE_ADDR_MDXF		0x600000
#define DSP_TEXT_BASE_ADDR_MEMD		0x300000

#endif


/*----------------------------------------------------------------------------*/

/* Cryptography enginer offset */
#define CRYPT_OFFSET	0x0
#define CRTYP_REG(x)	(CRYPT_BASE + (x))

#endif
