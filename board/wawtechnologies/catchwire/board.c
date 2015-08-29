/*
 * board.c
 *
 * Board functions for TI AM335X based boards
 *
 * Copyright (C) 2011, Texas Instruments, Incorporated - http://www.ti.com/
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <spl.h>
#include <asm/arch/cpu.h>
#include <asm/arch/hardware.h>
#include <asm/arch/omap.h>
#include <asm/arch/ddr_defs.h>
#include <asm/arch/clock.h>
#include <asm/arch/gpio.h>
#include <asm/arch/mmc_host_def.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/mem.h>
#include <asm/io.h>
#include <asm/emif.h>
#include <asm/gpio.h>
#include <i2c.h>
#include <miiphy.h>
#include <cpsw.h>
#include <power/tps65217.h>
#include <power/tps65910.h>
#include <environment.h>
#include <watchdog.h>
#include "board.h"

DECLARE_GLOBAL_DATA_PTR;

/* GPIO that controls power to DDR on EVM-SK */
#define GPIO_DDR_VTT_EN		7

/* GPIO that controls power of LCD backlight */
#define GPIO_LCD_BKLT_EN         54

/* GPIO that controls LCD backlight PWM */
#define GPIO_LCD_PWM_EN         7

static struct ctrl_dev *cdev = (struct ctrl_dev *)CTRL_DEVICE_BASE;

/*
 * Read header information from EEPROM into global structure.
 */
static int read_eeprom(struct am335x_baseboard_id *header)
{
	/* Check if baseboard eeprom is available */
	if (i2c_probe(CONFIG_SYS_I2C_EEPROM_ADDR)) {
		puts("Could not probe the EEPROM; something fundamentally "
			"wrong on the I2C bus.\n");
		return -ENODEV;
	}

	/* read the eeprom using i2c */
	if (i2c_read(CONFIG_SYS_I2C_EEPROM_ADDR, 0, 2, (uchar *)header,
		     sizeof(struct am335x_baseboard_id))) {
		puts("Could not read the EEPROM; something fundamentally"
			" wrong on the I2C bus.\n");
		return -EIO;
	}

	if (header->magic != 0xEE3355AA) {
		/*
		 * read the eeprom using i2c again,
		 * but use only a 1 byte address
		 */
		if (i2c_read(CONFIG_SYS_I2C_EEPROM_ADDR, 0, 1, (uchar *)header,
			     sizeof(struct am335x_baseboard_id))) {
			puts("Could not read the EEPROM; something "
				"fundamentally wrong on the I2C bus.\n");
			return -EIO;
		}

		if (header->magic != 0xEE3355AA) {
			printf("Incorrect magic number (0x%x) in EEPROM\n",
					header->magic);
			return -EINVAL;
		}
	}

	return 0;
}

#if defined(CONFIG_SPL_BUILD) || defined(CONFIG_NOR_BOOT)

static const struct ddr_data ddr3_catchwire_data = {
        .datardsratio0 = MT41K256M16HA125E_RD_DQS,
        .datawdsratio0 = MT41K256M16HA125E_WR_DQS,
        .datafwsratio0 = MT41K256M16HA125E_PHY_FIFO_WE,
        .datawrsratio0 = MT41K256M16HA125E_PHY_WR_DATA,
};

static const struct cmd_control ddr3_catchwire_cmd_ctrl_data = {
        .cmd0csratio = MT41K256M16HA125E_RATIO,
        .cmd0iclkout = MT41K256M16HA125E_INVERT_CLKOUT,

        .cmd1csratio = MT41K256M16HA125E_RATIO,
        .cmd1iclkout = MT41K256M16HA125E_INVERT_CLKOUT,

        .cmd2csratio = MT41K256M16HA125E_RATIO,
        .cmd2iclkout = MT41K256M16HA125E_INVERT_CLKOUT,
};

static struct emif_regs ddr3_catchwire_emif_reg_data = {
        .sdram_config = MT41K256M16HA125E_EMIF_SDCFG,
        .ref_ctrl = MT41K256M16HA125E_EMIF_SDREF,
        .sdram_tim1 = MT41K256M16HA125E_EMIF_TIM1,
        .sdram_tim2 = MT41K256M16HA125E_EMIF_TIM2,
        .sdram_tim3 = MT41K256M16HA125E_EMIF_TIM3,
        .zq_config = MT41K256M16HA125E_ZQ_CFG,
        .emif_ddr_phy_ctlr_1 = MT41K256M16HA125E_EMIF_READ_LATENCY,
};

#ifdef CONFIG_SPL_OS_BOOT
int spl_start_uboot(void)
{
	/* break into full u-boot on 'c' */
	return (serial_tstc() && serial_getc() == 'c');
}
#endif

