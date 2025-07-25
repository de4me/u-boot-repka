// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025 MediaTek Inc.
 *
 * Author: Neal Yen <neal.yen@mediatek.com>
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <phy.h>
#include <miiphy.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/mdio.h>
#include <linux/mii.h>
#include "mtk_eth.h"

/* AN8855 Register Definitions */
#define AN8855_SYS_CTRL_REG			0x100050c0
#define AN8855_SW_SYS_RST			BIT(31)

#define AN8855_PMCR_REG(p)			(0x10210000 + (p) * 0x200)
#define AN8855_FORCE_MODE_LNK			BIT(31)
#define AN8855_FORCE_MODE			0xb31593f0

#define AN8855_PORT_CTRL_BASE			0x10208000
#define AN8855_PORT_CTRL_REG(p, r)		(AN8855_PORT_CTRL_BASE + (p) * 0x200 + (r))

#define AN8855_PORTMATRIX_REG(p)		AN8855_PORT_CTRL_REG(p, 0x44)

#define AN8855_PVC(p)				AN8855_PORT_CTRL_REG(p, 0x10)
#define AN8855_STAG_VPID_S			16
#define AN8855_STAG_VPID_M			0xffff0000
#define AN8855_VLAN_ATTR_S			6
#define AN8855_VLAN_ATTR_M			0xc0

#define VLAN_ATTR_USER				0

#define AN8855_INT_MASK				0x100050F0
#define AN8855_INT_SYS_BIT			BIT(15)

#define AN8855_RG_CLK_CPU_ICG			0x10005034
#define AN8855_MCU_ENABLE			BIT(3)

#define AN8855_RG_TIMER_CTL			0x1000a100
#define AN8855_WDOG_ENABLE			BIT(25)

#define AN8855_CKGCR				0x10213e1c

#define AN8855_SCU_BASE				0x10000000
#define AN8855_RG_RGMII_TXCK_C			(AN8855_SCU_BASE + 0x1d0)
#define AN8855_RG_GPIO_LED_MODE			(AN8855_SCU_BASE + 0x0054)
#define AN8855_RG_GPIO_LED_SEL(i)		(AN8855_SCU_BASE + (0x0058 + ((i) * 4)))
#define AN8855_RG_INTB_MODE			(AN8855_SCU_BASE + 0x0080)
#define AN8855_RG_GDMP_RAM			(AN8855_SCU_BASE + 0x10000)
#define AN8855_RG_GPIO_L_INV			(AN8855_SCU_BASE + 0x0010)
#define AN8855_RG_GPIO_CTRL			(AN8855_SCU_BASE + 0xa300)
#define AN8855_RG_GPIO_DATA			(AN8855_SCU_BASE + 0xa304)
#define AN8855_RG_GPIO_OE			(AN8855_SCU_BASE + 0xa314)

#define AN8855_HSGMII_AN_CSR_BASE		0x10220000
#define AN8855_SGMII_REG_AN0			(AN8855_HSGMII_AN_CSR_BASE + 0x000)
#define AN8855_SGMII_REG_AN_13			(AN8855_HSGMII_AN_CSR_BASE + 0x034)
#define AN8855_SGMII_REG_AN_FORCE_CL37		(AN8855_HSGMII_AN_CSR_BASE + 0x060)

#define AN8855_HSGMII_CSR_PCS_BASE		0x10220000
#define AN8855_RG_HSGMII_PCS_CTROL_1		(AN8855_HSGMII_CSR_PCS_BASE + 0xa00)
#define AN8855_RG_AN_SGMII_MODE_FORCE		(AN8855_HSGMII_CSR_PCS_BASE + 0xa24)

#define AN8855_MULTI_SGMII_CSR_BASE		0x10224000
#define AN8855_SGMII_STS_CTRL_0			(AN8855_MULTI_SGMII_CSR_BASE + 0x018)
#define AN8855_MSG_RX_CTRL_0			(AN8855_MULTI_SGMII_CSR_BASE + 0x100)
#define AN8855_MSG_RX_LIK_STS_0			(AN8855_MULTI_SGMII_CSR_BASE + 0x514)
#define AN8855_MSG_RX_LIK_STS_2			(AN8855_MULTI_SGMII_CSR_BASE + 0x51c)
#define AN8855_PHY_RX_FORCE_CTRL_0		(AN8855_MULTI_SGMII_CSR_BASE + 0x520)

#define AN8855_XFI_CSR_PCS_BASE			0x10225000
#define AN8855_RG_USXGMII_AN_CONTROL_0		(AN8855_XFI_CSR_PCS_BASE + 0xbf8)

#define AN8855_MULTI_PHY_RA_CSR_BASE		0x10226000
#define AN8855_RG_RATE_ADAPT_CTRL_0		(AN8855_MULTI_PHY_RA_CSR_BASE + 0x000)
#define AN8855_RATE_ADP_P0_CTRL_0		(AN8855_MULTI_PHY_RA_CSR_BASE + 0x100)
#define AN8855_MII_RA_AN_ENABLE			(AN8855_MULTI_PHY_RA_CSR_BASE + 0x300)

#define AN8855_QP_DIG_CSR_BASE			0x1022a000
#define AN8855_QP_CK_RST_CTRL_4			(AN8855_QP_DIG_CSR_BASE + 0x310)
#define AN8855_QP_DIG_MODE_CTRL_0		(AN8855_QP_DIG_CSR_BASE + 0x324)
#define AN8855_QP_DIG_MODE_CTRL_1		(AN8855_QP_DIG_CSR_BASE + 0x330)

#define AN8855_QP_PMA_TOP_BASE			0x1022e000
#define AN8855_PON_RXFEDIG_CTRL_0		(AN8855_QP_PMA_TOP_BASE + 0x100)
#define AN8855_PON_RXFEDIG_CTRL_9		(AN8855_QP_PMA_TOP_BASE + 0x124)

