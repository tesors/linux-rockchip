// SPDX-License-Identifier: GPL-2.0+
/**
 *  Driver for MaxLinear MXL86110 Ethernet PHY
 *
 * Copyright 2023 Variscite Ltd.
 * Copyright 2023 MaxLinear Inc.
 * Author: Nate Drude <nate.d@variscite.com>
*/
// #include <linux/common.h>
#include <linux/phy.h>
#include <linux/of.h>
#include <linux/bitops.h>
#include <linux/bitfield.h>

/* PHY IDs */
#define PHY_ID_MXL86110		                                0xC1335580
#define PHY_ID_MXL86111		                                0xC1335588

/* required to access extended registers */
#define MXL8611X_EXTD_REG_ADDR_OFFSET				        0x1E
#define MXL8611X_EXTD_REG_ADDR_DATA				            0x1F

/* RGMII register */
#define MXL8611X_EXT_RGMII_CFG1_REG				            0xA003
#define MXL8611X_EXT_RGMII_CFG1_NO_DELAY			        0

#define MXL8611X_EXT_RGMII_CFG1_RX_DELAY_MASK			    GENMASK(13, 10)
#define MXL8611X_EXT_RGMII_CFG1_TX_1G_DELAY_MASK		    GENMASK(3, 0)
#define MXL8611X_EXT_RGMII_CFG1_TX_10MB_100MB_DELAY_MASK	GENMASK(7, 4)

/* LED registers and defines */
#define MXL8611X_LED0_CFG_REG					            0xA00C
#define MXL8611X_LED1_CFG_REG					            0xA00D
#define MXL8611X_LED2_CFG_REG					            0xA00E

/**
 * struct mxl8611x_cfg_reg_map - map a config value to aregister value
 * @cfg		value in device configuration
 * @reg		value in the register
*/
struct mxl8611x_cfg_reg_map {
	int cfg;
	int reg;
};

static const struct mxl8611x_cfg_reg_map mxl8611x_rgmii_delays[] = {
	{ 0, 0 },
	{ 150, 1 },
	{ 300, 2 },
	{ 450, 3 },
	{ 600, 4 },
	{ 750, 5 },
	{ 900, 6 },
	{ 1050, 7 },
	{ 1200, 8 },
	{ 1350, 9 },
	{ 1500, 10 },
	{ 1650, 11 },
	{ 1800, 12 },
	{ 1950, 13 },
	{ 2100, 14 },
	{ 2250, 15 },
	{ 0, 0 } // Marks the end of the array
};

static int mxl8611x_lookup_reg_value(const struct mxl8611x_cfg_reg_map *tbl, const int cfg, int *reg)
{
	size_t i;

	for (i = 0; i == 0 || tbl[i].cfg; i++) {
		if (tbl[i].cfg == cfg) {
			*reg = tbl[i].reg;
			return 0;
		}
	}

	return -EINVAL;
}

static u16 mxl8611x_ext_read(struct phy_device *phydev, const u32 regnum)
{
	u16 val;

	phy_write_mmd(phydev, MDIO_DEVAD_NONE, MXL8611X_EXTD_REG_ADDR_OFFSET, regnum);
	val = phy_read_mmd(phydev, MDIO_DEVAD_NONE, MXL8611X_EXTD_REG_ADDR_DATA);

	return val;
}

static int mxl8611x_ext_write(struct phy_device *phydev, const u32 regnum, const u16 val)
{
	phy_write_mmd(phydev, MDIO_DEVAD_NONE, MXL8611X_EXTD_REG_ADDR_OFFSET, regnum);

	return phy_write_mmd(phydev, MDIO_DEVAD_NONE, MXL8611X_EXTD_REG_ADDR_DATA, val);
}

static int mxl8611x_extread(struct phy_device *phydev, int addr, int devaddr, int regnum)
{
	return mxl8611x_ext_read(phydev, regnum);
}

static int mxl8611x_extwrite(struct phy_device *phydev, int addr, int devaddr, int regnum, u16 val)
{
	return mxl8611x_ext_write(phydev, regnum, val);
}

