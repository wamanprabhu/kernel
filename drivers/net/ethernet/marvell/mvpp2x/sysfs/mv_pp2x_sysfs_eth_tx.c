/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.


********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
*******************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/capability.h>
#include <linux/platform_device.h>
#include <linux/netdevice.h>

#include "mv_pp2x_sysfs.h"

static ssize_t mv_pp2_help(char *b)
{
	int o = 0; /* buffer offset */
	int s = PAGE_SIZE; /* buffer size */

	o += scnprintf(b+o, s-o, "cat                              txRegs          - show global TX registers\n");
	o += scnprintf(b+o, s-o, "echo [p] [txq]                   > pTxqCounters  - show TXQ Counters for port <p/txp/txq> where <txq> range [0..7]\n");
	o += scnprintf(b+o, s-o, "echo [p] [txq]                   > pTxqRegs      - show TXQ registers for port <p/txp/txq> where <txq> range [0..7]\n");
	o += scnprintf(b+o, s-o, "echo [txq]                       > gTxqRegs      - show TXQ registers for global <txq> range [0..255]\n");
	o += scnprintf(b+o, s-o, "echo [cpu]                       > aggrTxqRegs   - show Aggregation TXQ registers for <cpu> range [0..max]\n");
	o += scnprintf(b+o, s-o, "echo [cpu] [v]                   > aggrTxqShow   - show aggregated TXQ descriptors ring for <cpu>.\n");
	o += scnprintf(b+o, s-o, "echo [p]  [txq] [v]              > txqShow       - show TXQ descriptors ring for <p/txp/txq>. v: 0-brief, 1-full\n");
//	o += scnprintf(b+o, s-o, "echo [p] [hex]                   > txFlags       - set TX flags. bits: 0-no_pad, 1-mh, 2-hw_cmd\n");
//	o += scnprintf(b+o, s-o, "echo [p] [hex]                   > txMH          - set 2 bytes of Marvell Header for transmit\n");
//	o += scnprintf(b+o, s-o, "echo [p] [txp] [txq] [cpu]       > txqDef        - set default <txp/txq> for packets sent to port <p> by <cpu>\n");
//	o += scnprintf(b+o, s-o, "echo [p] [txp] [txq] [v]         > txqSize       - set TXQ size <v> for <p/txp/txq>.\n");
//	o += scnprintf(b+o, s-o, "echo [p] [txp] [txq] [hwf] [swf] > txqLimit      - set HWF <hwf> and SWF <swf> limits for <p/txp/txq>.\n");
//	o += scnprintf(b+o, s-o, "echo [p] [txp] [txq] [v]         > txqChunk      - set SWF request chunk [v] for <p/txp/txq>\n");
//#ifdef CONFIG_MV_PP2_TXDONE_IN_HRTIMER
//	o += scnprintf(b+o, s-o, "echo [period]                    > txPeriod      - set Tx Done high resolution timer period\n");
//	o += scnprintf(b+o, s-o, "				     [period]: period range is [%lu, %lu], unit usec\n",
//		MV_PP2_HRTIMER_PERIOD_MIN, MV_PP2_HRTIMER_PERIOD_MAX);
//#endif
	return o;
}

static ssize_t mv_pp2_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	const char      *name = attr->attr.name;
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "txRegs"))
		mvPp2TxRegs(sysfs_cur_priv);
	else
		off = mv_pp2_help(buf);

	return off;
}

static ssize_t mv_pp2_txq_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    p, v, a, b, c;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read port and value */
	err = p = v = a = b = c = 0;
	sscanf(buf, "%d %d %d %d %d", &p, &v, &a, &b, &c);

	local_irq_save(flags);

	if (!strcmp(name, "txqShow")) {
		mvPp2TxqShow(sysfs_cur_priv, p, v, a);
	}  else if (!strcmp(name, "aggrTxqShow")) {
		mvPp2AggrTxqShow(sysfs_cur_priv, p, v);
	} else if (!strcmp(name, "gTxqRegs")) {
		mvPp2PhysTxqRegs(sysfs_cur_priv, p);
	} else if (!strcmp(name, "pTxqRegs")) {
		mvPp2PortTxqRegs(sysfs_cur_priv, p, v);
	} else if (!strcmp(name, "pTxqCounters")) {
		mvPp2V1TxqDbgCntrs(sysfs_cur_priv, p, v);
	} else if (!strcmp(name, "aggrTxqRegs")) {
		mvPp2AggrTxqRegs(sysfs_cur_priv, p);
	} else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help,         S_IRUSR, mv_pp2_show, NULL);
static DEVICE_ATTR(txRegs,       S_IRUSR, mv_pp2_show, NULL);
static DEVICE_ATTR(aggrTxqRegs,  S_IWUSR, NULL, mv_pp2_txq_store);
static DEVICE_ATTR(pTxqCounters, S_IWUSR, NULL, mv_pp2_txq_store);
static DEVICE_ATTR(txqShow,      S_IWUSR, NULL, mv_pp2_txq_store);
static DEVICE_ATTR(gTxqRegs,     S_IWUSR, NULL, mv_pp2_txq_store);
static DEVICE_ATTR(pTxqRegs,     S_IWUSR, NULL, mv_pp2_txq_store);
static DEVICE_ATTR(aggrTxqShow,  S_IWUSR, NULL, mv_pp2_txq_store);


static struct attribute *mv_pp2_tx_attrs[] = {
	&dev_attr_pTxqCounters.attr,
	&dev_attr_aggrTxqRegs.attr,
	&dev_attr_help.attr,
	&dev_attr_txRegs.attr,
	&dev_attr_txqShow.attr,
	&dev_attr_gTxqRegs.attr,
	&dev_attr_pTxqRegs.attr,
	&dev_attr_aggrTxqShow.attr,
	NULL
};

static struct attribute_group mv_pp2_tx_group = {
	.name = "tx",
	.attrs = mv_pp2_tx_attrs,
};

int mv_pp2_tx_sysfs_init(struct kobject *gbe_kobj)
{
	int err;

	err = sysfs_create_group(gbe_kobj, &mv_pp2_tx_group);
	if (err)
		pr_err("sysfs group %s failed %d\n", mv_pp2_tx_group.name, err);

	return err;
}

int mv_pp2_tx_sysfs_exit(struct kobject *gbe_kobj)
{
	sysfs_remove_group(gbe_kobj, &mv_pp2_tx_group);

	return 0;
}