#define AN8855_SS_LCPLL_PWCTL_SETTING_2		(AN8855_QP_PMA_TOP_BASE + 0x208)
#define AN8855_SS_LCPLL_TDC_FLT_2		(AN8855_QP_PMA_TOP_BASE + 0x230)
#define AN8855_SS_LCPLL_TDC_FLT_5		(AN8855_QP_PMA_TOP_BASE + 0x23c)
#define AN8855_SS_LCPLL_TDC_PCW_1		(AN8855_QP_PMA_TOP_BASE + 0x248)
#define AN8855_INTF_CTRL_8			(AN8855_QP_PMA_TOP_BASE + 0x320)
#define AN8855_INTF_CTRL_9			(AN8855_QP_PMA_TOP_BASE + 0x324)
#define AN8855_PLL_CTRL_0			(AN8855_QP_PMA_TOP_BASE + 0x400)
#define AN8855_PLL_CTRL_2			(AN8855_QP_PMA_TOP_BASE + 0x408)
#define AN8855_PLL_CTRL_3			(AN8855_QP_PMA_TOP_BASE + 0x40c)
#define AN8855_PLL_CTRL_4			(AN8855_QP_PMA_TOP_BASE + 0x410)
#define AN8855_PLL_CK_CTRL_0			(AN8855_QP_PMA_TOP_BASE + 0x414)
#define AN8855_RX_DLY_0				(AN8855_QP_PMA_TOP_BASE + 0x614)
#define AN8855_RX_CTRL_2			(AN8855_QP_PMA_TOP_BASE + 0x630)
#define AN8855_RX_CTRL_5			(AN8855_QP_PMA_TOP_BASE + 0x63c)
#define AN8855_RX_CTRL_6			(AN8855_QP_PMA_TOP_BASE + 0x640)
#define AN8855_RX_CTRL_7			(AN8855_QP_PMA_TOP_BASE + 0x644)
#define AN8855_RX_CTRL_8			(AN8855_QP_PMA_TOP_BASE + 0x648)
#define AN8855_RX_CTRL_26			(AN8855_QP_PMA_TOP_BASE + 0x690)
#define AN8855_RX_CTRL_42			(AN8855_QP_PMA_TOP_BASE + 0x6d0)

#define AN8855_QP_ANA_CSR_BASE			0x1022f000
#define AN8855_RG_QP_RX_DAC_EN			(AN8855_QP_ANA_CSR_BASE + 0x00)
#define AN8855_RG_QP_RXAFE_RESERVE		(AN8855_QP_ANA_CSR_BASE + 0x04)
#define AN8855_RG_QP_CDR_LPF_MJV_LIM		(AN8855_QP_ANA_CSR_BASE + 0x0c)
#define AN8855_RG_QP_CDR_LPF_SETVALUE		(AN8855_QP_ANA_CSR_BASE + 0x14)
#define AN8855_RG_QP_CDR_PR_CKREF_DIV1		(AN8855_QP_ANA_CSR_BASE + 0x18)
#define AN8855_RG_QP_CDR_PR_KBAND_DIV_PCIE	(AN8855_QP_ANA_CSR_BASE + 0x1c)
#define AN8855_RG_QP_CDR_FORCE_IBANDLPF_R_OFF	(AN8855_QP_ANA_CSR_BASE + 0x20)
#define AN8855_RG_QP_TX_MODE_16B_EN		(AN8855_QP_ANA_CSR_BASE + 0x28)
#define AN8855_RG_QP_PLL_IPLL_DIG_PWR_SEL	(AN8855_QP_ANA_CSR_BASE + 0x3c)
#define AN8855_RG_QP_PLL_SDM_ORD		(AN8855_QP_ANA_CSR_BASE + 0x40)

#define AN8855_ETHER_SYS_BASE			0x1028c800
#define RG_GPHY_AFE_PWD				(AN8855_ETHER_SYS_BASE + 0x40)

#define AN8855_PKG_SEL				0x10000094
#define PAG_SEL_AN8855H				0x2

/* PHY LED Register bitmap of define */
#define PHY_LED_CTRL_SELECT			0x3e8
#define PHY_SINGLE_LED_ON_CTRL(i)		(0x3e0 + ((i) * 2))
#define PHY_SINGLE_LED_BLK_CTRL(i)		(0x3e1 + ((i) * 2))
#define PHY_SINGLE_LED_ON_DUR(i)		(0x3e9 + ((i) * 2))
#define PHY_SINGLE_LED_BLK_DUR(i)		(0x3ea + ((i) * 2))

#define PHY_PMA_CTRL				0x340

#define PHY_DEV1F				0x1f

#define PHY_LED_ON_CTRL(i)			(0x24 + ((i) * 2))
#define LED_ON_EN				BIT(15)
#define LED_ON_POL				BIT(14)
#define LED_ON_EVT_MASK				0x7f

/* LED ON Event */
#define LED_ON_EVT_FORCE			BIT(6)
#define LED_ON_EVT_LINK_HD			BIT(5)
#define LED_ON_EVT_LINK_FD			BIT(4)
#define LED_ON_EVT_LINK_DOWN			BIT(3)
#define LED_ON_EVT_LINK_10M			BIT(2)
#define LED_ON_EVT_LINK_100M			BIT(1)
#define LED_ON_EVT_LINK_1000M			BIT(0)

#define PHY_LED_BLK_CTRL(i)			(0x25 + ((i) * 2))
#define LED_BLK_EVT_MASK			0x3ff
/* LED Blinking Event */
#define LED_BLK_EVT_FORCE			BIT(9)
#define LED_BLK_EVT_10M_RX_ACT			BIT(5)
#define LED_BLK_EVT_10M_TX_ACT			BIT(4)
#define LED_BLK_EVT_100M_RX_ACT			BIT(3)
#define LED_BLK_EVT_100M_TX_ACT			BIT(2)
#define LED_BLK_EVT_1000M_RX_ACT		BIT(1)
#define LED_BLK_EVT_1000M_TX_ACT		BIT(0)

#define PHY_LED_BCR				(0x21)
#define LED_BCR_EXT_CTRL			BIT(15)
#define LED_BCR_CLK_EN				BIT(3)
#define LED_BCR_TIME_TEST			BIT(2)
#define LED_BCR_MODE_MASK			3
#define LED_BCR_MODE_DISABLE			0

#define PHY_LED_ON_DUR				0x22
#define LED_ON_DUR_MASK				0xffff

#define PHY_LED_BLK_DUR				0x23
#define LED_BLK_DUR_MASK			0xffff