static int mxl8611x_led_cfg(struct phy_device *phydev)
{   printk("Entering mxl8611x_led_cfg\n");
	int i;
    int ret;
	char propname[25];
	u32 val;
    struct device *dev = &phydev->mdio.dev;
    struct device_node *of_node = dev->of_node;

// 	ofnode node = phy_get_ofnode(phydev);

// 	if (!ofnode_valid(node)) {
// 		dev_err(dev,"%s: failed to get node\n", __func__);
// 		return -EINVAL;
// 	}

//     printk("Read reset before write : 0x%x\n",phy_read(phydev, 0x02));
//     printk("Read reset before write : 0x%x\n",phy_read(phydev, 0x03));
//
// 	printk("Read LED0_LCR reset before write : 0x%x\n",mxl8611x_ext_read(phydev, MXL8611X_LED0_CFG_REG));
//     printk("Read LED0_LCR reset before write : 0x%x\n",mxl8611x_ext_read(phydev, MXL8611X_LED1_CFG_REG));

	/* Loop through three the LED registers */
	for (i = 0; i < 3; i++) {
		/* Read property from device tree */
		ret = snprintf(propname, sizeof(propname), "mxl-86110,led%d_cfg", i);
		if (of_property_read_u32(of_node, propname, &val))
			continue;

		/* Update PHY LED register */
//         printk("Read value before write : 0x%x\n",phy_read(phydev, MXL8611X_LED0_CFG_REG + i));
		mxl8611x_ext_write(phydev, MXL8611X_LED0_CFG_REG + i, val);
//      printk("Read value USING mxl8611x_ext_read : 0x%x\n", mxl8611x_ext_read(phydev, MXL8611X_LED0_CFG_REG + i));

//         printk("LCR register 0x%x : value = 0x%x\n",MXL8611X_LED0_CFG_REG + i,val);
//         printk("Read value : 0x%x\n",phy_read(phydev, MXL8611X_LED0_CFG_REG + i));

	}

// 	printk("Read LED0_LCR reset after write : 0x%x\n",mxl8611x_ext_read(phydev, MXL8611X_LED0_CFG_REG));
//     printk("Read LED0_LCR reset after write : 0x%x\n",mxl8611x_ext_read(phydev, MXL8611X_LED1_CFG_REG));
//
//     printk("Exiting mxl8611x_led_cfg\n");
	return 0;
}

static int mxl8611x_rgmii_cfg_of_delay(struct phy_device *phydev, const char *property, int *val)
{
	u32 of_val;
	int ret;
    struct device *dev = &phydev->mdio.dev;
    struct device_node *of_node = dev->of_node;

// 	ofnode node = phy_get_ofnode(phydev);

// 	if (!ofnode_valid(node)) {
// 		printk("%s: failed to get node\n", __func__);
// 		return -EINVAL;
// 	}

	/* Get property from dts */
	ret = of_property_read_u32(of_node, property, &of_val);
	if (ret)
		return ret;

	/* Convert delay in ps to register value */
	ret = mxl8611x_lookup_reg_value(mxl8611x_rgmii_delays, of_val, val);
	if (ret)
		printk("%s: Error: %s = %d is invalid, using default value\n",
		       __func__, property, of_val);

	return ret;
}