#define OSC	(V_OSCK/1000000)
const struct dpll_params dpll_ddr = {
		266, OSC-1, 1, -1, -1, -1, -1};
const struct dpll_params dpll_ddr_evm_sk = {
		303, OSC-1, 1, -1, -1, -1, -1};
const struct dpll_params dpll_ddr_bone_black = {
		400, OSC-1, 1, -1, -1, -1, -1};
const struct dpll_params dpll_ddr_newt335x = {
                400, OSC-1, 1, -1, -1, -1, -1};
const struct dpll_params dpll_ddr_catchwire = {
                400, OSC-1, 1, -1, -1, -1, -1};

void am33xx_spl_board_init(void)
{
	struct am335x_baseboard_id header;
	int mpu_vdd;

	if (read_eeprom(&header) < 0)
	{
		puts("Wrong data in EEPROM.\n");
		hang();
	}

	if (!board_is_kalitap(&header) && !board_is_catchwire(&header))
	{
		puts("Could not get board ID (CatchWire/KaliTAP).\n");
		hang();
 	}

	/* Get the frequency */
	dpll_mpu_opp100.m = am335x_get_efuse_mpu_max_freq(cdev);

	/* BeagleBone and NEWT PMIC Code */
	int usb_cur_lim;

	if (i2c_probe(TPS65217_CHIP_PM))
		return;

	/*
	 * Override what we have detected since we know we have
	 * a CatchWire/KaliTAP that supports 1GHz.
	 */

         dpll_mpu_opp100.m = MPUPLL_M_1000;

	/*
	 * Increase USB current limit to 1300mA or 1800mA and set
	 * the MPU voltage controller as needed.
	 */
	if (dpll_mpu_opp100.m == MPUPLL_M_1000) {
		usb_cur_lim = TPS65217_USB_INPUT_CUR_LIMIT_1800MA;
		mpu_vdd = TPS65217_DCDC_VOLT_SEL_1325MV;
		puts("tps65217_reg_write USB_INPUT_CUR_LIMIT_1800MA\n");
	} else {
		usb_cur_lim = TPS65217_USB_INPUT_CUR_LIMIT_1300MA;
		mpu_vdd = TPS65217_DCDC_VOLT_SEL_1275MV;
		puts("tps65217_reg_write USB_INPUT_CUR_LIMIT_1300MA\n");
	}

	if (tps65217_reg_write(TPS65217_PROT_LEVEL_NONE,
			       TPS65217_POWER_PATH,
			       usb_cur_lim,
			       TPS65217_USB_INPUT_CUR_LIMIT_MASK))
	{ 
		puts("tps65217_reg_write POWER_PATH failure\n");
	}
	else
	{
		uchar pmic_power_path_reg = 0;
		puts("tps65217_reg_write POWER_PATH success\n");
                       tps65217_reg_read(TPS65217_POWER_PATH,
				  &pmic_power_path_reg);
		printf("POWER_PATH : 0%02X\n", pmic_power_path_reg); 
	}

	/* Set DCDC3 (CORE) voltage to 1.125V */
	if (tps65217_voltage_update(TPS65217_DEFDCDC3,
				    TPS65217_DCDC_VOLT_SEL_1125MV)) {
		puts("tps65217_voltage_update failure\n");
		return;
	}

	/* Set CORE Frequencies to OPP100 */
	do_setup_dpll(&dpll_core_regs, &dpll_core_opp100);

	/* Set DCDC2 (MPU) voltage */
	if (tps65217_voltage_update(TPS65217_DEFDCDC2, mpu_vdd)) {
		puts("tps65217_voltage_update failure\n");
		return;
	}
	else
	{
		puts("tps65217_voltage_update success\n");
	}

	/*
	 * Set LDO3 to 1.8V and LDO4 to 3.3V for CatchWire/KaliTAP.
	 */
	if (tps65217_reg_write(TPS65217_PROT_LEVEL_2,
			       TPS65217_DEFLS1,
			       TPS65217_LDO_VOLTAGE_OUT_1_8,
			       TPS65217_LDO_MASK))
		puts("tps65217_reg_write failure\n");

	if (tps65217_reg_write(TPS65217_PROT_LEVEL_2,
			       TPS65217_DEFLS2,
			       TPS65217_LDO_VOLTAGE_OUT_3_3,
			       TPS65217_LDO_MASK))
		puts("tps65217_reg_write failure\n");

	/* Set MPU Frequency to what we detected now that voltages are set */
	do_setup_dpll(&dpll_mpu_regs, &dpll_mpu_opp100);
}