#define PHY_LED_BLINK_DUR_CTRL			0x720

/* Definition of LED */
#define LED_ON_EVENT	(LED_ON_EVT_LINK_1000M | \
			LED_ON_EVT_LINK_100M | LED_ON_EVT_LINK_10M |\
			LED_ON_EVT_LINK_HD | LED_ON_EVT_LINK_FD)

#define LED_BLK_EVENT	(LED_BLK_EVT_1000M_TX_ACT | \
			LED_BLK_EVT_1000M_RX_ACT | \
			LED_BLK_EVT_100M_TX_ACT | \
			LED_BLK_EVT_100M_RX_ACT | \
			LED_BLK_EVT_10M_TX_ACT | \
			LED_BLK_EVT_10M_RX_ACT)

#define LED_FREQ				AIR_LED_BLK_DUR_64M

#define AN8855_NUM_PHYS				5
#define AN8855_NUM_PORTS			6
#define AN8855_PHY_ADDR(base, addr)		(((base) + (addr)) & 0x1f)

/* PHY LED Register bitmap of define */
#define PHY_LED_CTRL_SELECT			0x3e8
#define PHY_SINGLE_LED_ON_CTRL(i)		(0x3e0 + ((i) * 2))
#define PHY_SINGLE_LED_BLK_CTRL(i)		(0x3e1 + ((i) * 2))
#define PHY_SINGLE_LED_ON_DUR(i)		(0x3e9 + ((i) * 2))
#define PHY_SINGLE_LED_BLK_DUR(i)		(0x3ea + ((i) * 2))

/* AN8855 LED */
enum an8855_led_blk_dur {
	AIR_LED_BLK_DUR_32M,
	AIR_LED_BLK_DUR_64M,
	AIR_LED_BLK_DUR_128M,
	AIR_LED_BLK_DUR_256M,
	AIR_LED_BLK_DUR_512M,
	AIR_LED_BLK_DUR_1024M,
	AIR_LED_BLK_DUR_LAST
};

enum an8855_led_polarity {
	LED_LOW,
	LED_HIGH,
};

enum an8855_led_mode {
	AN8855_LED_MODE_DISABLE,
	AN8855_LED_MODE_USER_DEFINE,
	AN8855_LED_MODE_LAST
};

enum phy_led_idx {
	P0_LED0,
	P0_LED1,
	P0_LED2,
	P0_LED3,
	P1_LED0,
	P1_LED1,
	P1_LED2,
	P1_LED3,
	P2_LED0,
	P2_LED1,
	P2_LED2,
	P2_LED3,
	P3_LED0,
	P3_LED1,
	P3_LED2,
	P3_LED3,
	P4_LED0,
	P4_LED1,
	P4_LED2,
	P4_LED3,
	PHY_LED_MAX
};

struct an8855_led_cfg {
	u16 en;
	u8  phy_led_idx;
	u16 pol;
	u16 on_cfg;
	u16 blk_cfg;
	u8 led_freq;
};

struct an8855_switch_priv {
	struct mtk_eth_switch_priv epriv;
	struct mii_dev *mdio_bus;
	u32 phy_base;
};

/* AN8855 Reference Board */
static const struct an8855_led_cfg led_cfg[] = {
/*************************************************************************
 * Enable, LED idx, LED Polarity, LED ON event,  LED Blink event  LED Freq
 *************************************************************************
 */
	/* GPIO0 */
	{1, P4_LED0, LED_HIGH, LED_ON_EVENT, LED_BLK_EVENT, LED_FREQ},
	/* GPIO1 */
	{1, P4_LED1, LED_HIGH, LED_ON_EVENT, LED_BLK_EVENT, LED_FREQ},
	/* GPIO2 */
	{1, P0_LED0, LED_HIGH, LED_ON_EVENT, LED_BLK_EVENT, LED_FREQ},
	/* GPIO3 */
	{1, P0_LED1, LED_HIGH, LED_ON_EVENT, LED_BLK_EVENT, LED_FREQ},
	/* GPIO4 */
	{1, P1_LED0, LED_LOW,  LED_ON_EVENT, LED_BLK_EVENT, LED_FREQ},
	/* GPIO5 */
	{1, P1_LED1, LED_LOW,  LED_ON_EVENT, LED_BLK_EVENT, LED_FREQ},
	/* GPIO6 */
	{0, PHY_LED_MAX, LED_LOW,  LED_ON_EVENT, LED_BLK_EVENT, LED_FREQ},
	/* GPIO7 */
	{0, PHY_LED_MAX, LED_HIGH, LED_ON_EVENT, LED_BLK_EVENT, LED_FREQ},
	/* GPIO8 */
	{0, PHY_LED_MAX, LED_HIGH, LED_ON_EVENT, LED_BLK_EVENT, LED_FREQ},
	/* GPIO9 */
	{1, P2_LED0, LED_HIGH, LED_ON_EVENT, LED_BLK_EVENT, LED_FREQ},
	/* GPIO10 */
	{1, P2_LED1, LED_HIGH, LED_ON_EVENT, LED_BLK_EVENT, LED_FREQ},
	/* GPIO11 */
	{1, P3_LED0, LED_HIGH, LED_ON_EVENT, LED_BLK_EVENT, LED_FREQ},
	/* GPIO12 */
	{1, P3_LED1, LED_HIGH,  LED_ON_EVENT, LED_BLK_EVENT, LED_FREQ},
	/* GPIO13 */
	{0, PHY_LED_MAX, LED_HIGH, LED_ON_EVENT, LED_BLK_EVENT, LED_FREQ},
	/* GPIO14 */
	{0, PHY_LED_MAX, LED_HIGH, LED_ON_EVENT, LED_BLK_EVENT, LED_FREQ},
	/* GPIO15 */
	{0, PHY_LED_MAX, LED_HIGH, LED_ON_EVENT, LED_BLK_EVENT, LED_FREQ},
	/* GPIO16 */
	{0, PHY_LED_MAX, LED_HIGH, LED_ON_EVENT, LED_BLK_EVENT, LED_FREQ},
	/* GPIO17 */
	{0, PHY_LED_MAX, LED_HIGH, LED_ON_EVENT, LED_BLK_EVENT, LED_FREQ},
	/* GPIO18 */
	{0, PHY_LED_MAX, LED_HIGH, LED_ON_EVENT, LED_BLK_EVENT, LED_FREQ},
	/* GPIO19 */
	{0, PHY_LED_MAX, LED_LOW,  LED_ON_EVENT, LED_BLK_EVENT, LED_FREQ},
	/* GPIO20 */
	{0, PHY_LED_MAX, LED_LOW,  LED_ON_EVENT, LED_BLK_EVENT, LED_FREQ},
};