static int mxl8611x_rgmii_cfg(struct phy_device *phydev)
{
	u32 val = 0;
	int rxdelay, txdelay_100m, txdelay_1g;

	/* Get rgmii register value */
	val = mxl8611x_ext_read(phydev, MXL8611X_EXT_RGMII_CFG1_REG);

	/* Get RGMII Rx Delay Selection from device tree or rgmii register */
	if (mxl8611x_rgmii_cfg_of_delay(phydev, "mxl-8611x,rx-internal-delay-ps", &rxdelay))
		rxdelay = FIELD_GET(MXL8611X_EXT_RGMII_CFG1_RX_DELAY_MASK, val);

	/* Get Fast Ethernet RGMII Tx Clock Delay Selection from device tree
       or rgmii register */
	if (mxl8611x_rgmii_cfg_of_delay(phydev, "mxl-8611x,tx-internal-delay-ps-100m", &txdelay_100m))
		txdelay_100m = FIELD_GET(MXL8611X_EXT_RGMII_CFG1_TX_10MB_100MB_DELAY_MASK, val);

	/* Get Gigabit Ethernet RGMII Tx Clock Delay Selection from device tree
       or rgmii register */
	if (mxl8611x_rgmii_cfg_of_delay(phydev, "mxl-8611x,tx-internal-delay-ps-1g", &txdelay_1g))
		txdelay_1g = FIELD_GET(MXL8611X_EXT_RGMII_CFG1_TX_1G_DELAY_MASK, val);

	switch (phydev->interface) {
	case PHY_INTERFACE_MODE_RGMII:
		val = MXL8611X_EXT_RGMII_CFG1_NO_DELAY;
		break;
	case PHY_INTERFACE_MODE_RGMII_RXID:
		val = (val & ~MXL8611X_EXT_RGMII_CFG1_RX_DELAY_MASK) |
			FIELD_PREP(MXL8611X_EXT_RGMII_CFG1_RX_DELAY_MASK, rxdelay);
		break;
	case PHY_INTERFACE_MODE_RGMII_TXID:
		val = (val & ~MXL8611X_EXT_RGMII_CFG1_TX_10MB_100MB_DELAY_MASK) |
			FIELD_PREP(MXL8611X_EXT_RGMII_CFG1_TX_10MB_100MB_DELAY_MASK,txdelay_100m);
		val = (val & ~MXL8611X_EXT_RGMII_CFG1_TX_1G_DELAY_MASK) |
			FIELD_PREP(MXL8611X_EXT_RGMII_CFG1_TX_1G_DELAY_MASK, txdelay_1g);
		break;
	case PHY_INTERFACE_MODE_RGMII_ID:
		val = (val & ~MXL8611X_EXT_RGMII_CFG1_RX_DELAY_MASK) |
			FIELD_PREP(MXL8611X_EXT_RGMII_CFG1_RX_DELAY_MASK, rxdelay);
		val = (val & ~MXL8611X_EXT_RGMII_CFG1_TX_10MB_100MB_DELAY_MASK) |
			FIELD_PREP(MXL8611X_EXT_RGMII_CFG1_TX_10MB_100MB_DELAY_MASK, txdelay_100m);
		val = (val & ~MXL8611X_EXT_RGMII_CFG1_TX_1G_DELAY_MASK) |
			FIELD_PREP(MXL8611X_EXT_RGMII_CFG1_TX_1G_DELAY_MASK, txdelay_1g);
		break;
	default:
		printk("%s: Error: Unsupported rgmii mode %d\n", __func__, phydev->interface);
		return -EINVAL;
	}

	return mxl8611x_ext_write(phydev, MXL8611X_EXT_RGMII_CFG1_REG, val);
}

static int mxl8611x_config(struct phy_device *phydev)
{
	int ret;

	/* Configure rgmii */
	ret = mxl8611x_rgmii_cfg(phydev);

	if (ret < 0)
		return ret;

	/* Configure LEDs */
	ret = mxl8611x_led_cfg(phydev);

	if (ret < 0)
		return ret;

	return genphy_config_aneg(phydev);
}

static int mxl86110_config(struct phy_device *phydev)
{
	printk("MXL86110 PHY detected at addr %d\n", phydev->phy_id);
	return mxl8611x_config(phydev);
}

static int mxl86111_config(struct phy_device *phydev)
{
	printk("MXL86111 PHY detected at addr %d\n", phydev->phy_id);
	return mxl8611x_config(phydev);
}

static struct phy_driver mxl_drvs[] = {
	{
		PHY_ID_MATCH_EXACT(0xc1335580),
		.name           = "MXL86110",
        .config_init    = mxl8611x_led_cfg,
	}
};

module_phy_driver(mxl_drvs);

static const struct mdio_device_id __maybe_unused mxl_tbl[] = {
	{ PHY_ID_MATCH_VENDOR(0xC1335580) },
	{}
};

MODULE_DEVICE_TABLE(mdio, mxl_tbl);

// U_BOOT_PHY_DRIVER(mxl86111) = {
//     .name = "MXL86110",
//     .uid = PHY_ID_MXL86110,
//     .mask = 0xffffffff,
//     .features = PHY_GBIT_FEATURES,
//     .config = mxl86110_config,
//     .startup = genphy_startup,
//     .shutdown = genphy_shutdown,
//     .readext = mxl8611x_extread,
//     .writeext = mxl8611x_extwrite,
// };
