/*
 * drivers/sensor/mma7455l.c
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
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

#include "sensor.h"

#define MMA7455L_SPI_MODE	0
#define MMA7455L_SPI_BPW	8
#define MMA7455L_SPI_BAUDRATE	(1 * 1000 * 1000)

#define MMA7455L_INT1_GPIO	49
#define MMA7455L_INT2_GPIO	48

#define MMA7455L_IRQ1		gpio_to_irq(MMA7455L_INT1_GPIO)
#define MMA7455L_IRQ2		gpio_to_irq(MMA7455L_INT2_GPIO)

enum mma7455l_regs
{
	MMA7455L_XOUTL		= 0x00,
	MMA7455L_XOUTH,
	MMA7455L_YOUTL,
	MMA7455L_YOUTH,
	MMA7455L_ZOUTL,
	MMA7455L_ZOUTH,
	MMA7455L_XOUT8,
	MMA7455L_YOUT8,
	MMA7455L_ZOUT8,
	MMA7455L_STATUS,
	MMA7455L_DETSRC,
	MMA7455L_TOUT,
	MMA7455L_RESERVED,
	MMA7455L_I2CAD,
	MMA7455L_USRINF,
	MMA7455L_WHOAMI,
	MMA7455L_XOFFL,
	MMA7455L_XOFFH,
	MMA7455L_YOFFL,
	MMA7455L_YOFFH,
	MMA7455L_ZOFFL,
	MMA7455L_ZOFFH,
	MMA7455L_MCTL,
	MMA7455L_INTRST,
	MMA7455L_CTL1,
	MMA7455L_CTL2,
	MMA7455L_LDTH,
	MMA7455L_PDTH,
	MMA7455L_PW,
	MMA7455L_LT,
	MMA7455L_TW,
};

struct mma7455l {
	struct spi_device 		*spidev;
	struct workqueue_struct 	*workqueue;
	struct work_struct		worker;
	int				irq;
};

static int mma7455l_write_reg(struct spi_device *spi,
	unsigned char reg, unsigned char val)
{
	unsigned char		buf[2];

	buf[0]			= 0x80 | (reg << 1);
	buf[1]			= val;

	return spi_write(spi, buf, sizeof(buf));
}

static int mma7455l_read_reg(struct spi_device *spi,
	unsigned char reg, unsigned char *val)
{
	reg <<= 1;

	return spi_write_then_read(spi, &reg, 1, val, 1);
}

static void mma7455l_report_worker(struct work_struct *work)
{
	int			errorCode = 0;
	struct mma7455l		*pm = container_of(work, struct mma7455l, worker);
	struct irq_desc		*mma7455l_desc;
	struct irq_chip		*mma7455l_chip = NULL;
	struct timeval		time;
	struct amb_event	event;
	u8			xl, xh, yl, yh, zl, zh;
	int			buf[3];

	msleep(300);
	do_gettimeofday(&time);

	mma7455l_read_reg(pm->spidev, MMA7455L_XOUTL, &xl);
	mma7455l_read_reg(pm->spidev, MMA7455L_XOUTH, &xh);
	mma7455l_read_reg(pm->spidev, MMA7455L_YOUTL, &yl);
	mma7455l_read_reg(pm->spidev, MMA7455L_YOUTH, &yh);
	mma7455l_read_reg(pm->spidev, MMA7455L_ZOUTL, &zl);
	mma7455l_read_reg(pm->spidev, MMA7455L_ZOUTH, &zh);

	if(xh & 0x02) {
		xh |= 0xfc;
	}
	if(yh & 0x02) {
		yh |= 0xfc;
	}
	if(zh & 0x02) {
		zh |= 0xfc;
	}

	buf[0] = (s16) ((xh << 8) | xl);
	buf[1] = (s16) ((yh << 8) | yl);
	buf[2] = (s16) ((zh << 8) | zl);

	SENSOR_DEBUG("%s: %d %d %d\n", __func__, buf[0], buf[1], buf[2]);

	event.type	= AMB_EV_ACCELEROMETER_REPORT;
	event.time_code	= time.tv_sec * 1000000000 + time.tv_usec * 1000000;
	memcpy(event.data, buf, sizeof(buf));
	errorCode = amb_event_pool_affuse(&sensor_event_pool, event);
	if (!errorCode && sensor_event_proc.fasync_queue) {
		kill_fasync(&sensor_event_proc.fasync_queue, SIGIO, POLL_IN);
	}

	mma7455l_write_reg(pm->spidev, MMA7455L_INTRST, 0x03);
	mma7455l_desc = irq_to_desc(pm->irq);
	if (mma7455l_desc)
		mma7455l_chip = get_irq_desc_chip(mma7455l_desc);
	if (mma7455l_chip && mma7455l_chip->irq_ack)
		mma7455l_chip->irq_ack(&mma7455l_desc->irq_data);
	mma7455l_write_reg(pm->spidev, MMA7455L_INTRST, 0x00);
	enable_irq(pm->irq);
}

static irqreturn_t mma7455l_irq(int irq, void *dev_data)
{
	struct mma7455l *pm = (struct mma7455l *)dev_data;

	disable_irq_nosync(pm->irq);
	queue_work(pm->workqueue, &pm->worker);

	return IRQ_HANDLED;
}

static int mma7455l_spi_probe(struct spi_device *spi)
{
	int			errorCode = 0;
	struct mma7455l		*pm;

	pm	= kzalloc(sizeof(*pm), GFP_KERNEL);
	if (!pm) {
		errorCode = -ENOMEM;
		goto mma7455l_spi_probe_exit;
	}

	spi_set_drvdata(spi, pm);

	spi->mode		= MMA7455L_SPI_MODE;
	spi->bits_per_word	= MMA7455L_SPI_BPW;
	spi->max_speed_hz	= MMA7455L_SPI_BAUDRATE;
	errorCode = spi_setup(spi);
	if (errorCode) {
		goto mma7455l_spi_probe_exit;
	}

	/* 8g Measurement mode */
	errorCode = mma7455l_write_reg(spi, MMA7455L_MCTL, 0x01);
	if (errorCode) {
		goto mma7455l_spi_probe_exit;
	}

	/* x, y, z offset */
	errorCode = mma7455l_write_reg(spi, MMA7455L_XOFFL, 0x0a);
	if (errorCode) {
		goto mma7455l_spi_probe_exit;
	}

	errorCode = mma7455l_write_reg(spi, MMA7455L_XOFFH, 0x00);
	if (errorCode) {
		goto mma7455l_spi_probe_exit;
	}

	errorCode = mma7455l_write_reg(spi, MMA7455L_YOFFL, 0x2c);
	if (errorCode) {
		goto mma7455l_spi_probe_exit;
	}

	errorCode = mma7455l_write_reg(spi, MMA7455L_YOFFH, 0x00);
	if (errorCode) {
		goto mma7455l_spi_probe_exit;
	}

	errorCode = mma7455l_write_reg(spi, MMA7455L_ZOFFL, 0x00);
	if (errorCode) {
		goto mma7455l_spi_probe_exit;
	}

	errorCode = mma7455l_write_reg(spi, MMA7455L_ZOFFH, 0x00);
	if (errorCode) {
		goto mma7455l_spi_probe_exit;
	}

	pm->workqueue = create_singlethread_workqueue("mma7455l");
	if (!pm->workqueue) {
		goto mma7455l_spi_probe_exit;
	}
	INIT_WORK(&pm->worker, mma7455l_report_worker);

	pm->spidev	= spi;
	pm->irq		= MMA7455L_IRQ1;
	ambarella_gpio_config(MMA7455L_INT1_GPIO, GPIO_FUNC_SW_INPUT);
	errorCode = request_irq(pm->irq, mma7455l_irq,
			IRQF_TRIGGER_RISING, "mma7455l", pm);
	if (errorCode) {
		destroy_workqueue(pm->workqueue);
		pm->workqueue = NULL;
		goto mma7455l_spi_probe_exit;
	}

	return 0;