const struct dpll_params *get_dpll_ddr_params(void)
{
	struct am335x_baseboard_id header;

	enable_i2c0_pin_mux();
        i2c_init(CONFIG_SYS_OMAP24_I2C_SPEED, CONFIG_SYS_OMAP24_I2C_SLAVE);
	if (read_eeprom(&header) < 0)
	{
		puts("Wrong data in EEPROM.\n");
		hang();
	}

        if (board_is_kalitap(&header))
                return &dpll_ddr_catchwire;
        else if (board_is_catchwire(&header))
                return &dpll_ddr_catchwire;
	else
		return &dpll_ddr;
}

void set_uart_mux_conf(void)
{
#ifdef CONFIG_SERIAL1
	enable_uart0_pin_mux();
#endif /* CONFIG_SERIAL1 */
#ifdef CONFIG_SERIAL2
	enable_uart1_pin_mux();
#endif /* CONFIG_SERIAL2 */
#ifdef CONFIG_SERIAL3
	enable_uart2_pin_mux();
#endif /* CONFIG_SERIAL3 */
#ifdef CONFIG_SERIAL4
	enable_uart3_pin_mux();
#endif /* CONFIG_SERIAL4 */
#ifdef CONFIG_SERIAL5
	enable_uart4_pin_mux();
#endif /* CONFIG_SERIAL5 */
#ifdef CONFIG_SERIAL6
	enable_uart5_pin_mux();
#endif /* CONFIG_SERIAL6 */
}

void set_mux_conf_regs(void)
{
	__maybe_unused struct am335x_baseboard_id header;

	if (read_eeprom(&header) < 0)
	{
		puts("Wrong data in EEPROM.\n");
		hang();
	}

	if (!board_is_kalitap(&header) && !board_is_catchwire(&header))
        {
                puts("Could not get board ID (CatchWire/KaliTAP).\n");
                hang();
        }

	enable_board_pin_mux(&header);
}

const struct ctrl_ioregs ioregs_catchwire = {
        .cm0ioctl               = MT41K256M16HA125E_IOCTRL_VALUE,
        .cm1ioctl               = MT41K256M16HA125E_IOCTRL_VALUE,
        .cm2ioctl               = MT41K256M16HA125E_IOCTRL_VALUE,
        .dt0ioctl               = MT41K256M16HA125E_IOCTRL_VALUE,
        .dt1ioctl               = MT41K256M16HA125E_IOCTRL_VALUE,
};

void sdram_init(void)
{
	__maybe_unused struct am335x_baseboard_id header;

	if (read_eeprom(&header) < 0)
	{
		puts("Wrong data in EEEPROM.\n");
		hang();
	}

	if (!board_is_kalitap(&header) && !board_is_catchwire(&header))
        {
                puts("Could not get board ID (CatchWire/KaliTAP).\n");
                hang();
        }

        config_ddr(400, &ioregs_catchwire,
                   &ddr3_catchwire_data,
                   &ddr3_catchwire_cmd_ctrl_data,
                   &ddr3_catchwire_emif_reg_data, 0);
        puts("Set DDR3 to 800MHz.\n");
}
#endif

/*
 * Basic board specific setup.  Pinmux has been handled already.
 */
int board_init(void)
{
#if defined(CONFIG_HW_WATCHDOG)
        hw_watchdog_init();
#endif

        gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;
#if defined(CONFIG_NOR) || defined(CONFIG_NAND)
        gpmc_init();
#endif
        return 0;
}

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	char safe_string[HDR_NAME_LEN + 1];
	struct am335x_baseboard_id header;

	if (read_eeprom(&header) < 0)
		puts("Could not get board ID.\n");

	/* Now set variables based on the header. */
	strncpy(safe_string, (char *)header.name, sizeof(header.name));
	safe_string[sizeof(header.name)] = 0;
	setenv("board_name", safe_string);

	strncpy(safe_string, (char *)header.version, sizeof(header.version));
	safe_string[sizeof(header.version)] = 0;
	setenv("board_rev", safe_string);
#endif

	return 0;
}
#endif

#if (defined(CONFIG_DRIVER_TI_CPSW) && !defined(CONFIG_SPL_BUILD)) || \
	(defined(CONFIG_SPL_ETH_SUPPORT) && defined(CONFIG_SPL_BUILD))
static void cpsw_control(int enabled)
{
	/* VTP can be added here */

	return;
}

static struct cpsw_slave_data cpsw_slaves[] = {
	{
		.slave_reg_ofs	= 0x208,
		.sliver_reg_ofs	= 0xd80,
		.phy_addr	= 3,
	},
	{
		.slave_reg_ofs	= 0x308,
		.sliver_reg_ofs	= 0xdc0,
		.phy_addr	= 1,
	},
};