static int __an8855_reg_read(struct mtk_eth_priv *priv, u8 phy_base, u32 reg, u32 *data)
{
	int ret, low_word, high_word;

	ret = mtk_mii_write(priv, phy_base, 0x1f, 0x4);
	if (ret)
		return ret;

	ret = mtk_mii_write(priv, phy_base, 0x10, 0);
	if (ret)
		return ret;

	ret = mtk_mii_write(priv, phy_base, 0x15, ((reg >> 16) & 0xFFFF));
	if (ret)
		return ret;

	ret = mtk_mii_write(priv, phy_base, 0x16, (reg & 0xFFFF));
	if (ret)
		return ret;

	low_word = mtk_mii_read(priv, phy_base, 0x18);
	if (low_word < 0)
		return low_word;

	high_word = mtk_mii_read(priv, phy_base, 0x17);
	if (high_word < 0)
		return high_word;

	ret = mtk_mii_write(priv, phy_base, 0x1f, 0);
	if (ret)
		return ret;

	ret = mtk_mii_write(priv, phy_base, 0x10, 0);
	if (ret)
		return ret;

	if (data)
		*data = ((u32)high_word << 16) | (low_word & 0xffff);

	return 0;
}

static int an8855_reg_read(struct an8855_switch_priv *priv, u32 reg, u32 *data)
{
	return __an8855_reg_read(priv->epriv.eth, priv->phy_base, reg, data);
}

static int an8855_reg_write(struct an8855_switch_priv *priv, u32 reg, u32 data)
{
	int ret;

	ret = mtk_mii_write(priv->epriv.eth, priv->phy_base, 0x1f, 0x4);
	if (ret)
		return ret;

	ret = mtk_mii_write(priv->epriv.eth, priv->phy_base, 0x10, 0);
	if (ret)
		return ret;

	ret = mtk_mii_write(priv->epriv.eth, priv->phy_base, 0x11,
			    ((reg >> 16) & 0xFFFF));
	if (ret)
		return ret;

	ret = mtk_mii_write(priv->epriv.eth, priv->phy_base, 0x12,
			    (reg & 0xFFFF));
	if (ret)
		return ret;

	ret = mtk_mii_write(priv->epriv.eth, priv->phy_base, 0x13,
			    ((data >> 16) & 0xFFFF));
	if (ret)
		return ret;

	ret = mtk_mii_write(priv->epriv.eth, priv->phy_base, 0x14,
			    (data & 0xFFFF));
	if (ret)
		return ret;

	ret = mtk_mii_write(priv->epriv.eth, priv->phy_base, 0x1f, 0);
	if (ret)
		return ret;

	ret = mtk_mii_write(priv->epriv.eth, priv->phy_base, 0x10, 0);
	if (ret)
		return ret;

	return 0;
}

static int an8855_phy_cl45_read(struct an8855_switch_priv *priv, int port,
				int devad, int regnum, u16 *data)
{
	u16 phy_addr = AN8855_PHY_ADDR(priv->phy_base, port);

	*data = mtk_mmd_ind_read(priv->epriv.eth, phy_addr, devad, regnum);

	return 0;
}

static int an8855_phy_cl45_write(struct an8855_switch_priv *priv, int port,
				 int devad, int regnum, u16 data)
{
	u16 phy_addr = AN8855_PHY_ADDR(priv->phy_base, port);

	mtk_mmd_ind_write(priv->epriv.eth, phy_addr, devad, regnum, data);

	return 0;
}

