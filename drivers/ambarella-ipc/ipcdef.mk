# Servers
X-S-y 						+=	lk_util.x \
							lk_sdhc.x \
							lk_gpio.x \
							lk_pmic.x \
							lk_vfs.x \
							lk_fifo.x \
							lk_host_dsp.x \
							lk_sdresp.x \
							lk_example_util.x

# Clients
X-C-y 						+=	i_util.x \
							i_status.x \
							i_flpart.x \
							i_nand.x \
							i_sdhc.x \
							i_gpio.x \
							i_pmic.x \
							i_ffs.x \
							i_fifo.x \
							i_sdresp.x \
							i_host_dsp.x \
							i_example_util.x \
							i_display.x \
							i_streamer.x \
							i_preview.x

X-S-$(CONFIG_VIRTUAL_SERIAL_AMBARELLA)		+=	lk_vserial.x

X-S-$(CONFIG_TOUCHSCREEN_AMBA_VTOUCH)		+=	lk_touch.x
X-C-$(CONFIG_TOUCHSCREEN_AMBA_VTOUCH)		+=	i_touch.x

X-S-$(CONFIG_INPUT_AMBA_VBUTTON)		+=	lk_button.x
X-C-$(CONFIG_INPUT_AMBA_VBUTTON)		+=	i_button.x

X-S-$(CONFIG_SOUND)				+=	lk_alsa_fifo.x
X-C-$(CONFIG_SOUND)				+=	i_alsa_op.x

X-C-$(CONFIG_AMBARELLA_AUDIOMEM)		+=	i_audio2_api.x

X-C-$(CONFIG_AMBARELLA_HEAPMEM)		+=	i_heapmem.x

X-S-$(CONFIG_RTC_DRV_AMBARELLA)			+= 	lk_rtc.x
ifeq ($(CONFIG_PLAT_AMBARELLA_CORTEX), y)
X-C-$(CONFIG_RTC_DRV_AMBARELLA)			+= 	i_rtc.x
endif