static struct cpsw_platform_data cpsw_data = {
	.mdio_base		= CPSW_MDIO_BASE,
	.cpsw_base		= CPSW_BASE,
	.mdio_div		= 0xff,
	.channels		= 8,
	.cpdma_reg_ofs		= 0x800,
	.slaves			= 1,
	.slave_data		= cpsw_slaves,
	.ale_reg_ofs		= 0xd00,
	.ale_entries		= 1024,
	.host_port_reg_ofs	= 0x108,
	.hw_stats_reg_ofs	= 0x900,
	.bd_ram_ofs		= 0x2000,
	.mac_control		= (1 << 5),
	.control		= cpsw_control,
	.host_port_num		= 0,
	.version		= CPSW_CTRL_VERSION_2,
};
#endif

/*
 * This function will:
 * Read the eFuse for MAC addresses, and set ethaddr/eth1addr/usbnet_devaddr
 * in the environment
 * Perform fixups to the PHY present on certain boards.  We only need this
 * function in:
 * - SPL with either CPSW or USB ethernet support
 * - Full U-Boot, with either CPSW or USB ethernet
 * Build in only these cases to avoid warnings about unused variables
 * when we build an SPL that has neither option but full U-Boot will.
 */
#if ((defined(CONFIG_SPL_ETH_SUPPORT) || defined(CONFIG_SPL_USBETH_SUPPORT)) \
                && defined(CONFIG_SPL_BUILD)) || \
        ((defined(CONFIG_DRIVER_TI_CPSW) || \
          defined(CONFIG_USB_ETHER) && defined(CONFIG_MUSB_GADGET)) && \
         !defined(CONFIG_SPL_BUILD))
int board_eth_init(bd_t *bis)
{
        int rv, n = 0;
        uint8_t mac_addr[6];
        uint32_t mac_hi, mac_lo;
        __maybe_unused struct am335x_baseboard_id header;

        /* try reading mac address from efuse */
        mac_lo = readl(&cdev->macid0l);
        mac_hi = readl(&cdev->macid0h);
        mac_addr[0] = mac_hi & 0xFF;
        mac_addr[1] = (mac_hi & 0xFF00) >> 8;
        mac_addr[2] = (mac_hi & 0xFF0000) >> 16;
        mac_addr[3] = (mac_hi & 0xFF000000) >> 24;
        mac_addr[4] = mac_lo & 0xFF;
        mac_addr[5] = (mac_lo & 0xFF00) >> 8;

#if (defined(CONFIG_DRIVER_TI_CPSW) && !defined(CONFIG_SPL_BUILD)) || \
        (defined(CONFIG_SPL_ETH_SUPPORT) && defined(CONFIG_SPL_BUILD))
        if (!getenv("ethaddr")) {
                printf("<ethaddr> not set. Validating first E-fuse MAC\n");

                if (is_valid_ether_addr(mac_addr))
                        eth_setenv_enetaddr("ethaddr", mac_addr);
        }

#ifdef CONFIG_DRIVER_TI_CPSW

        mac_lo = readl(&cdev->macid1l);
        mac_hi = readl(&cdev->macid1h);
        mac_addr[0] = mac_hi & 0xFF;
        mac_addr[1] = (mac_hi & 0xFF00) >> 8;
        mac_addr[2] = (mac_hi & 0xFF0000) >> 16;
        mac_addr[3] = (mac_hi & 0xFF000000) >> 24;
        mac_addr[4] = mac_lo & 0xFF;
        mac_addr[5] = (mac_lo & 0xFF00) >> 8;

        if (!getenv("eth1addr")) {
                if (is_valid_ether_addr(mac_addr))
                        eth_setenv_enetaddr("eth1addr", mac_addr);
        }

        if (read_eeprom(&header) < 0)
                puts("Could not get board ID.\n");

	if (board_is_catchwire(&header) || board_is_kalitap(&header)) {
               writel((RGMII_MODE_ENABLE | RGMII_INT_DELAY), &cdev->miisel);
               cpsw_slaves[0].phy_if = cpsw_slaves[1].phy_if =
                               PHY_INTERFACE_MODE_RGMII; 
        } else {
                writel((RGMII_MODE_ENABLE | RGMII_INT_DELAY), &cdev->miisel);
                cpsw_slaves[0].phy_if = cpsw_slaves[1].phy_if =
                                PHY_INTERFACE_MODE_RGMII;
        }

        rv = cpsw_register(&cpsw_data);
        if (rv < 0)
                printf("Error %d registering CPSW switch\n", rv);
        else
                n += rv;
#endif

#endif
#if defined(CONFIG_USB_ETHER) && \
        (!defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_USBETH_SUPPORT))
        if (is_valid_ether_addr(mac_addr))
                eth_setenv_enetaddr("usbnet_devaddr", mac_addr);

        rv = usb_eth_initialize(bis);
        if (rv < 0)
                printf("Error %d registering USB_ETHER\n", rv);
        else
                n += rv;
#endif
        return n;
}
#endif