static int an8855_port_sgmii_init(struct an8855_switch_priv *priv, u32 port)
{
	u32 val = 0;

	if (port != 5) {
		printf("an8855: port %d is not a SGMII port\n", port);
		return -EINVAL;
	}

	/* PLL */
	an8855_reg_read(priv, AN8855_QP_DIG_MODE_CTRL_1, &val);
	val &= ~(0x3 << 2);
	val |= (0x1 << 2);
	an8855_reg_write(priv, AN8855_QP_DIG_MODE_CTRL_1, val);

	/* PLL - LPF */
	an8855_reg_read(priv, AN8855_PLL_CTRL_2, &val);
	val &= ~(0x3 << 0);
	val |= (0x1 << 0);
	val &= ~(0x7 << 2);
	val |= (0x5 << 2);
	val &= ~GENMASK(7, 6);
	val &= ~(0x7 << 8);
	val |= (0x3 << 8);
	val |= BIT(29);
	val &= ~GENMASK(13, 12);
	an8855_reg_write(priv, AN8855_PLL_CTRL_2, val);

	/* PLL - ICO */
	an8855_reg_read(priv, AN8855_PLL_CTRL_4, &val);
	val |= BIT(2);
	an8855_reg_write(priv, AN8855_PLL_CTRL_4, val);

	an8855_reg_read(priv, AN8855_PLL_CTRL_2, &val);
	val &= ~BIT(14);
	an8855_reg_write(priv, AN8855_PLL_CTRL_2, val);

	/* PLL - CHP */
	an8855_reg_read(priv, AN8855_PLL_CTRL_2, &val);
	val &= ~(0xf << 16);
	val |= (0x6 << 16);
	an8855_reg_write(priv, AN8855_PLL_CTRL_2, val);

	/* PLL - PFD */
	an8855_reg_read(priv, AN8855_PLL_CTRL_2, &val);
	val &= ~(0x3 << 20);
	val |= (0x1 << 20);
	val &= ~(0x3 << 24);
	val |= (0x1 << 24);
	val &= ~BIT(26);
	an8855_reg_write(priv, AN8855_PLL_CTRL_2, val);

	/* PLL - POSTDIV */
	an8855_reg_read(priv, AN8855_PLL_CTRL_2, &val);
	val |= BIT(22);
	val &= ~BIT(27);
	val &= ~BIT(28);
	an8855_reg_write(priv, AN8855_PLL_CTRL_2, val);

	/* PLL - SDM */
	an8855_reg_read(priv, AN8855_PLL_CTRL_4, &val);
	val &= ~GENMASK(4, 3);
	an8855_reg_write(priv, AN8855_PLL_CTRL_4, val);

	an8855_reg_read(priv, AN8855_PLL_CTRL_2, &val);
	val &= ~BIT(30);
	an8855_reg_write(priv, AN8855_PLL_CTRL_2, val);

	an8855_reg_read(priv, AN8855_SS_LCPLL_PWCTL_SETTING_2, &val);
	val &= ~(0x3 << 16);
	val |= (0x1 << 16);
	an8855_reg_write(priv, AN8855_SS_LCPLL_PWCTL_SETTING_2, val);

	an8855_reg_write(priv, AN8855_SS_LCPLL_TDC_FLT_2, 0x7a000000);
	an8855_reg_write(priv, AN8855_SS_LCPLL_TDC_PCW_1, 0x7a000000);

	an8855_reg_read(priv, AN8855_SS_LCPLL_TDC_FLT_5, &val);
	val &= ~BIT(24);
	an8855_reg_write(priv, AN8855_SS_LCPLL_TDC_FLT_5, val);

	an8855_reg_read(priv, AN8855_PLL_CK_CTRL_0, &val);
	val &= ~BIT(8);
	an8855_reg_write(priv, AN8855_PLL_CK_CTRL_0, val);

	/* PLL - SS */
	an8855_reg_read(priv, AN8855_PLL_CTRL_3, &val);
	val &= ~GENMASK(15, 0);
	an8855_reg_write(priv, AN8855_PLL_CTRL_3, val);

	an8855_reg_read(priv, AN8855_PLL_CTRL_4, &val);
	val &= ~GENMASK(1, 0);
	an8855_reg_write(priv, AN8855_PLL_CTRL_4, val);

	an8855_reg_read(priv, AN8855_PLL_CTRL_3, &val);
	val &= ~GENMASK(31, 16);
	an8855_reg_write(priv, AN8855_PLL_CTRL_3, val);

	/* PLL - TDC */
	an8855_reg_read(priv, AN8855_PLL_CK_CTRL_0, &val);
	val &= ~BIT(9);
	an8855_reg_write(priv, AN8855_PLL_CK_CTRL_0, val);

	an8855_reg_read(priv, AN8855_RG_QP_PLL_SDM_ORD, &val);
	val |= BIT(3);
	val |= BIT(4);
	an8855_reg_write(priv, AN8855_RG_QP_PLL_SDM_ORD, val);

	an8855_reg_read(priv, AN8855_RG_QP_RX_DAC_EN, &val);
	val &= ~(0x3 << 16);
	val |= (0x2 << 16);
	an8855_reg_write(priv, AN8855_RG_QP_RX_DAC_EN, val);

	/* TCL Disable (only for Co-SIM) */
	an8855_reg_read(priv, AN8855_PON_RXFEDIG_CTRL_0, &val);
	val &= ~BIT(12);
	an8855_reg_write(priv, AN8855_PON_RXFEDIG_CTRL_0, val);

	/* TX Init */
	an8855_reg_read(priv, AN8855_RG_QP_TX_MODE_16B_EN, &val);
	val &= ~BIT(0);
	val &= ~(0xffff << 16);
	val |= (0x4 << 16);
	an8855_reg_write(priv, AN8855_RG_QP_TX_MODE_16B_EN, val);

	/* RX Control */
	an8855_reg_read(priv, AN8855_RG_QP_RXAFE_RESERVE, &val);
	val |= BIT(11);
	an8855_reg_write(priv, AN8855_RG_QP_RXAFE_RESERVE, val);

	an8855_reg_read(priv, AN8855_RG_QP_CDR_LPF_MJV_LIM, &val);
	val &= ~(0x3 << 4);
	val |= (0x1 << 4);
	an8855_reg_write(priv, AN8855_RG_QP_CDR_LPF_MJV_LIM, val);

	an8855_reg_read(priv, AN8855_RG_QP_CDR_LPF_SETVALUE, &val);
	val &= ~(0xf << 25);
	val |= (0x1 << 25);
	val &= ~(0x7 << 29);
	val |= (0x3 << 29);
	an8855_reg_write(priv, AN8855_RG_QP_CDR_LPF_SETVALUE, val);

	an8855_reg_read(priv, AN8855_RG_QP_CDR_PR_CKREF_DIV1, &val);
	val &= ~(0x1f << 8);
	val |= (0xf << 8);
	an8855_reg_write(priv, AN8855_RG_QP_CDR_PR_CKREF_DIV1, val);

	an8855_reg_read(priv, AN8855_RG_QP_CDR_PR_KBAND_DIV_PCIE, &val);
	val &= ~(0x3f << 0);
	val |= (0x19 << 0);
	val &= ~BIT(6);
	an8855_reg_write(priv, AN8855_RG_QP_CDR_PR_KBAND_DIV_PCIE, val);

	an8855_reg_read(priv, AN8855_RG_QP_CDR_FORCE_IBANDLPF_R_OFF, &val);
	val &= ~(0x7f << 6);
	val |= (0x21 << 6);
	val &= ~(0x3 << 16);
	val |= (0x2 << 16);
	val &= ~BIT(13);
	an8855_reg_write(priv, AN8855_RG_QP_CDR_FORCE_IBANDLPF_R_OFF, val);

	an8855_reg_read(priv, AN8855_RG_QP_CDR_PR_KBAND_DIV_PCIE, &val);
	val &= ~BIT(30);
	an8855_reg_write(priv, AN8855_RG_QP_CDR_PR_KBAND_DIV_PCIE, val);

	an8855_reg_read(priv, AN8855_RG_QP_CDR_PR_CKREF_DIV1, &val);
	val &= ~(0x7 << 24);
	val |= (0x4 << 24);
	an8855_reg_write(priv, AN8855_RG_QP_CDR_PR_CKREF_DIV1, val);

	an8855_reg_read(priv, AN8855_PLL_CTRL_0, &val);
	val |= BIT(0);
	an8855_reg_write(priv, AN8855_PLL_CTRL_0, val);

	an8855_reg_read(priv, AN8855_RX_CTRL_26, &val);
	val &= ~BIT(23);
	val |= BIT(26);
	an8855_reg_write(priv, AN8855_RX_CTRL_26, val);

	an8855_reg_read(priv, AN8855_RX_DLY_0, &val);
	val &= ~(0xff << 0);
	val |= (0x6f << 0);
	val |= GENMASK(13, 8);
	an8855_reg_write(priv, AN8855_RX_DLY_0, val);

	an8855_reg_read(priv, AN8855_RX_CTRL_42, &val);
	val &= ~(0x1fff << 0);
	val |= (0x150 << 0);
	an8855_reg_write(priv, AN8855_RX_CTRL_42, val);

	an8855_reg_read(priv, AN8855_RX_CTRL_2, &val);
	val &= ~(0x1fff << 16);
	val |= (0x150 << 16);
	an8855_reg_write(priv, AN8855_RX_CTRL_2, val);

	an8855_reg_read(priv, AN8855_PON_RXFEDIG_CTRL_9, &val);
	val &= ~(0x7 << 0);
	val |= (0x1 << 0);
	an8855_reg_write(priv, AN8855_PON_RXFEDIG_CTRL_9, val);

	an8855_reg_read(priv, AN8855_RX_CTRL_8, &val);
	val &= ~(0xfff << 16);
	val |= (0x200 << 16);
	val &= ~(0x7fff << 14);
	val |= (0xfff << 14);
	an8855_reg_write(priv, AN8855_RX_CTRL_8, val);

	/* Frequency memter */
	an8855_reg_read(priv, AN8855_RX_CTRL_5, &val);
	val &= ~(0xfffff << 10);
	val |= (0x10 << 10);
	an8855_reg_write(priv, AN8855_RX_CTRL_5, val);

	an8855_reg_read(priv, AN8855_RX_CTRL_6, &val);
	val &= ~(0xfffff << 0);
	val |= (0x64 << 0);
	an8855_reg_write(priv, AN8855_RX_CTRL_6, val);

	an8855_reg_read(priv, AN8855_RX_CTRL_7, &val);
	val &= ~(0xfffff << 0);
	val |= (0x2710 << 0);
	an8855_reg_write(priv, AN8855_RX_CTRL_7, val);

	/* PCS Init */
	an8855_reg_read(priv, AN8855_RG_HSGMII_PCS_CTROL_1, &val);
	val &= ~BIT(30);
	an8855_reg_write(priv, AN8855_RG_HSGMII_PCS_CTROL_1, val);

	/* Rate Adaption */
	an8855_reg_read(priv, AN8855_RATE_ADP_P0_CTRL_0, &val);
	val &= ~BIT(31);
	an8855_reg_write(priv, AN8855_RATE_ADP_P0_CTRL_0, val);

	an8855_reg_read(priv, AN8855_RG_RATE_ADAPT_CTRL_0, &val);
	val |= BIT(0);
	val |= BIT(4);
	val |= GENMASK(27, 26);
	an8855_reg_write(priv, AN8855_RG_RATE_ADAPT_CTRL_0, val);

	/* Disable AN */
	an8855_reg_read(priv, AN8855_SGMII_REG_AN0, &val);
	val &= ~BIT(12);
	an8855_reg_write(priv, AN8855_SGMII_REG_AN0, val);

	/* Force Speed */
	an8855_reg_read(priv, AN8855_SGMII_STS_CTRL_0, &val);
	val |= BIT(2);
	val |= GENMASK(5, 4);
	an8855_reg_write(priv, AN8855_SGMII_STS_CTRL_0, val);

	/* bypass flow control to MAC */
	an8855_reg_write(priv, AN8855_MSG_RX_LIK_STS_0, 0x01010107);
	an8855_reg_write(priv, AN8855_MSG_RX_LIK_STS_2, 0x00000EEF);

	return 0;
}