mma7455l_spi_probe_exit:
	if (pm) {
		kfree(pm);
	}
	return errorCode;
}

static int __devexit mma7455l_spi_remove(struct spi_device *spi)
{
	struct mma7455l		*pm = spi_get_drvdata(spi);

	free_irq(MMA7455L_IRQ2, &pm->worker);
	if (pm->workqueue) {
		destroy_workqueue(pm->workqueue);
		pm->workqueue = NULL;
	}

	return 0;
}

static struct spi_driver mma7455l_spi_driver = {
	.driver = {
		.name	= "mma7455l",
		.bus	= &spi_bus_type,
		.owner	= THIS_MODULE,
	},
	.probe		= mma7455l_spi_probe,
	.remove		= __devexit_p(mma7455l_spi_remove),
};

static int __init mma7455l_init(void)
{
	return spi_register_driver(&mma7455l_spi_driver);
}

static void __exit mma7455l_exit(void)
{
	spi_unregister_driver(&mma7455l_spi_driver);
}

module_init(mma7455l_init);
module_exit(mma7455l_exit);

MODULE_DESCRIPTION("FreeScale MMA7455L Sensor Driver");
MODULE_AUTHOR("Zhenwu Xue, <zwxue@ambarella.com>");
MODULE_LICENSE("GPL");
