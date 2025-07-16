// SPDX-License-Identifier: GPL-2.0+
/*
 * FB driver for the st7315 LCD Controller
 *
 * Copyright (C) 2025 Your Name
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>

#include "fbtft.h"

#define DRVNAME		"fb_st7315"
#define WIDTH		128
#define HEIGHT		64

/**
 * Initialize the st7315 display.
 *
 * This function resets the display and sets up the initial configuration
 * by sending a sequence of commands to the st7315 controller.
 * It configures display clock, multiplex ratio, offset, charge pump,
 * memory addressing mode, segment remap, COM output scan direction,
 * COM pins hardware configuration, pre-charge period, VCOMH deselect level,
 * display mode, and finally turns the display on.
 *
 * @param par Pointer to the fbtft_par structure containing device info.
 * @return 0 on success.
 */
static int init_display(struct fbtft_par *par)
{
	par->fbtftops.reset(par);  // Reset the display hardware
	par->fbtftops.reset(par);  // Reset the display hardware
	// Initialize gamma curve if not already set
	if (par->gamma.curves[0] == 0) {
		mutex_lock(&par->gamma.lock);
		if (par->info->var.yres == 64)
			par->gamma.curves[0] = 0xCF;  // Gamma value for 64-row display
		else
			par->gamma.curves[0] = 0x8F;  // Gamma value for other sizes
		mutex_unlock(&par->gamma.lock);
	}

	/* Set Display OFF */
	write_reg(par, 0xAE); /*display off*/

	write_reg(par, 0x00); /*set lower column address*/
	write_reg(par, 0x10); /*set higher column address*/

	write_reg(par, 0x40); /*set display start line*/

	write_reg(par, 0xB0); /*set page address*/

	write_reg(par, 0x81); /*contract control*/
	write_reg(par, 0x8f); /*128*/

	write_reg(par, 0xA1); /*set segment remap*/

	write_reg(par, 0xA6); /*normal / reverse*/

	write_reg(par, 0xA8); /*multiplex ratio*/
	write_reg(par, 0x3F); /*duty = 1/64*/

	write_reg(par, 0xC8); /*Com scan direction*/

	write_reg(par, 0xD3); /*set display offset*/
	write_reg(par, 0x00);

	write_reg(par, 0xD5); /*set osc division*/
	write_reg(par, 0x80);

	write_reg(par, 0xD9); /*set pre-charge period*/
	write_reg(par, 0x22);

	write_reg(par, 0xDA); /*set COM pins*/
	write_reg(par, 0x12);

	write_reg(par, 0xdb); /*set vcomh*/
	write_reg(par, 0x30);

	write_reg(par, 0x8d); /*set charge pump disable*/
	write_reg(par, 0x10);

	write_reg(par, 0xAF); /*display ON*/
	return 0;
}

/**
 * Set the address window for subsequent data writes.
 *
 * This function sets the column and page start addresses to zero,
 * and sets the display start line to zero.
 * It prepares the display controller to receive data starting from the top-left corner.
 *
 * @param par Pointer to the fbtft_par structure containing device info.
 * @param xs Starting column (ignored, set to 0)
 * @param ys Starting page (ignored, set to 0)
 * @param xe Ending column (ignored)
 * @param ye Ending page (ignored)
 */
static void set_addr_win(struct fbtft_par *par, int xs, int ys, int xe, int ye)
{
	/* Set Lower Column Start Address for Page Addressing Mode */
	write_reg(par, 0x00 | 0x0);
	/* Set Higher Column Start Address for Page Addressing Mode */
	write_reg(par, 0x10 | 0x0);
	/* Set Display Start Line */
	write_reg(par, 0x40 | 0x0);

}


/**
 * Enable or disable blanking (turn display on or off).
 *
 * This function controls the display blanking by sending the appropriate
 * command to the st7315 controller. When 'on' is true, the display is turned off;
 * when 'on' is false, the display is turned on.
 *
 * @param par Pointer to the fbtft_par structure containing device info.
 * @param on Boolean flag indicating whether to blank (true) or unblank (false) the display.
 * @return 0 on success.
 */
static int blank(struct fbtft_par *par, bool on)
{
	fbtft_par_dbg(DEBUG_BLANK, par, "(%s=%s)\n",
		      __func__, on ? "true" : "false");

	if (on)
		write_reg(par, 0xAE);  // Display OFF command
	else
		write_reg(par, 0xAF);  // Display ON command
	return 0;
}

/**
 * Set the gamma correction curve.
 *
 * This function applies a mask to the first gamma curve value and then
 * sends the contrast control command along with the gamma value to the display.
 *
 * @param par Pointer to the fbtft_par structure containing device info.
 * @param curves Pointer to an array of gamma curve values.
 * @return 0 on success.
 */
static int set_gamma(struct fbtft_par *par, u32 *curves)
{
	/* Apply mask to ensure gamma value fits in 8 bits */
	curves[0] &= 0xFF;

	/* Set Contrast Control for BANK0 */
	write_reg(par, 0x81);
	write_reg(par, curves[0]);

	return 0;
}

/**
 * Write video memory to the display.
 *
 * This function converts the framebuffer data from a 16-bit format to the
 * display's expected 1-bit-per-pixel format, packing 8 vertical pixels into
 * one byte. It then sends the converted data to the display via the write
 * operation defined in fbtftops.
 *
 * @param par Pointer to the fbtft_par structure containing device info.
 * @param offset Offset in the framebuffer (unused here).
 * @param len Length of data to write (unused here).
 * @return 0 on success, negative error code on failure.
 */
static int write_vmem(struct fbtft_par *par, size_t offset, size_t len)
{
	u16 *vmem16 = (u16 *)par->info->screen_buffer;  // Pointer to 16-bit framebuffer
	u32 xres = par->info->var.xres;                  // Horizontal resolution
	u32 yres = par->info->var.yres;                  // Vertical resolution
	u8 *buf = par->txbuf.buf;                         // Transmit buffer
	int x, y, i;
	int ret = 0;

	// Convert framebuffer data to display format: 8 vertical pixels per byte
	for (x = 0; x < xres; x++) {
		for (y = 0; y < yres / 8; y++) {
			*buf = 0x00;
			for (i = 0; i < 8; i++)
				if (vmem16[(y * 8 + i) * xres + x])
					*buf |= BIT(i);
			buf++;
		}
	}

	// Set DC line to data mode before writing
	gpiod_set_value(par->gpio.dc, 1);

	// Write the converted buffer to the display
	ret = par->fbtftops.write(par, par->txbuf.buf, xres * yres / 8);
	if (ret < 0)
		dev_err(par->info->device, "write failed and returned: %d\n",
			ret);

	return ret;
}

static struct fbtft_display display = {
	.regwidth = 8,
	.width = WIDTH,
	.height = HEIGHT,
	.gamma_num = 1,
	.gamma_len = 1,
	.gamma = "00",
	.fbtftops = {
		.write_vmem = write_vmem,
		.init_display = init_display,
		.set_addr_win = set_addr_win,
		.blank = blank,
		.set_gamma = set_gamma,
	},
};

FBTFT_REGISTER_DRIVER(DRVNAME, "sitronix,st7315", &display);

MODULE_ALIAS("spi:" DRVNAME);
MODULE_ALIAS("platform:" DRVNAME);
MODULE_ALIAS("spi:st7315");
MODULE_ALIAS("platform:st7315");

MODULE_DESCRIPTION("st7315 LCD Driver");
MODULE_AUTHOR("pksg");
MODULE_LICENSE("GPL");