static void an8855_led_set_usr_def(struct an8855_switch_priv *priv, u8 entity,
				   enum an8855_led_polarity pol, u16 on_evt,
				   u16 blk_evt, u8 led_freq)
{
	u32 cl45_data;

	if (pol == LED_HIGH)
		on_evt |= LED_ON_POL;
	else
		on_evt &= ~LED_ON_POL;

	/* LED on event */
	an8855_phy_cl45_write(priv, (entity / 4), 0x1e,
			      PHY_SINGLE_LED_ON_CTRL(entity % 4),
			      on_evt | LED_ON_EN);

	/* LED blink event */
	an8855_phy_cl45_write(priv, (entity / 4), 0x1e,
			      PHY_SINGLE_LED_BLK_CTRL(entity % 4),
			      blk_evt);

	/* LED freq */
	switch (led_freq) {
	case AIR_LED_BLK_DUR_32M:
		cl45_data = 0x30e;
		break;

	case AIR_LED_BLK_DUR_64M:
		cl45_data = 0x61a;
		break;

	case AIR_LED_BLK_DUR_128M:
		cl45_data = 0xc35;
		break;

	case AIR_LED_BLK_DUR_256M:
		cl45_data = 0x186a;
		break;

	case AIR_LED_BLK_DUR_512M:
		cl45_data = 0x30d4;
		break;

	case AIR_LED_BLK_DUR_1024M:
		cl45_data = 0x61a8;
		break;

	default:
		cl45_data = 0;
		break;
	}

	an8855_phy_cl45_write(priv, (entity / 4), 0x1e,
			      PHY_SINGLE_LED_BLK_DUR(entity % 4),
			      cl45_data);

	an8855_phy_cl45_write(priv, (entity / 4), 0x1e,
			      PHY_SINGLE_LED_ON_DUR(entity % 4),
			      (cl45_data >> 1));

	/* Disable DATA & BAD_SSD for port LED blink behavior */
	cl45_data = mtk_mmd_ind_read(priv->epriv.eth, (entity / 4), 0x1e, PHY_PMA_CTRL);
	cl45_data &= ~BIT(0);
	cl45_data &= ~BIT(15);
	an8855_phy_cl45_write(priv, (entity / 4), 0x1e, PHY_PMA_CTRL, cl45_data);
}

