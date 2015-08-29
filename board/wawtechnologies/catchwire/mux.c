/*
 * mux.c
 *
 * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/hardware.h>
#include <asm/arch/mux.h>
#include <asm/io.h>
#include <i2c.h>
#include "board.h"

static struct module_pin_mux uart0_pin_mux[] = {
	{OFFSET(uart0_rxd), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* UART0_RXD */
	{OFFSET(uart0_txd), (MODE(0) | PULLUDEN)},		/* UART0_TXD */
	{-1},
};

static struct module_pin_mux uart1_pin_mux[] = {
	{OFFSET(uart1_rxd), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* UART1_RXD */
	{OFFSET(uart1_txd), (MODE(0) | PULLUDEN)},		/* UART1_TXD */
	{-1},
};

static struct module_pin_mux uart2_pin_mux[] = {
        {OFFSET(mii1_txclk), (MODE(1) | PULLUP_EN | RXACTIVE)}, /* UART2_RXD */
        {OFFSET(mii1_rxclk), (MODE(1) | PULLUDEN)},             /* UART2_TXD */
	{-1},
};

static struct module_pin_mux uart3_pin_mux[] = {
        {OFFSET(mii1_rxd3), (MODE(1) | PULLUP_EN | RXACTIVE)},  /* UART3_RXD */
        {OFFSET(mii1_rxd2), (MODE(1) | PULLUDEN)},      /* UART3_TXD */
	{-1},
};

static struct module_pin_mux uart4_pin_mux[] = {
	{OFFSET(gpmc_wait0), (MODE(6) | PULLUP_EN | RXACTIVE)},	/* UART4_RXD */
	{OFFSET(gpmc_wpn), (MODE(6) | PULLUDEN)},		/* UART4_TXD */
	{-1},
};

static struct module_pin_mux uart5_pin_mux[] = {
	{OFFSET(lcd_data9), (MODE(4) | PULLUP_EN | RXACTIVE)},	/* UART5_RXD */
	{OFFSET(lcd_data8), (MODE(4) | PULLUDEN)},		/* UART5_TXD */
	{-1},
};

static struct module_pin_mux mmc0_pin_mux[] = {
	{OFFSET(mmc0_dat3), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* MMC0_DAT3 */
	{OFFSET(mmc0_dat2), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* MMC0_DAT2 */
	{OFFSET(mmc0_dat1), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* MMC0_DAT1 */
	{OFFSET(mmc0_dat0), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* MMC0_DAT0 */
	{OFFSET(mmc0_clk), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* MMC0_CLK */
	{OFFSET(mmc0_cmd), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* MMC0_CMD */
        /*{OFFSET(gpmc_a2), (MODE(7) | RXACTIVE)},*/                /* MMC0_WP */
        {OFFSET(gpmc_csn3), (MODE(7) | RXACTIVE | PULLUP_EN)},    /* MMC0_CD */
	{-1},
};

static struct module_pin_mux i2c0_pin_mux[] = {
	{OFFSET(i2c0_sda), (MODE(0) | RXACTIVE |
			PULLUDEN | SLEWCTRL)}, /* I2C_DATA */
	{OFFSET(i2c0_scl), (MODE(0) | RXACTIVE |
			PULLUDEN | SLEWCTRL)}, /* I2C_SCLK */
	{-1},
};

static struct module_pin_mux gpio0_7_pin_mux[] = {
        {OFFSET(ecap0_in_pwm0_out), (MODE(7) | PULLUDEN)},      /* GPIO0_7 */
        {OFFSET(gpmc_ad1), (MODE(7) | RXACTIVE | PULLUP_EN)},   /* USB1_OC# */
        {OFFSET(gpmc_ad0), (MODE(7) | RXACTIVE | PULLUP_EN)},   /* PLOSS# */
        {-1},
};

static struct module_pin_mux rgmii1_pin_mux[] = {
	{OFFSET(mii1_txen), MODE(2)},			/* RGMII1_TCTL */
	{OFFSET(mii1_rxdv), MODE(2) | RXACTIVE},	/* RGMII1_RCTL */
	{OFFSET(mii1_txd3), MODE(2)},			/* RGMII1_TD3 */
	{OFFSET(mii1_txd2), MODE(2)},			/* RGMII1_TD2 */
	{OFFSET(mii1_txd1), MODE(2)},			/* RGMII1_TD1 */
	{OFFSET(mii1_txd0), MODE(2)},			/* RGMII1_TD0 */
	{OFFSET(mii1_txclk), MODE(2)},			/* RGMII1_TCLK */
	{OFFSET(mii1_rxclk), MODE(2) | RXACTIVE},	/* RGMII1_RCLK */
	{OFFSET(mii1_rxd3), MODE(2) | RXACTIVE},	/* RGMII1_RD3 */
	{OFFSET(mii1_rxd2), MODE(2) | RXACTIVE},	/* RGMII1_RD2 */
	{OFFSET(mii1_rxd1), MODE(2) | RXACTIVE},	/* RGMII1_RD1 */
	{OFFSET(mii1_rxd0), MODE(2) | RXACTIVE},	/* RGMII1_RD0 */
	{OFFSET(mdio_data), MODE(0) | RXACTIVE | PULLUP_EN},/* MDIO_DATA */
	{OFFSET(mdio_clk), MODE(0) | PULLUP_EN},	/* MDIO_CLK */
	{-1},
};

static struct module_pin_mux rgmii2_pin_mux[] = {
        {OFFSET(gpmc_a0), MODE(2)},                   /* RGMII2_TCTL */
        {OFFSET(gpmc_a1), MODE(2) | RXACTIVE},        /* RGMII2_RCTL */
        {OFFSET(gpmc_a2), MODE(2)},                   /* RGMII2_TD3 */
        {OFFSET(gpmc_a3), MODE(2)},                   /* RGMII2_TD2 */
        {OFFSET(gpmc_a4), MODE(2)},                   /* RGMII2_TD1 */
        {OFFSET(gpmc_a5), MODE(2)},                   /* RGMII2_TD0 */
        {OFFSET(gpmc_a6), MODE(2)},                  /* RGMII2_TCLK */
        {OFFSET(gpmc_a7), MODE(2) | RXACTIVE},       /* RGMII2_RCLK */
        {OFFSET(gpmc_a8), MODE(2) | RXACTIVE},        /* RGMII2_RD3 */
        {OFFSET(gpmc_a9), MODE(2) | RXACTIVE},        /* RGMII2_RD2 */
        {OFFSET(gpmc_a10), MODE(2) | RXACTIVE},        /* RGMII2_RD1 */
        {OFFSET(gpmc_a11), MODE(2) | RXACTIVE},        /* RGMII2_RD0 */
        {OFFSET(mdio_data), MODE(0) | RXACTIVE | PULLUP_EN},/* MDIO_DATA */
        {OFFSET(mdio_clk), MODE(0) | PULLUP_EN},        /* MDIO_CLK */
        {-1},
};

#if defined(CONFIG_NOR) && !defined(CONFIG_NOR_BOOT)
static struct module_pin_mux bone_norcape_pin_mux[] = {
	{OFFSET(lcd_data0), MODE(1) | PULLUDEN | RXACTIVE},     /* NOR_A0 */
	{OFFSET(lcd_data1), MODE(1) | PULLUDEN | RXACTIVE},     /* NOR_A1 */
	{OFFSET(lcd_data2), MODE(1) | PULLUDEN | RXACTIVE},     /* NOR_A2 */
	{OFFSET(lcd_data3), MODE(1) | PULLUDEN | RXACTIVE},     /* NOR_A3 */
	{OFFSET(lcd_data4), MODE(1) | PULLUDEN | RXACTIVE},     /* NOR_A4 */
	{OFFSET(lcd_data5), MODE(1) | PULLUDEN | RXACTIVE},     /* NOR_A5 */
	{OFFSET(lcd_data6), MODE(1) | PULLUDEN | RXACTIVE},     /* NOR_A6 */
	{OFFSET(lcd_data7), MODE(1) | PULLUDEN | RXACTIVE},     /* NOR_A7 */
	{OFFSET(lcd_vsync), MODE(1) | PULLUDEN | RXACTIVE},     /* NOR_A8 */
	{OFFSET(lcd_hsync), MODE(1) | PULLUDEN | RXACTIVE},     /* NOR_A9 */
	{OFFSET(lcd_pclk), MODE(1)| PULLUDEN | RXACTIVE},       /* NOR_A10 */
	{OFFSET(lcd_ac_bias_en), MODE(1)| PULLUDEN | RXACTIVE}, /* NOR_A11 */
	{OFFSET(lcd_data8), MODE(1) | PULLUDEN | RXACTIVE},     /* NOR_A12 */
	{OFFSET(lcd_data9), MODE(1) | PULLUDEN | RXACTIVE},     /* NOR_A13 */
	{OFFSET(lcd_data10), MODE(1) | PULLUDEN | RXACTIVE},    /* NOR_A14 */
	{OFFSET(lcd_data11), MODE(1) | PULLUDEN | RXACTIVE},    /* NOR_A15 */
	{OFFSET(lcd_data12), MODE(1) | PULLUDEN | RXACTIVE},    /* NOR_A16 */
	{OFFSET(lcd_data13), MODE(1) | PULLUDEN | RXACTIVE},    /* NOR_A17 */
	{OFFSET(lcd_data14), MODE(1) | PULLUDEN | RXACTIVE},    /* NOR_A18 */
	{OFFSET(lcd_data15), MODE(1) | PULLUDEN | RXACTIVE},    /* NOR_A19 */
	{OFFSET(gpmc_ad0), MODE(0) | PULLUDEN | RXACTIVE},      /* NOR_AD0 */
	{OFFSET(gpmc_ad1), MODE(0) | PULLUDEN | RXACTIVE},      /* NOR_AD1 */
	{OFFSET(gpmc_ad2), MODE(0) | PULLUDEN | RXACTIVE},      /* NOR_AD2 */
	{OFFSET(gpmc_ad3), MODE(0) | PULLUDEN | RXACTIVE},      /* NOR_AD3 */
	{OFFSET(gpmc_ad4), MODE(0) | PULLUDEN | RXACTIVE},      /* NOR_AD4 */
	{OFFSET(gpmc_ad5), MODE(0) | PULLUDEN | RXACTIVE},      /* NOR_AD5 */
	{OFFSET(gpmc_ad6), MODE(0) | PULLUDEN | RXACTIVE},      /* NOR_AD6 */
	{OFFSET(gpmc_ad7), MODE(0) | PULLUDEN | RXACTIVE},      /* NOR_AD7 */
	{OFFSET(gpmc_ad8), MODE(0) | PULLUDEN | RXACTIVE},      /* NOR_AD8 */
	{OFFSET(gpmc_ad9), MODE(0) | PULLUDEN | RXACTIVE},      /* NOR_AD9 */
	{OFFSET(gpmc_ad10), MODE(0) | PULLUDEN | RXACTIVE},     /* NOR_AD10 */
	{OFFSET(gpmc_ad11), MODE(0) | PULLUDEN | RXACTIVE},     /* NOR_AD11 */
	{OFFSET(gpmc_ad12), MODE(0) | PULLUDEN | RXACTIVE},     /* NOR_AD12 */
	{OFFSET(gpmc_ad13), MODE(0) | PULLUDEN | RXACTIVE},     /* NOR_AD13 */
	{OFFSET(gpmc_ad14), MODE(0) | PULLUDEN | RXACTIVE},     /* NOR_AD14 */
	{OFFSET(gpmc_ad15), MODE(0) | PULLUDEN | RXACTIVE},     /* NOR_AD15 */

	{OFFSET(gpmc_csn0), (MODE(0) | PULLUDEN) | RXACTIVE},   /* NOR_CE */
	{OFFSET(gpmc_advn_ale), (MODE(0) | PULLUDEN) | RXACTIVE}, /* NOR_ADVN_ALE */
	{OFFSET(gpmc_oen_ren), (MODE(0) | PULLUDEN | RXACTIVE)},/* NOR_OE */
	{OFFSET(gpmc_be0n_cle), (MODE(0) | PULLUDEN | RXACTIVE)},/* NOR_BE0N_CLE */
	{OFFSET(gpmc_wen), (MODE(0) | PULLUDEN | RXACTIVE)},    /* NOR_WEN */
	{OFFSET(gpmc_wait0), (MODE(0) | RXACTIVE | PULLUDEN)}, /* NOR WAIT */
	{-1},
};
#endif

#if defined(CONFIG_NOR_BOOT)
static struct module_pin_mux norboot_pin_mux[] = {
	{OFFSET(lcd_data1), MODE(1) | PULLUDDIS},
	{OFFSET(lcd_data2), MODE(1) | PULLUDDIS},
	{OFFSET(lcd_data3), MODE(1) | PULLUDDIS},
	{OFFSET(lcd_data4), MODE(1) | PULLUDDIS},
	{OFFSET(lcd_data5), MODE(1) | PULLUDDIS},
	{OFFSET(lcd_data6), MODE(1) | PULLUDDIS},
	{OFFSET(lcd_data7), MODE(1) | PULLUDDIS},
	{OFFSET(lcd_data8), MODE(1) | PULLUDDIS},
	{OFFSET(lcd_data9), MODE(1) | PULLUDDIS},
	{-1},
};

void enable_norboot_pin_mux(void)
{
	configure_module_pin_mux(norboot_pin_mux);
}
#endif

void enable_uart0_pin_mux(void)
{
	configure_module_pin_mux(uart0_pin_mux);
}

void enable_uart1_pin_mux(void)
{
	configure_module_pin_mux(uart1_pin_mux);
}

void enable_uart2_pin_mux(void)
{
	configure_module_pin_mux(uart2_pin_mux);
}

void enable_uart3_pin_mux(void)
{
	configure_module_pin_mux(uart3_pin_mux);
}

void enable_uart4_pin_mux(void)
{
	configure_module_pin_mux(uart4_pin_mux);
}

void enable_uart5_pin_mux(void)
{
	configure_module_pin_mux(uart5_pin_mux);
}

void enable_i2c0_pin_mux(void)
{
	configure_module_pin_mux(i2c0_pin_mux);
}

/*
 * The AM335x GP EVM, if daughter card(s) are connected, can have 8
 * different profiles.  These profiles determine what peripherals are
 * valid and need pinmux to be configured.
 */
#define PROFILE_NONE	0x0
#define PROFILE_0	(1 << 0)
#define PROFILE_1	(1 << 1)
#define PROFILE_2	(1 << 2)
#define PROFILE_3	(1 << 3)
#define PROFILE_4	(1 << 4)
#define PROFILE_5	(1 << 5)
#define PROFILE_6	(1 << 6)
#define PROFILE_7	(1 << 7)
#define PROFILE_MASK	0x7
#define PROFILE_ALL	0xFF

/* CPLD registers */
#define I2C_CPLD_ADDR	0x35
#define CFG_REG		0x10


void enable_board_pin_mux(struct am335x_baseboard_id *header)
{
	/* Do board-specific muxes. */
	if (board_is_catchwire(header) || board_is_kalitap(header)) {
                /* CATCHWIRE Appliance pinmux */
                configure_module_pin_mux(mmc0_pin_mux);
                configure_module_pin_mux(uart0_pin_mux);
                configure_module_pin_mux(rgmii1_pin_mux);
                configure_module_pin_mux(rgmii2_pin_mux);
                configure_module_pin_mux(gpio0_7_pin_mux);
	} else {
		puts("Unknown board, cannot configure pinmux.");
		hang();
	}
}