static int an8855_led_set_mode(struct an8855_switch_priv *priv, u8 mode)
{
	u16 cl45_data;

	an8855_phy_cl45_read(priv, 0, 0x1f, PHY_LED_BCR, &cl45_data);

	switch (mode) {
	case AN8855_LED_MODE_DISABLE:
		cl45_data &= ~LED_BCR_EXT_CTRL;
		cl45_data &= ~LED_BCR_MODE_MASK;
		cl45_data |= LED_BCR_MODE_DISABLE;
		break;

	case AN8855_LED_MODE_USER_DEFINE:
		cl45_data |= LED_BCR_EXT_CTRL;
		cl45_data |= LED_BCR_CLK_EN;
		break;

	default:
		printf("an8855: LED mode%d is not supported!\n", mode);
		return -EINVAL;
	}

	an8855_phy_cl45_write(priv, 0, 0x1f, PHY_LED_BCR, cl45_data);

	return 0;
}

static int an8855_led_set_state(struct an8855_switch_priv *priv, u8 entity,
				u8 state)
{
	u16 cl45_data = 0;

	/* Change to per port contorl */
	an8855_phy_cl45_read(priv, (entity / 4), 0x1e, PHY_LED_CTRL_SELECT,
			     &cl45_data);

	if (state == 1)
		cl45_data |= (1 << (entity % 4));
	else
		cl45_data &= ~(1 << (entity % 4));

	an8855_phy_cl45_write(priv, (entity / 4), 0x1e, PHY_LED_CTRL_SELECT,
			      cl45_data);

	/* LED enable setting */
	an8855_phy_cl45_read(priv, (entity / 4), 0x1e,
			     PHY_SINGLE_LED_ON_CTRL(entity % 4), &cl45_data);

	if (state == 1)
		cl45_data |= LED_ON_EN;
	else
		cl45_data &= ~LED_ON_EN;

	an8855_phy_cl45_write(priv, (entity / 4), 0x1e,
			      PHY_SINGLE_LED_ON_CTRL(entity % 4), cl45_data);

	return 0;
}

static int an8855_led_init(struct an8855_switch_priv *priv)
{
	u32 val, id, tmp_id = 0;
	int ret;

	ret = an8855_led_set_mode(priv, AN8855_LED_MODE_USER_DEFINE);
	if (ret) {
		printf("an8855: led_set_mode failed with %d!\n", ret);
		return ret;
	}

	for (id = 0; id < ARRAY_SIZE(led_cfg); id++) {
		ret = an8855_led_set_state(priv, led_cfg[id].phy_led_idx,
					   led_cfg[id].en);
		if (ret != 0) {
			printf("an8855: led_set_state failed with %d!\n", ret);
			return ret;
		}

		if (led_cfg[id].en == 1) {
			an8855_led_set_usr_def(priv,
					       led_cfg[id].phy_led_idx,
					       led_cfg[id].pol,
					       led_cfg[id].on_cfg,
					       led_cfg[id].blk_cfg,
					       led_cfg[id].led_freq);
		}
	}

	/* Setting for System LED & Loop LED */
	an8855_reg_write(priv, AN8855_RG_GPIO_OE, 0x0);
	an8855_reg_write(priv, AN8855_RG_GPIO_CTRL, 0x0);
	an8855_reg_write(priv, AN8855_RG_GPIO_L_INV, 0);

	an8855_reg_write(priv, AN8855_RG_GPIO_CTRL, 0x1001);
	an8855_reg_read(priv, AN8855_RG_GPIO_DATA, &val);
	val |= GENMASK(3, 1);
	val &= ~(BIT(0));
	val &= ~(BIT(6));
	an8855_reg_write(priv, AN8855_RG_GPIO_DATA, val);

	an8855_reg_read(priv, AN8855_RG_GPIO_OE, &val);
	val |= 0x41;
	an8855_reg_write(priv, AN8855_RG_GPIO_OE, val);

	/* Mapping between GPIO & LED */
	val = 0;
	for (id = 0; id < ARRAY_SIZE(led_cfg); id++) {
		/* Skip GPIO6, due to GPIO6 does not support PORT LED */
		if (id == 6)
			continue;

		if (led_cfg[id].en == 1) {
			if (id < 7)
				val |= led_cfg[id].phy_led_idx << ((id % 4) * 8);
			else
				val |= led_cfg[id].phy_led_idx << (((id - 1) % 4) * 8);
		}

		if (id < 7)
			tmp_id = id;
		else
			tmp_id = id - 1;

		if ((tmp_id % 4) == 0x3) {
			an8855_reg_write(priv,
					 AN8855_RG_GPIO_LED_SEL(tmp_id / 4),
					 val);
			val = 0;
		}
	}

	/* Turn on LAN LED mode */
	val = 0;
	for (id = 0; id < ARRAY_SIZE(led_cfg); id++) {
		if (led_cfg[id].en == 1)
			val |= 0x1 << id;
	}
	an8855_reg_write(priv, AN8855_RG_GPIO_LED_MODE, val);

	/* Force clear blink pulse for per port LED */
	an8855_phy_cl45_write(priv, 0, 0x1f, PHY_LED_BLINK_DUR_CTRL, 0x1f);
	udelay(1000);
	an8855_phy_cl45_write(priv, 0, 0x1f, PHY_LED_BLINK_DUR_CTRL, 0);

	return 0;
}

static void an8855_port_isolation(struct an8855_switch_priv *priv)
{
	u32 i;

	for (i = 0; i < AN8855_NUM_PORTS; i++) {
		/* Set port matrix mode */
		if (i != 5)
			an8855_reg_write(priv, AN8855_PORTMATRIX_REG(i), 0x20);
		else
			an8855_reg_write(priv, AN8855_PORTMATRIX_REG(i), 0x1f);

		/* Set port mode to user port */
		an8855_reg_write(priv, AN8855_PVC(i),
				 (0x9100 << AN8855_STAG_VPID_S) |
				 (VLAN_ATTR_USER << AN8855_VLAN_ATTR_S));
	}
}

static void an8855_mac_control(struct mtk_eth_switch_priv *swpriv, bool enable)
{
	struct an8855_switch_priv *priv = (struct an8855_switch_priv *)swpriv;
	u32 pmcr = AN8855_FORCE_MODE_LNK;

	if (enable)
		pmcr = AN8855_FORCE_MODE;

	an8855_reg_write(priv, AN8855_PMCR_REG(5), pmcr);
}

static int an8855_mdio_read(struct mii_dev *bus, int addr, int devad, int reg)
{
	struct an8855_switch_priv *priv = bus->priv;

	if (devad < 0)
		return mtk_mii_read(priv->epriv.eth, addr, reg);

	return mtk_mmd_ind_read(priv->epriv.eth, addr, devad, reg);
}

static int an8855_mdio_write(struct mii_dev *bus, int addr, int devad, int reg,
			     u16 val)
{
	struct an8855_switch_priv *priv = bus->priv;

	if (devad < 0)
		return mtk_mii_write(priv->epriv.eth, addr, reg, val);

	return mtk_mmd_ind_write(priv->epriv.eth, addr, devad, reg, val);
}

static int an8855_mdio_register(struct an8855_switch_priv *priv)
{
	struct mii_dev *mdio_bus = mdio_alloc();
	int ret;

	if (!mdio_bus)
		return -ENOMEM;

	mdio_bus->read = an8855_mdio_read;
	mdio_bus->write = an8855_mdio_write;
	snprintf(mdio_bus->name, sizeof(mdio_bus->name), priv->epriv.sw->name);

	mdio_bus->priv = priv;

	ret = mdio_register(mdio_bus);
	if (ret) {
		mdio_free(mdio_bus);
		return ret;
	}

	priv->mdio_bus = mdio_bus;

	return 0;
}

static int an8855_setup(struct mtk_eth_switch_priv *swpriv)
{
	struct an8855_switch_priv *priv = (struct an8855_switch_priv *)swpriv;
	u16 phy_addr, phy_val;
	u32 i, id, val = 0;
	int ret;

	priv->phy_base = 1;

	/* Turn off PHYs */
	for (i = 0; i < AN8855_NUM_PHYS; i++) {
		phy_addr = AN8855_PHY_ADDR(priv->phy_base, i);
		phy_val = mtk_mii_read(priv->epriv.eth, phy_addr, MII_BMCR);
		phy_val |= BMCR_PDOWN;
		mtk_mii_write(priv->epriv.eth, phy_addr, MII_BMCR, phy_val);
	}

	/* Force MAC link down before reset */
	an8855_reg_write(priv, AN8855_PMCR_REG(5), AN8855_FORCE_MODE_LNK);

	/* Switch soft reset */
	an8855_reg_write(priv, AN8855_SYS_CTRL_REG, AN8855_SW_SYS_RST);
	mdelay(100);

	an8855_reg_read(priv, AN8855_PKG_SEL, &val);
	if ((val & 0x7) == PAG_SEL_AN8855H) {
		/* Release power down */
		an8855_reg_write(priv, RG_GPHY_AFE_PWD, 0x0);

		/* Invert for LED activity change */
		an8855_reg_read(priv, AN8855_RG_GPIO_L_INV, &val);
		for (id = 0; id < ARRAY_SIZE(led_cfg); id++) {
			if (led_cfg[id].pol == LED_HIGH && led_cfg[id].en == 1)
				val |= 0x1 << id;
		}
		an8855_reg_write(priv, AN8855_RG_GPIO_L_INV, (val | 0x1));

		/* MCU NOP CMD */
		an8855_reg_write(priv, AN8855_RG_GDMP_RAM, 0x846);
		an8855_reg_write(priv, AN8855_RG_GDMP_RAM + 4, 0x4a);

		/* Enable MCU */
		an8855_reg_read(priv, AN8855_RG_CLK_CPU_ICG, &val);
		an8855_reg_write(priv, AN8855_RG_CLK_CPU_ICG,
				 val | AN8855_MCU_ENABLE);
		udelay(1000);

		/* Disable MCU watchdog */
		an8855_reg_read(priv, AN8855_RG_TIMER_CTL, &val);
		an8855_reg_write(priv, AN8855_RG_TIMER_CTL,
				 (val & (~AN8855_WDOG_ENABLE)));

		/* LED settings for T830 reference board */
		ret = an8855_led_init(priv);
		if (ret < 0) {
			printf("an8855: an8855_led_init failed with %d\n", ret);
			return ret;
		}
	}

	switch (priv->epriv.phy_interface) {
	case PHY_INTERFACE_MODE_2500BASEX:
		an8855_port_sgmii_init(priv, 5);
		break;

	default:
		break;
	}

	an8855_reg_read(priv, AN8855_CKGCR, &val);
	val &= ~(0x3);
	an8855_reg_write(priv, AN8855_CKGCR, val);

	/* Enable port isolation to block inter-port communication */
	an8855_port_isolation(priv);

	/* Turn on PHYs */
	for (i = 0; i < AN8855_NUM_PHYS; i++) {
		phy_addr = AN8855_PHY_ADDR(priv->phy_base, i);
		phy_val = mtk_mii_read(priv->epriv.eth, phy_addr, MII_BMCR);
		phy_val &= ~BMCR_PDOWN;
		mtk_mii_write(priv->epriv.eth, phy_addr, MII_BMCR, phy_val);
	}

	return an8855_mdio_register(priv);
}

static int an8855_cleanup(struct mtk_eth_switch_priv *swpriv)
{
	struct an8855_switch_priv *priv = (struct an8855_switch_priv *)swpriv;

	mdio_unregister(priv->mdio_bus);

	return 0;
}

static int an8855_detect(struct mtk_eth_priv *priv)
{
	int ret;
	u32 val;

	ret = __an8855_reg_read(priv, 1, 0x10005000, &val);
	if (ret)
		return ret;

	if (val == 0x8855)
		return 0;

	return -ENODEV;
}

MTK_ETH_SWITCH(an8855) = {
	.name = "an8855",
	.desc = "Airoha AN8855",
	.priv_size = sizeof(struct an8855_switch_priv),
	.reset_wait_time = 100,

	.detect = an8855_detect,
	.setup = an8855_setup,
	.cleanup = an8855_cleanup,
	.mac_control = an8855_mac_control,
};
