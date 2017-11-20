/*
* ***************************************************************************
* Copyright (C) 2016 Marvell International Ltd.
* ***************************************************************************
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the Free
* Software Foundation, either version 2 of the License, or any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ***************************************************************************
*/

#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/skbuff.h>
#include <linux/inetdevice.h>
#include <linux/mbus.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/cpumask.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_mdio.h>
#include <linux/of_net.h>
#include <linux/of_address.h>
#include <linux/of_device.h>

#include <linux/phy.h>
#include <linux/clk.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <uapi/linux/ppp_defs.h>
#include <net/ip.h>
#include <net/ipv6.h>

#include "mv_pp2x.h"
#include "mv_pp2x_hw.h"
#include "mv_pp2x_sysfs_debug.h"

void mv_pp2x_print_reg(struct mv_pp2x_hw *hw, unsigned int reg_addr,
		       char *reg_name)
{
	DBG_MSG("  %-32s: 0x%x = 0x%08x\n", reg_name, reg_addr,
		mv_pp2x_read(hw, reg_addr));
}
EXPORT_SYMBOL(mv_pp2x_print_reg);

void mv_pp2x_print_reg2(struct mv_pp2x_hw *hw, unsigned int reg_addr,
			char *reg_name, unsigned int index)
{
	char buf[64];

	sprintf(buf, "%s[%d]", reg_name, index);
	DBG_MSG("  %-32s: 0x%x = 0x%08x\n", reg_name, reg_addr,
		mv_pp2x_read(hw, reg_addr));
}
EXPORT_SYMBOL(mv_pp2x_print_reg2);

void mv_pp2x_bm_pool_regs(struct mv_pp2x_hw *hw, int pool)
{
	if (mv_pp2x_max_check(pool, MVPP2_BM_POOLS_NUM, "bm_pool"))
		return;

	DBG_MSG("\n[BM pool registers: pool=%d]\n", pool);
	mv_pp2x_print_reg(hw, MVPP2_BM_POOL_BASE_ADDR_REG(pool),
			  "MV_BM_POOL_BASE_REG");
	mv_pp2x_print_reg(hw, MVPP2_BM_POOL_SIZE_REG(pool),
			  "MVPP2_BM_POOL_SIZE_REG");
	mv_pp2x_print_reg(hw, MVPP2_BM_POOL_READ_PTR_REG(pool),
			  "MVPP2_BM_POOL_READ_PTR_REG");
	mv_pp2x_print_reg(hw, MVPP2_BM_POOL_PTRS_NUM_REG(pool),
			  "MVPP2_BM_POOL_PTRS_NUM_REG");
	mv_pp2x_print_reg(hw, MVPP2_BM_BPPI_READ_PTR_REG(pool),
			  "MVPP2_BM_BPPI_READ_PTR_REG");
	mv_pp2x_print_reg(hw, MVPP2_BM_BPPI_PTRS_NUM_REG(pool),
			  "MVPP2_BM_BPPI_PTRS_NUM_REG");
	mv_pp2x_print_reg(hw, MVPP2_BM_POOL_CTRL_REG(pool),
			  "MVPP2_BM_POOL_CTRL_REG");
	mv_pp2x_print_reg(hw, MVPP2_BM_INTR_CAUSE_REG(pool),
			  "MVPP2_BM_INTR_CAUSE_REG");
	mv_pp2x_print_reg(hw, MVPP2_BM_INTR_MASK_REG(pool),
			  "MVPP2_BM_INTR_MASK_REG");
}
EXPORT_SYMBOL(mv_pp2x_bm_pool_regs);

void mv_pp2x_bm_pool_drop_count(struct mv_pp2x_hw *hw, int pool)
{
	if (mv_pp2x_max_check(pool, MVPP2_BM_POOLS_NUM, "bm_pool"))
		return;

	mv_pp2x_print_reg2(hw, MVPP2_BM_DROP_CNTR_REG(pool),
			   "MVPP2_BM_DROP_CNTR_REG", pool);
	mv_pp2x_print_reg2(hw, MVPP2_BM_MC_DROP_CNTR_REG(pool),
			   "MVPP2_BM_MC_DROP_CNTR_REG", pool);
}
EXPORT_SYMBOL(mv_pp2x_bm_pool_drop_count);

void mv_pp2x_pool_status(struct mv_pp2x *priv, int log_pool_num)
{
	struct mv_pp2x_bm_pool *bm_pool = NULL;
	int /*buf_size,*/ total_size, i, pool;

	if (mv_pp2x_max_check(log_pool_num, MVPP2_BM_SWF_NUM_POOLS,
			      "log_pool"))
		return;

	for (i = 0; i < priv->num_pools; i++) {
		if (priv->bm_pools[i].log_id == log_pool_num) {
			bm_pool = &priv->bm_pools[i];
			pool = bm_pool->id;
		}
	}
	if (!bm_pool) {
		pr_err("%s: Logical BM pool %d is not initialized\n",
		       __func__, log_pool_num);
		return;
	}

	total_size = RX_TOTAL_SIZE(bm_pool->buf_size);

	DBG_MSG(
		"\n%12s log_pool=%d, phy_pool=%d: pkt_size=%d, buf_size=%d total_size=%d\n",
		mv_pp2x_pool_description_get(log_pool_num), log_pool_num,
		pool, bm_pool->pkt_size, bm_pool->buf_size, total_size);
	DBG_MSG("\tcapacity=%d, buf_num=%d, in_use_thresh=%u\n",
		bm_pool->size, bm_pool->buf_num,
		bm_pool->in_use_thresh);
}
EXPORT_SYMBOL(mv_pp2x_pool_status);

void mv_pp2_pool_stats_print(struct mv_pp2x *priv, int log_pool_num)
{
	int i, pool;
	struct mv_pp2x_bm_pool *bm_pool = NULL;

	if (mv_pp2x_max_check(log_pool_num, MVPP2_BM_SWF_NUM_POOLS,
			      "log_pool"))
		return;

	for (i = 0; i < priv->num_pools; i++) {
		if (priv->bm_pools[i].log_id == log_pool_num) {
			bm_pool = &priv->bm_pools[i];
			pool = bm_pool->id;
		}
	}
	if (!bm_pool) {
		pr_err("%s: Logical BM pool %d is not initialized\n",
		       __func__, log_pool_num);
		return;
	}
}
EXPORT_SYMBOL(mv_pp2_pool_stats_print);

void mvPp2RxDmaRegsPrint(struct mv_pp2x *priv, bool print_all,
			 int start, int stop)
{
	int i, num_rx_queues, result;
	bool enabled;

	struct mv_pp2x_hw *hw = &priv->hw;

	num_rx_queues = (MVPP2_MAX_PORTS * priv->pp2xdata->pp2x_max_port_rxqs);
	if (stop >= num_rx_queues || start > stop || start < 0) {
		DBG_MSG("\nERROR: wrong inputs\n");
		return;
	}

	DBG_MSG("\n[RX DMA regs]\n");
	DBG_MSG("\nRXQs [0..%d], registers\n", num_rx_queues - 1);

	for (i = start; i <= stop; i++) {
		if (!print_all) {
			result = mv_pp2x_read(hw, MVPP2_RXQ_CONFIG_REG(i));
			enabled = !(result & MVPP2_RXQ_DISABLE_MASK);
		}
		if (print_all || enabled) {
			DBG_MSG("RXQ[%d]:\n", i);
			mv_pp2x_print_reg(hw, MVPP2_RXQ_STATUS_REG(i),
					  "MVPP2_RX_STATUS");
			mv_pp2x_print_reg2(hw, MVPP2_RXQ_CONFIG_REG(i),
					   "MVPP2_RXQ_CONFIG_REG", i);
		}
	}
	DBG_MSG("\nBM pools [0..%d] registers\n", MVPP2_BM_POOLS_NUM - 1);
	for (i = 0; i < MVPP2_BM_POOLS_NUM; i++) {
		if (!print_all) {
			enabled = mv_pp2x_read(hw, MVPP2_BM_POOL_CTRL_REG(i)) &
				MVPP2_BM_STATE_MASK;
		}
		if (print_all || enabled) {
			DBG_MSG("POOL[%d]:\n", i);
			mv_pp2x_print_reg2(hw, MVPP2_POOL_BUF_SIZE_REG(i),
					   "MVPP2_POOL_BUF_SIZE_REG", i);
		}
	}
	DBG_MSG("\nIngress ports [0..%d] registers\n", MVPP2_MAX_PORTS - 1);
	for (i = 0; i < MVPP2_MAX_PORTS; i++) {
		mv_pp2x_print_reg2(hw, MVPP2_RX_CTRL_REG(i),
				   "MVPP2_RX_CTRL_REG", i);
	}
	DBG_MSG("\n");
}
EXPORT_SYMBOL(mvPp2RxDmaRegsPrint);

static void mvPp2RxQueueDetailedShow(struct mv_pp2x *priv,
				     struct mv_pp2x_rx_queue *pp_rxq)
{
	int i;
	struct mv_pp2x_rx_desc *rx_desc = pp_rxq->first_desc;

	for (i = 0; i < pp_rxq->size; i++) {
		DBG_MSG("%3d. desc=%p, status=%08x, data_size=%4d, ",
			i, rx_desc+i, rx_desc[i].status,
			rx_desc[i].data_size);
		if (priv->pp2_version == PPV21) {
			DBG_MSG("buf_addr=%lx, buf_cookie=%p, ",
				(unsigned long)
				mv_pp21_rxdesc_phys_addr_get(&rx_desc[i]),
				mv_pp21_rxdesc_cookie_get(&rx_desc[i]));
		} else {
			DBG_MSG("buf_addr=%lx, buf_cookie=%p, ",
				(unsigned long)
				mv_pp22_rxdesc_phys_addr_get(&rx_desc[i]),
				mv_pp22_rxdesc_cookie_get(&rx_desc[i]));
		}
		DBG_MSG("parser_info=%03x\n", rx_desc[i].rsrvd_parser);
	}
}

/* Show Port/Rxq descriptors ring */
void mvPp2RxqShow(struct mv_pp2x *priv, int port, int rxq, int mode)
{
	struct mv_pp2x_port *pp_port;
	struct mv_pp2x_rx_queue *pp_rxq;

	pp_port = mv_pp2x_port_struct_get(priv, port);

	if (pp_port == NULL) {
		DBG_MSG("port #%d is not initialized\n", port);
		return;
	}

	if (mv_pp2x_max_check(rxq, pp_port->num_rx_queues, "logical rxq"))
		return;

	pp_rxq = pp_port->rxqs[rxq];

	if (pp_rxq->first_desc == NULL) {
		DBG_MSG("rxq #%d of port #%d is not initialized\n", rxq, port);
		return;
	}

	DBG_MSG("\n[PPv2 RxQ show: port=%d, logical rxq=%d -> phys rxq=%d]\n",
		port, pp_rxq->log_id, pp_rxq->id);

	DBG_MSG("size=%d, pkts_coal=%d, time_coal=%d\n",
		pp_rxq->size, pp_rxq->pkts_coal, pp_rxq->time_coal);
	preempt_disable();
	DBG_MSG(
		"first_virt_addr=%p, first_dma_addr=%lx, next_rx_desc=%d, rxq_cccupied=%d, rxq_nonoccupied=%d\n",
		pp_rxq->first_desc,
		(unsigned long)MVPP2_DESCQ_MEM_ALIGN(pp_rxq->descs_phys),
		pp_rxq->next_desc_to_proc,
		mv_pp2x_rxq_received(pp_port, pp_rxq->id),
		mv_pp2x_rxq_free(pp_port, pp_rxq->id));
	preempt_enable();
	DBG_MSG("virt_mem_area_addr=%p, dma_mem_area_addr=%lx\n",
		pp_rxq->desc_mem, (unsigned long)pp_rxq->descs_phys);

	if (mode)
		mvPp2RxQueueDetailedShow(priv, pp_rxq);
}
EXPORT_SYMBOL(mvPp2RxqShow);

void mvPp2PhysRxqRegs(struct mv_pp2x *pp2, int rxq)
{
	struct mv_pp2x_hw *hw  = &pp2->hw;

	DBG_MSG("\n[PPv2 RxQ registers: global rxq=%d]\n", rxq);

	mv_pp2x_write(hw, MVPP2_RXQ_NUM_REG, rxq);
	mv_pp2x_print_reg(hw, MVPP2_RXQ_NUM_REG,
			  "MVPP2_RXQ_NUM_REG");
	mv_pp2x_print_reg(hw, MVPP2_RXQ_DESC_ADDR_REG,
			  "MVPP2_RXQ_DESC_ADDR_REG");
	mv_pp2x_print_reg(hw, MVPP2_RXQ_DESC_SIZE_REG,
			  "MVPP2_RXQ_DESC_SIZE_REG");
	mv_pp2x_print_reg(hw, MVPP2_RXQ_STATUS_REG(rxq),
			  "MVPP2_RXQ_STATUS_REG");
	mv_pp2x_print_reg(hw, MVPP2_RXQ_THRESH_REG,
			  "MVPP2_RXQ_THRESH_REG");
	mv_pp2x_print_reg(hw, MVPP2_RXQ_INDEX_REG,
			  "MVPP2_RXQ_INDEX_REG");
	mv_pp2x_print_reg(hw, MVPP2_RXQ_CONFIG_REG(rxq),
			  "MVPP2_RXQ_CONFIG_REG");
}
EXPORT_SYMBOL(mvPp2PhysRxqRegs);

void mvPp2PortRxqRegs(struct mv_pp2x *pp2, int port, int rxq)
{
	int phy_rxq;
	struct mv_pp2x_port *pp2_port = mv_pp2x_port_struct_get(pp2, port);

	DBG_MSG("\n[PPv2 RxQ registers: port=%d, local rxq=%d]\n", port, rxq);

	if (rxq >= pp2_port->num_rx_queues)
		return;

	if (!pp2_port)
		return;

	phy_rxq = pp2_port->first_rxq + rxq;
	mvPp2PhysRxqRegs(pp2, phy_rxq);
}
EXPORT_SYMBOL(mvPp2PortRxqRegs);


void mv_pp22_isr_rx_group_regs(struct mv_pp2x *priv, int port, bool print_all)
{
	int val, i, num_threads, cpu_offset, cpu;
	struct mv_pp2x_hw *hw = &priv->hw;
	struct mv_pp2x_port *pp_port;


	pp_port = mv_pp2x_port_struct_get(priv, port);
	if (!pp_port) {
		DBG_MSG("Input Error\n %s", __func__);
		return;
	}

	if (print_all)
		num_threads = MVPP2_MAX_SW_THREADS;
	else
		num_threads = pp_port->num_qvector;

	for (i = 0; i < num_threads; i++) {
		DBG_MSG("\n[PPv2 RxQ GroupConfig registers: port=%d cpu=%d]",
				port, i);

		val = (port << MVPP22_ISR_RXQ_GROUP_INDEX_GROUP_OFFSET) | i;
		mv_pp2x_write(hw, MVPP22_ISR_RXQ_GROUP_INDEX_REG, val);

		mv_pp2x_print_reg(hw, MVPP22_ISR_RXQ_GROUP_INDEX_REG,
				  "MVPP22_ISR_RXQ_GROUP_INDEX_REG");
		mv_pp2x_print_reg(hw, MVPP22_ISR_RXQ_SUB_GROUP_CONFIG_REG,
				  "MVPP22_ISR_RXQ_SUB_GROUP_CONFIG_REG");
		/*reg_val = mv_pp2x_read(hw,
		 *	MVPP22_ISR_RXQ_SUB_GROUP_CONFIG_REG);
		 */
		/*start_queue  = reg_val &
		 *	MVPP22_ISR_RXQ_SUB_GROUP_STARTQ_MASK;
		 */
		/*sub_group_size = reg_val &
		 *	MVPP22_ISR_RXQ_SUB_GROUP_SIZE_MASK;
		 */
	}
	DBG_MSG("\n[PPv2 Port Interrupt Enable register : port=%d]\n", port);
	mv_pp2x_print_reg(hw, MVPP2_ISR_ENABLE_REG(port),
			  "MVPP2_ISR_ENABLE_REG");

	DBG_MSG("\n[PPv2 Eth Occupied Interrupt registers: port=%d]\n", port);
	for (i = 0; i < num_threads; i++) {
		if (print_all)
			cpu = i;
		else
			cpu = pp_port->q_vector[i].sw_thread_id;
		cpu_offset = cpu*MVPP2_ADDR_SPACE_SIZE;
		DBG_MSG("cpu=%d]\n", cpu);
		mv_pp2x_print_reg(hw, cpu_offset +
				  MVPP2_ISR_RX_TX_CAUSE_REG(port),
				  "MVPP2_ISR_RX_TX_CAUSE_REG");
		mv_pp2x_print_reg(hw, cpu_offset +
				  MVPP2_ISR_RX_TX_MASK_REG(port),
				  "MVPP2_ISR_RX_TX_MASK_REG");
	}

}
EXPORT_SYMBOL(mv_pp22_isr_rx_group_regs);

static void mvPp2TxQueueDetailedShow(struct mv_pp2x *priv,
				     void *pp_txq, bool aggr_queue)
{
	int i, j, size;
	struct mv_pp2x_tx_desc *tx_desc;

	if (aggr_queue) {
		size = ((struct mv_pp2x_aggr_tx_queue *)pp_txq)->size;
		tx_desc = ((struct mv_pp2x_aggr_tx_queue *)pp_txq)->first_desc;
	} else {
		size = ((struct mv_pp2x_tx_queue *)pp_txq)->size;
		tx_desc = ((struct mv_pp2x_tx_queue *)pp_txq)->first_desc;
	}

	for (i = 0; i < 16/*size*/; i++) {
		DBG_MSG(
			"%3d. desc=%p, cmd=%x, data_size=%-4d pkt_offset=%-3d, phy_txq=%d\n",
		   i, tx_desc+i, tx_desc[i].command, tx_desc[i].data_size,
		   tx_desc[i].packet_offset, tx_desc[i].phys_txq);
		if (priv->pp2_version == PPV21) {
			DBG_MSG("buf_phys_addr=%08x, buf_cookie=%08x\n",
				tx_desc[i].u.pp21.buf_phys_addr,
				tx_desc[i].u.pp21.buf_cookie);
			DBG_MSG(
				"hw_cmd[0]=%x, hw_cmd[1]=%x, hw_cmd[2]=%x, rsrvd1=%x\n",
				tx_desc[i].u.pp21.rsrvd_hw_cmd[0],
				tx_desc[i].u.pp21.rsrvd_hw_cmd[1],
				tx_desc[i].u.pp21.rsrvd_hw_cmd[2],
				tx_desc[i].u.pp21.rsrvd1);
		} else {
			DBG_MSG(
				"     rsrvd_hw_cmd1=%llx, buf_phys_addr_cmd2=%llx, buf_cookie_bm_cmd3=%llx\n",
				tx_desc[i].u.pp22.rsrvd_hw_cmd1,
				tx_desc[i].u.pp22.buf_phys_addr_hw_cmd2,
				tx_desc[i].u.pp22.buf_cookie_bm_qset_hw_cmd3);

			for (j = 0; j < 8; j++)
				DBG_MSG("%d:%x\n", j, *((u32 *)(tx_desc+i)+j));
		}
	}
}

/* Show Port/TXQ descriptors ring */
void mvPp2TxqShow(struct mv_pp2x *priv, int port, int txq, int mode)
{
	struct mv_pp2x_port *pp_port;
	struct mv_pp2x_tx_queue *pp_txq;
	struct mv_pp2x_txq_pcpu *txq_pcpu;
	int cpu;

	pp_port = mv_pp2x_port_struct_get(priv, port);

	if (pp_port == NULL) {
		DBG_MSG("port #%d is not initialized\n", port);
		return;
	}

	if (mv_pp2x_max_check(txq, pp_port->num_tx_queues, "logical txq"))
		return;

	pp_txq = pp_port->txqs[txq];

	if (pp_txq->first_desc == NULL) {
		DBG_MSG("txq #%d of port #%d is not initialized\n", txq, port);
		return;
	}

	DBG_MSG("\n[PPv2 TxQ show: port=%d, logical_txq=%d]\n",
		port, pp_txq->log_id);

	DBG_MSG("physical_txq=%d, size=%d, pkts_coal=%d\n",
		pp_txq->id, pp_txq->size, pp_txq->pkts_coal);

	DBG_MSG("first_virt_addr=%p, first_dma_addr=%lx, next_tx_desc=%d\n",
		pp_txq->first_desc,
		(unsigned long)MVPP2_DESCQ_MEM_ALIGN(pp_txq->descs_phys),
		pp_txq->next_desc_to_proc);
	DBG_MSG("virt_mem_area_addr=%p, dma_mem_area_addr=%lx\n",
		pp_txq->desc_mem, (unsigned long)pp_txq->descs_phys);

	for_each_online_cpu(cpu) {
		txq_pcpu = per_cpu_ptr(pp_txq->pcpu, cpu);
		DBG_MSG("\n[PPv2 TxQ %d cpu=%d show:\n", txq, cpu);

		DBG_MSG("cpu=%d, size=%d, reserved_num=%d\n",
			txq_pcpu->cpu, txq_pcpu->size,
			txq_pcpu->reserved_num);
		DBG_MSG("txq_put_index=%d, txq_get_index=%d\n",
			txq_pcpu->txq_put_index, txq_pcpu->txq_get_index);
		DBG_MSG("tx_skb=%p, tx_buffs=%p\n",
			txq_pcpu->tx_skb, txq_pcpu->tx_buffs);
	}

	if (mode)
		mvPp2TxQueueDetailedShow(priv, pp_txq, 0);
}
EXPORT_SYMBOL(mvPp2TxqShow);

/* Show CPU aggregation TXQ descriptors ring */
void mvPp2AggrTxqShow(struct mv_pp2x *priv, int cpu, int mode)
{

	struct mv_pp2x_aggr_tx_queue *aggr_queue = NULL;
	int i;

	DBG_MSG("\n[PPv2 AggrTxQ: cpu=%d]\n", cpu);

	for (i = 0; i < priv->num_aggr_qs; i++) {
		if (priv->aggr_txqs[i].id == cpu) {
			aggr_queue = &priv->aggr_txqs[i];
			break;
		}
	}
	if (!aggr_queue) {
		DBG_MSG("aggr_txq for cpu #%d is not initialized\n", cpu);
		return;
	}

	DBG_MSG("id=%d, size=%d, count=%d, next_desc=%d, pending_cntr=%d\n",
		aggr_queue->id,
		aggr_queue->size, aggr_queue->count,
		aggr_queue->next_desc_to_proc,
		mv_pp2x_aggr_desc_num_read(priv, cpu));

	if (mode)
		mvPp2TxQueueDetailedShow(priv, aggr_queue, 1);

}
EXPORT_SYMBOL(mvPp2AggrTxqShow);

void mvPp2PhysTxqRegs(struct mv_pp2x *priv, int txq)
{
	struct mv_pp2x_hw *hw = &priv->hw;

	DBG_MSG("\n[PPv2 TxQ registers: global txq=%d]\n", txq);

	if (mv_pp2x_max_check(txq, MVPP2_TXQ_TOTAL_NUM, "global txq"))
		return;

	mv_pp2x_write(hw, MVPP2_TXQ_NUM_REG, txq);
	mv_pp2x_print_reg(hw, MVPP2_TXQ_NUM_REG,
			  "MVPP2_TXQ_NUM_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXQ_DESC_ADDR_LOW_REG,
			  "MVPP2_TXQ_DESC_ADDR_LOW_REG");
	if (priv->pp2_version == PPV22)
		mv_pp2x_print_reg(hw, MVPP22_TXQ_DESC_ADDR_HIGH_REG,
				  "MVPP22_TXQ_DESC_ADDR_HIGH_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXQ_DESC_SIZE_REG,
			  "MVPP2_TXQ_DESC_SIZE_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXQ_DESC_HWF_SIZE_REG,
			  "MVPP2_TXQ_DESC_HWF_SIZE_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXQ_INDEX_REG,
			  "MVPP2_TXQ_INDEX_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXQ_PREF_BUF_REG,
			  "MVPP2_TXQ_PREF_BUF_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXQ_PENDING_REG,
			  "MVPP2_TXQ_PENDING_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXQ_INT_STATUS_REG,
			  "MVPP2_TXQ_INT_STATUS_REG");
}
EXPORT_SYMBOL(mvPp2PhysTxqRegs);

void mvPp2PortTxqRegs(struct mv_pp2x *priv, int port, int txq)
{
	struct mv_pp2x_port *pp2_port;

	pp2_port = mv_pp2x_port_struct_get(priv, port);

	if (mv_pp2x_max_check(txq, pp2_port->num_tx_queues, "port txq"))
		return;

	DBG_MSG("\n[PPv2 TxQ registers: port=%d, local txq=%d]\n", port, txq);

	mvPp2PhysTxqRegs(priv, pp2_port->txqs[txq]->id);
}
EXPORT_SYMBOL(mvPp2PortTxqRegs);

void mvPp2AggrTxqRegs(struct mv_pp2x *priv, int cpu)
{
	struct mv_pp2x_hw *hw = &priv->hw;

	DBG_MSG("\n[PP2 Aggr TXQ registers: cpu=%d]\n", cpu);

	mv_pp2x_print_reg(hw, MVPP2_AGGR_TXQ_DESC_ADDR_REG(cpu),
			  "MVPP2_AGGR_TXQ_DESC_ADDR_REG");
	mv_pp2x_print_reg(hw, MVPP2_AGGR_TXQ_DESC_SIZE_REG(cpu),
			  "MVPP2_AGGR_TXQ_DESC_SIZE_REG");
	mv_pp2x_print_reg(hw, MVPP2_AGGR_TXQ_STATUS_REG(cpu),
			  "MVPP2_AGGR_TXQ_STATUS_REG");
	mv_pp2x_print_reg(hw, MVPP2_AGGR_TXQ_INDEX_REG(cpu),
			  "MVPP2_AGGR_TXQ_INDEX_REG");
}
EXPORT_SYMBOL(mvPp2AggrTxqRegs);

void mvPp2V1TxqDbgCntrs(struct mv_pp2x *priv, int port, int txq)
{
	struct mv_pp2x_hw *hw = &priv->hw;

	DBG_MSG("\n------ [Port #%d txq #%d counters] -----\n", port, txq);
	mv_pp2x_write(hw, MVPP2_CNT_IDX_REG, MVPP2_CNT_IDX_TX(port, txq));
	mv_pp2x_print_reg(hw, MVPP2_CNT_IDX_REG,
			  "MVPP2_CNT_IDX_REG");
	mv_pp2x_print_reg(hw, MVPP2_TX_DESC_ENQ_REG,
			  "MVPP2_TX_DESC_ENQ_REG");
	mv_pp2x_print_reg(hw, MVPP2_TX_DESC_ENQ_TO_DRAM_REG,
			  "MVPP2_TX_DESC_ENQ_TO_DRAM_REG");
	mv_pp2x_print_reg(hw, MVPP2_TX_BUF_ENQ_TO_DRAM_REG,
			  "MVPP2_TX_BUF_ENQ_TO_DRAM_REG");
	mv_pp2x_print_reg(hw, MVPP2_TX_DESC_HWF_ENQ_REG,
			  "MVPP2_TX_DESC_HWF_ENQ_REG");
	mv_pp2x_print_reg(hw, MVPP2_TX_PKT_DQ_REG,
			  "MVPP2_TX_PKT_DQ_REG");
	mv_pp2x_print_reg(hw, MVPP2_TX_PKT_FULLQ_DROP_REG,
			  "MVPP2_TX_PKT_FULLQ_DROP_REG");
	mv_pp2x_print_reg(hw, MVPP2_TX_PKT_EARLY_DROP_REG,
			  "MVPP2_TX_PKT_EARLY_DROP_REG");
	mv_pp2x_print_reg(hw, MVPP2_TX_PKT_BM_DROP_REG,
			  "MVPP2_TX_PKT_BM_DROP_REG");
	mv_pp2x_print_reg(hw, MVPP2_TX_PKT_BM_MC_DROP_REG,
			  "MVPP2_TX_PKT_BM_MC_DROP_REG");
}
EXPORT_SYMBOL(mvPp2V1TxqDbgCntrs);

void mvPp2V1DropCntrs(struct mv_pp2x *priv, int port)
{
	int q;
	struct mv_pp2x_hw *hw = &priv->hw;
	struct mv_pp2x_port *pp2_port = mv_pp2x_port_struct_get(priv, port);

	DBG_MSG("\n[global drop counters]\n");
	mv_pp2x_print_reg(hw, MVPP2_V1_OVERFLOW_MC_DROP_REG,
			  "MV_PP2_OVERRUN_DROP_REG");

	DBG_MSG("\n[Port #%d Drop counters]\n", port);
	mv_pp2x_print_reg(hw, MV_PP2_OVERRUN_DROP_REG(port),
			  "MV_PP2_OVERRUN_DROP_REG");
	mv_pp2x_print_reg(hw, MV_PP2_CLS_DROP_REG(port),
			  "MV_PP2_CLS_DROP_REG");

	for (q = 0; q < pp2_port->num_tx_queues; q++) {
		DBG_MSG("\n------ [Port #%d txp #%d txq #%d counters] -----\n",
				port, port, q);
		mv_pp2x_write(hw, MVPP2_CNT_IDX_REG, MVPP2_CNT_IDX_TX(port, q));
		mv_pp2x_print_reg(hw, MVPP2_TX_PKT_FULLQ_DROP_REG,
				  "MV_PP2_TX_PKT_FULLQ_DROP_REG");
		mv_pp2x_print_reg(hw, MVPP2_TX_PKT_EARLY_DROP_REG,
				  "MV_PP2_TX_PKT_EARLY_DROP_REG");
		mv_pp2x_print_reg(hw, MVPP2_TX_PKT_BM_DROP_REG,
				  "MV_PP2_TX_PKT_BM_DROP_REG");
		mv_pp2x_print_reg(hw, MVPP2_TX_PKT_BM_MC_DROP_REG,
				  "MV_PP2_TX_PKT_BM_MC_DROP_REG");
	}

	for (q = pp2_port->first_rxq; q < (pp2_port->first_rxq +
			pp2_port->num_rx_queues); q++) {
		DBG_MSG("\n------ [Port #%d, rxq #%d counters] -----\n",
			port, q);
		mv_pp2x_write(hw, MVPP2_CNT_IDX_REG, q);
		mv_pp2x_print_reg(hw, MVPP2_RX_PKT_FULLQ_DROP_REG,
				  "MV_PP2_RX_PKT_FULLQ_DROP_REG");
		mv_pp2x_print_reg(hw, MVPP2_RX_PKT_EARLY_DROP_REG,
				  "MV_PP2_RX_PKT_EARLY_DROP_REG");
		mv_pp2x_print_reg(hw, MVPP2_RX_PKT_BM_DROP_REG,
				  "MV_PP2_RX_PKT_BM_DROP_REG");
	}
}
EXPORT_SYMBOL(mvPp2V1DropCntrs);

void mvPp2TxRegs(struct mv_pp2x *priv)
{
	struct mv_pp2x_hw *hw = &priv->hw;
	int i;

	DBG_MSG("\n[TX general registers]\n");

	mv_pp2x_print_reg(hw, MVPP2_TX_SNOOP_REG, "MVPP2_TX_SNOOP_REG");
	if (priv->pp2_version == PPV21) {
		mv_pp2x_print_reg(hw, MVPP21_TX_FIFO_THRESH_REG,
				  "MVPP21_TX_FIFO_THRESH_REG");
	} else {
		for (i = 0 ; i < MVPP2_MAX_PORTS; i++) {
			DBG_MSG("\n[TX port-%d registers]\n", i);
			mv_pp2x_print_reg(hw, MVPP22_TX_FIFO_THRESH_REG(i),
					  "MVPP22_TX_FIFO_THRESH_REG");
			mv_pp2x_print_reg(hw, MVPP22_TX_FIFO_SIZE_REG(i),
					  "MVPP22_TX_FIFO_SIZE_REG");
			mv_pp2x_print_reg(hw, MVPP2_TX_BAD_FCS_CNTR_REG(i),
					  "MVPP2_TX_BAD_FCS_CNTR_REG");
			mv_pp2x_print_reg(hw, MVPP2_TX_DROP_CNTR_REG(i),
					  "MVPP2_TX_DROP_CNTR_REG");
			mv_pp2x_print_reg(hw, MVPP2_TX_ETH_DSEC_THRESH_REG(i),
					  "MVPP2_TX_ETH_DSEC_THRESH_REG");
			mv_pp2x_print_reg(hw, MVPP22_TX_EGR_PIPE_DELAY_REG(i),
					  "MVPP22_TX_EGR_PIPE_DELAY_REG");
		}
	}
	mv_pp2x_print_reg(hw, MVPP2_TX_PORT_FLUSH_REG,
			  "MVPP2_TX_PORT_FLUSH_REG");
}
EXPORT_SYMBOL(mvPp2TxRegs);

void mvPp2TxSchedRegs(struct mv_pp2x *priv, int port)
{
	struct mv_pp2x_hw *hw = &priv->hw;
	struct mv_pp2x_port *pp2_port = mv_pp2x_port_struct_get(priv, port);
	int physTxp, txq;

	physTxp = mv_pp2x_egress_port(pp2_port);

	DBG_MSG("\n[TXP Scheduler registers: port=%d, physPort=%d]\n",
			port, physTxp);

	mv_pp2x_write(hw, MVPP2_TXP_SCHED_PORT_INDEX_REG, physTxp);
	mv_pp2x_print_reg(hw, MVPP2_TXP_SCHED_PORT_INDEX_REG,
			  "MV_PP2_TXP_SCHED_PORT_INDEX_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXP_SCHED_Q_CMD_REG,
			  "MV_PP2_TXP_SCHED_Q_CMD_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXP_SCHED_CMD_1_REG,
			  "MV_PP2_TXP_SCHED_CMD_1_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXP_SCHED_FIXED_PRIO_REG,
			  "MV_PP2_TXP_SCHED_FIXED_PRIO_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXP_SCHED_PERIOD_REG,
			  "MV_PP2_TXP_SCHED_PERIOD_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXP_SCHED_MTU_REG,
			  "MV_PP2_TXP_SCHED_MTU_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXP_SCHED_REFILL_REG,
			  "MV_PP2_TXP_SCHED_REFILL_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXP_SCHED_TOKEN_SIZE_REG,
			  "MV_PP2_TXP_SCHED_TOKEN_SIZE_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXP_SCHED_TOKEN_CNTR_REG,
			  "MV_PP2_TXP_SCHED_TOKEN_CNTR_REG");

	for (txq = 0; txq < MVPP2_MAX_TXQ; txq++) {
		DBG_MSG("\n[TxQ Scheduler registers: port=%d, txq=%d]\n",
			port, txq);
		mv_pp2x_print_reg(hw, MVPP2_TXQ_SCHED_REFILL_REG(txq),
				  "MV_PP2_TXQ_SCHED_REFILL_REG");
		mv_pp2x_print_reg(hw, MVPP2_TXQ_SCHED_TOKEN_SIZE_REG(txq),
				  "MV_PP2_TXQ_SCHED_TOKEN_SIZE_REG");
		mv_pp2x_print_reg(hw, MVPP2_TXQ_SCHED_TOKEN_CNTR_REG(txq),
				  "MV_PP2_TXQ_SCHED_TOKEN_CNTR_REG");
	}
}
EXPORT_SYMBOL(mvPp2TxSchedRegs);

/* Calculate period and tokens accordingly with required rate and accuracy */
int mvPp2RateCalc(int rate, unsigned int accuracy, unsigned int *pPeriod,
		  unsigned int *pTokens)
{
	/* Calculate refill tokens and period - rate [Kbps] =
	 * tokens [bits] * 1000 / period [usec]
	 */
	/* Assume:  Tclock [MHz] / BasicRefillNoOfClocks = 1
	*/
	unsigned int period, tokens, calc;

	if (rate == 0) {
		/* Disable traffic from the port: tokens = 0 */
		if (pPeriod != NULL)
			*pPeriod = 1000;

		if (pTokens != NULL)
			*pTokens = 0;

		return 0;
	}

	/* Find values of "period" and "tokens" match "rate" and
	 * "accuracy" when period is minimal
	 */
	for (period = 1; period <= 1000; period++) {
		tokens = 1;
		while (1)	{
			calc = (tokens * 1000) / period;
			if (((abs(calc - rate) * 100) / rate) <= accuracy) {
				if (pPeriod != NULL)
					*pPeriod = period;

				if (pTokens != NULL)
					*pTokens = tokens;

				return 0;
			}
			if (calc > rate)
				break;

			tokens++;
		}
	}
	return -1;
}

/* Set bandwidth limitation for TX port
 *   rate [Kbps]    - steady state TX bandwidth limitation
 */
int mvPp2TxpRateSet(struct mv_pp2x *priv, int port, int rate)
{
	u32 regVal;
	unsigned int tokens, period, txPortNum, accuracy = 0;
	int status;
	struct mv_pp2x_hw *hw = &priv->hw;
	struct mv_pp2x_port *pp2_port = mv_pp2x_port_struct_get(priv, port);

	if (port >= MVPP2_MAX_PORTS)
		return -1;

	txPortNum = mv_pp2x_egress_port(pp2_port);
	mv_pp2x_write(hw, MVPP2_TXP_SCHED_PORT_INDEX_REG, txPortNum);

	regVal = mv_pp2x_read(hw, MVPP2_TXP_SCHED_PERIOD_REG);

	status = mvPp2RateCalc(rate, accuracy, &period, &tokens);
	if (status != MV_OK) {
		DBG_MSG(
			"%s: Can't provide rate of %d [Kbps] with accuracy of %d [%%]\n",
			__func__, rate, accuracy);
		return status;
	}
	if (tokens > MVPP2_TXP_REFILL_TOKENS_MAX)
		tokens = MVPP2_TXP_REFILL_TOKENS_MAX;

	if (period > MVPP2_TXP_REFILL_PERIOD_MAX)
		period = MVPP2_TXP_REFILL_PERIOD_MAX;

	regVal = mv_pp2x_read(hw, MVPP2_TXP_SCHED_REFILL_REG);

	regVal &= ~MVPP2_TXP_REFILL_TOKENS_ALL_MASK;
	regVal |= MVPP2_TXP_REFILL_TOKENS_MASK(tokens);

	regVal &= ~MVPP2_TXP_REFILL_PERIOD_ALL_MASK;
	regVal |= MVPP2_TXP_REFILL_PERIOD_MASK(period);

	mv_pp2x_write(hw, MVPP2_TXP_SCHED_REFILL_REG, regVal);

	return 0;
}
EXPORT_SYMBOL(mvPp2TxpRateSet);

#if 0
void mvPp2IsrRegs(struct mv_pp2x_hw *hw, int port)
{
	int physPort;

	if (mvPp2PortCheck(port))
		return;

	physPort = MV_PPV2_PORT_PHYS(port);

	DBG_MSG("\n[PPv2 ISR registers: port=%d - %s]\n",
		port, MVPP2_IS_PON_PORT(port) ? "PON" : "GMAC");
	mv_pp2x_print_reg(MVPP2_ISR_RXQ_GROUP_REG(port),
		"MVPP2_ISR_RXQ_GROUP_REG");
	mv_pp2x_print_reg(MVPP2_ISR_ENABLE_REG(port),
		"MVPP2_ISR_ENABLE_REG");
	mv_pp2x_print_reg(MVPP2_ISR_RX_TX_CAUSE_REG(physPort),
		"MVPP2_ISR_RX_TX_CAUSE_REG");
	mv_pp2x_print_reg(MVPP2_ISR_RX_TX_MASK_REG(physPort),
		"MVPP2_ISR_RX_TX_MASK_REG");

	mv_pp2x_print_reg(MVPP2_ISR_RX_ERR_CAUSE_REG(physPort),
		"MVPP2_ISR_RX_ERR_CAUSE_REG");
	mv_pp2x_print_reg(MVPP2_ISR_RX_ERR_MASK_REG(physPort),
		"MVPP2_ISR_RX_ERR_MASK_REG");

	if (MVPP2_IS_PON_PORT(port)) {
		mv_pp2x_print_reg(MVPP2_ISR_PON_TX_UNDR_CAUSE_REG,
			"MVPP2_ISR_PON_TX_UNDR_CAUSE_REG");
		mv_pp2x_print_reg(MVPP2_ISR_PON_TX_UNDR_MASK_REG,
			"MVPP2_ISR_PON_TX_UNDR_MASK_REG");
	} else {
		mv_pp2x_print_reg(MVPP2_ISR_TX_ERR_CAUSE_REG(physPort),
			"MVPP2_ISR_TX_ERR_CAUSE_REG");
		mv_pp2x_print_reg(MVPP2_ISR_TX_ERR_MASK_REG(physPort),
			"MVPP2_ISR_TX_ERR_MASK_REG");
	}
	mv_pp2x_print_reg(MVPP2_ISR_MISC_CAUSE_REG, "MVPP2_ISR_MISC_CAUSE_REG");
	mv_pp2x_print_reg(MVPP2_ISR_MISC_MASK_REG, "MVPP2_ISR_MISC_MASK_REG");
}




void mvPp2AddrDecodeRegs(struct mv_pp2x_hw *hw)
{
	MV_U32 regValue;
	int win;

	/* ToDo - print Misc interrupt Cause and Mask registers */

	mv_pp2x_print_reg(ETH_BASE_ADDR_ENABLE_REG, "ETH_BASE_ADDR_ENABLE_REG");
	mv_pp2x_print_reg(ETH_TARGET_DEF_ADDR_REG, "ETH_TARGET_DEF_ADDR_REG");
	mv_pp2x_print_reg(ETH_TARGET_DEF_ID_REG, "ETH_TARGET_DEF_ID_REG");

	regValue = mv_pp2x_read(ETH_BASE_ADDR_ENABLE_REG);
	for (win = 0; win < ETH_MAX_DECODE_WIN; win++) {
		if ((regValue & (1 << win)) == 0)
			continue; /* window is disable */

		DBG_MSG("\t win[%d]\n", win);
		mv_pp2x_print_reg(ETH_WIN_BASE_REG(win), "\t ETH_WIN_BASE_REG");
		mv_pp2x_print_reg(ETH_WIN_SIZE_REG(win), "\t ETH_WIN_SIZE_REG");
		if (win < ETH_MAX_HIGH_ADDR_REMAP_WIN)
			mv_pp2x_print_reg(ETH_WIN_REMAP_REG(win),
				"\t ETH_WIN_REMAP_REG");
	}
}


void mvPp2TxSchedRegs(struct mv_pp2x_hw *hw, int port, int txp)
{
	int physTxp, txq;

	physTxp = MV_PPV2_TXP_PHYS(port, txp);

	DBG_MSG("\n[TXP Scheduler registers: port=%d, txp=%d, physPort=%d]\n",
		port, txp, physTxp);

	mv_pp2x_write(MVPP2_TXP_SCHED_PORT_INDEX_REG, physTxp);
	mv_pp2x_print_reg(MVPP2_TXP_SCHED_PORT_INDEX_REG,
		"MVPP2_TXP_SCHED_PORT_INDEX_REG");
	mv_pp2x_print_reg(MVPP2_TXP_SCHED_Q_CMD_REG,
		"MVPP2_TXP_SCHED_Q_CMD_REG");
	mv_pp2x_print_reg(MVPP2_TXP_SCHED_CMD_1_REG,
		"MVPP2_TXP_SCHED_CMD_1_REG");
	mv_pp2x_print_reg(MVPP2_TXP_SCHED_FIXED_PRIO_REG,
		"MVPP2_TXP_SCHED_FIXED_PRIO_REG");
	mv_pp2x_print_reg(MVPP2_TXP_SCHED_PERIOD_REG,
		"MVPP2_TXP_SCHED_PERIOD_REG");
	mv_pp2x_print_reg(MVPP2_TXP_SCHED_MTU_REG,
		"MVPP2_TXP_SCHED_MTU_REG");
	mv_pp2x_print_reg(MVPP2_TXP_SCHED_REFILL_REG,
		"MVPP2_TXP_SCHED_REFILL_REG");
	mv_pp2x_print_reg(MVPP2_TXP_SCHED_TOKEN_SIZE_REG,
		"MVPP2_TXP_SCHED_TOKEN_SIZE_REG");
	mv_pp2x_print_reg(MVPP2_TXP_SCHED_TOKEN_CNTR_REG,
		"MVPP2_TXP_SCHED_TOKEN_CNTR_REG");

	for (txq = 0; txq < MVPP2_MAX_TXQ; txq++) {
		DBG_MSG(
			"\n[TxQ Scheduler registers: port=%d, txp=%d, txq=%d]\n",
			port, txp, txq);
		mv_pp2x_print_reg(MVPP2_TXQ_SCHED_REFILL_REG(txq),
			"MVPP2_TXQ_SCHED_REFILL_REG");
		mv_pp2x_print_reg(MVPP2_TXQ_SCHED_TOKEN_SIZE_REG(txq),
			"MVPP2_TXQ_SCHED_TOKEN_SIZE_REG");
		mv_pp2x_print_reg(MVPP2_TXQ_SCHED_TOKEN_CNTR_REG(txq),
			"MVPP2_TXQ_SCHED_TOKEN_CNTR_REG");
	}
}

void      mvPp2FwdSwitchRegs(struct mv_pp2x_hw *hw)
{
	DBG_MSG("\n[FWD Switch registers]\n");

	mv_pp2x_print_reg(MVPP2_FWD_SWITCH_FLOW_ID_REG,
		"MVPP2_FWD_SWITCH_FLOW_ID_REG");
	mv_pp2x_print_reg(MVPP2_FWD_SWITCH_CTRL_REG,
		"MVPP2_FWD_SWITCH_CTRL_REG");
	mv_pp2x_print_reg(MVPP2_FWD_SWITCH_STATUS_REG,
		"MVPP2_FWD_SWITCH_STATUS_REG");
}


void mvPp2BmPoolRegs(struct mv_pp2x_hw *hw, int pool)
{
	if (mvPp2MaxCheck(pool, MV_BM_POOLS, "bm_pool"))
		return;

	DBG_MSG("\n[BM pool registers: pool=%d]\n", pool);
	mv_pp2x_print_reg(MV_BM_POOL_BASE_REG(pool),
		"MV_BM_POOL_BASE_REG");
	mv_pp2x_print_reg(MV_BM_POOL_SIZE_REG(pool),
		"MV_BM_POOL_SIZE_REG");
	mv_pp2x_print_reg(MV_BM_POOL_READ_PTR_REG(pool),
		"MV_BM_POOL_READ_PTR_REG");
	mv_pp2x_print_reg(MV_BM_POOL_PTRS_NUM_REG(pool),
		"MV_BM_POOL_PTRS_NUM_REG");
	mv_pp2x_print_reg(MV_BM_BPPI_READ_PTR_REG(pool),
		"MV_BM_BPPI_READ_PTR_REG");
	mv_pp2x_print_reg(MV_BM_BPPI_PTRS_NUM_REG(pool),
		"MV_BM_BPPI_PTRS_NUM_REG");
	mv_pp2x_print_reg(MV_BM_POOL_CTRL_REG(pool),
		"MV_BM_POOL_CTRL_REG");
	mv_pp2x_print_reg(MV_BM_INTR_CAUSE_REG(pool),
		"MV_BM_INTR_CAUSE_REG");
	mv_pp2x_print_reg(MV_BM_INTR_MASK_REG(pool),
		"MV_BM_INTR_MASK_REG");
}

void mvPp2V0DropCntrs(struct mv_pp2x_hw *hw, int port)
{
	int i;

	DBG_MSG("\n[Port #%d Drop counters]\n", port);
	mv_pp2x_print_reg(MVPP2_OVERRUN_DROP_REG(MV_PPV2_PORT_PHYS(port)),
		"MVPP2_OVERRUN_DROP_REG");
	mv_pp2x_print_reg(MVPP2_CLS_DROP_REG(MV_PPV2_PORT_PHYS(port)),
		"MVPP2_CLS_DROP_REG");

	if (MVPP2_IS_PON_PORT(port)) {
		for (i = 0; i < mvPp2HalData.maxTcont; i++) {
			mv_pp2x_print_reg2(MVPP2_V0_TX_EARLY_DROP_REG(i),
				"MVPP2_TX_EARLY_DROP_REG", i);
			mv_pp2x_print_reg2(MVPP2_V0_TX_DESC_DROP_REG(i),
				"MVPP2_TX_DESC_DROP_REG", i);
		}
	} else {
		i = MVPP2_MAX_TCONT + port;
		mv_pp2x_print_reg2(MVPP2_V0_TX_EARLY_DROP_REG(i),
			"MVPP2_TX_EARLY_DROP_REG", i);
		mv_pp2x_print_reg2(MVPP2_V0_TX_DESC_DROP_REG(i),
			"MVPP2_TX_DESC_DROP_REG", i);
	}
	for (i = port * CONFIG_MVPP2_RXQ; i < (port * CONFIG_MVPP2_RXQ +
			CONFIG_MVPP2_RXQ); i++) {
		mv_pp2x_print_reg2(MVPP2_V0_RX_EARLY_DROP_REG(i),
			"MVPP2_RX_EARLY_DROP_REG", i);
		mv_pp2x_print_reg2(MVPP2_V0_RX_DESC_DROP_REG(i),
			"MVPP2_RX_DESC_DROP_REG", i);
	}
}

void mvPp2V1DropCntrs(struct mv_pp2x_hw *hw, int port)
{
	int txp, phyRxq, q;
	MVPP2_PORT_CTRL *pPortCtrl = mvPp2PortHndlGet(port);
	int physPort = MV_PPV2_PORT_PHYS(port);


	DBG_MSG("\n[global drop counters]\n");
	mvPp2RegPrintNonZero(MVPP2_V1_OVERFLOW_MC_DROP_REG,
		"MVPP2_OVERRUN_DROP_REG");

	DBG_MSG("\n[Port #%d Drop counters]\n", port);
	mvPp2RegPrintNonZero(MVPP2_OVERRUN_DROP_REG(physPort),
		"MVPP2_OVERRUN_DROP_REG");
	mvPp2RegPrintNonZero(MVPP2_CLS_DROP_REG(physPort),
		"MVPP2_CLS_DROP_REG");

	for (txp = 0; txp < pPortCtrl->txpNum; txp++) {
		for (q = 0; q < MVPP2_MAX_TXQ; q++) {
			DBG_MSG(
				"\n------ [Port #%d txp #%d txq #%d counters] -----\n",
				port, txp, q);
			mv_pp2x_write(MVPP2_V1_CNT_IDX_REG, TX_CNT_IDX(port,
				txp, q));
			mvPp2RegPrintNonZero(MVPP2_V1_TX_PKT_FULLQ_DROP_REG,
				"MVPP2_V1_TX_PKT_FULLQ_DROP_REG");
			mvPp2RegPrintNonZero(MVPP2_V1_TX_PKT_EARLY_DROP_REG,
				"MVPP2_V1_TX_PKT_EARLY_DROP_REG");
			mvPp2RegPrintNonZero(MVPP2_V1_TX_PKT_BM_DROP_REG,
				"MVPP2_V1_TX_PKT_BM_DROP_REG");
			mvPp2RegPrintNonZero(MVPP2_V1_TX_PKT_BM_MC_DROP_REG,
				"MVPP2_V1_TX_PKT_BM_MC_DROP_REG");
		}
	}

	for (q = 0; q < CONFIG_MVPP2_RXQ; q++) {
		DBG_MSG("\n------ [Port #%d, rxq #%d counters] -----\n",
			port, q);
		phyRxq = mvPp2LogicRxqToPhysRxq(port, q);
		mv_pp2x_write(MVPP2_V1_CNT_IDX_REG, phyRxq);
		mvPp2RegPrintNonZero(MVPP2_V1_RX_PKT_FULLQ_DROP_REG,
			"MVPP2_V1_RX_PKT_FULLQ_DROP_REG");
		mvPp2RegPrintNonZero(MVPP2_V1_RX_PKT_EARLY_DROP_REG,
			"MVPP2_V1_RX_PKT_EARLY_DROP_REG");
		mvPp2RegPrintNonZero(MVPP2_V1_RX_PKT_BM_DROP_REG,
			"MVPP2_V1_RX_PKT_BM_DROP_REG");
	}
}

#endif

void mvPp2V1RxqDbgCntrs(struct mv_pp2x *priv, int port, int rxq)
{
	struct mv_pp2x_port *pp_port;
	int phy_rxq;
	struct mv_pp2x_hw *hw = &priv->hw;

	pp_port = mv_pp2x_port_struct_get(priv, port);
	if (pp_port)
		phy_rxq = pp_port->first_rxq + rxq;
	else
		return;

	DBG_MSG("\n------ [Port #%d, rxq #%d counters] -----\n", port, rxq);
	mv_pp2x_write(hw, MVPP2_CNT_IDX_REG, phy_rxq);
	mv_pp2x_print_reg(hw, MVPP2_RX_PKT_FULLQ_DROP_REG,
			  "MV_PP2_RX_PKT_FULLQ_DROP_REG");
	mv_pp2x_print_reg(hw, MVPP2_RX_PKT_EARLY_DROP_REG,
			  "MVPP2_V1_RX_PKT_EARLY_DROP_REG");
	mv_pp2x_print_reg(hw, MVPP2_RX_PKT_BM_DROP_REG,
			  "MVPP2_V1_RX_PKT_BM_DROP_REG");
	mv_pp2x_print_reg(hw, MVPP2_RX_DESC_ENQ_REG,
			  "MVPP2_V1_RX_DESC_ENQ_REG");
}
EXPORT_SYMBOL(mvPp2V1RxqDbgCntrs);

void mvPp2RxFifoRegs(struct mv_pp2x_hw *hw, int port)
{
	DBG_MSG("\n[Port #%d RX Fifo]\n", port);
	mv_pp2x_print_reg(hw, MVPP2_RX_DATA_FIFO_SIZE_REG(port),
			  "MVPP2_RX_DATA_FIFO_SIZE_REG");
	mv_pp2x_print_reg(hw, MVPP2_RX_ATTR_FIFO_SIZE_REG(port),
			  "MVPP2_RX_ATTR_FIFO_SIZE_REG");
	DBG_MSG("\n[Global RX Fifo regs]\n");
	mv_pp2x_print_reg(hw, MVPP2_RX_MIN_PKT_SIZE_REG,
			  "MVPP2_RX_MIN_PKT_SIZE_REG");
}
EXPORT_SYMBOL(mvPp2RxFifoRegs);

#if 0
/* Print status of Ethernet port */
void mvPp2PortStatus(struct mv_pp2x_hw *hw, int port)
{
	int i, txp, txq;
	MV_ETH_PORT_STATUS	link;
	MVPP2_PORT_CTRL		*pPortCtrl;

	if (mvPp2PortCheck(port))
		return;

	pPortCtrl = mvPp2PortHndlGet(port);
	if (!pPortCtrl)
		return;

	DBG_MSG("\n[RXQ mapping: port=%d, ctrl=%p]\n", port, pPortCtrl);
	if (pPortCtrl->pRxQueue) {
		DBG_MSG("         RXQ: ");
		for (i = 0; i < pPortCtrl->rxqNum; i++)
			DBG_MSG(" %4d", i);

		DBG_MSG("\nphysical RXQ: ");
		for (i = 0; i < pPortCtrl->rxqNum; i++) {
			if (pPortCtrl->pRxQueue[i])
				DBG_MSG(" %4d", pPortCtrl->pRxQueue[i]->rxq);
			else
				DBG_MSG(" NULL");
		}
		DBG_MSG("\n");
	}

	DBG_MSG("\n[BM queue to Qset mapping]\n");
	if (pPortCtrl->pRxQueue) {
		DBG_MSG("       RXQ: ");
		for (i = 0; i < pPortCtrl->rxqNum; i++)
			DBG_MSG(" %4d", i);

		DBG_MSG("\n long Qset: ");
		for (i = 0; i < pPortCtrl->rxqNum; i++)
			DBG_MSG(" %4d",
				mvBmRxqToQsetLongGet(
					mvPp2LogicRxqToPhysRxq(port, i)));

		DBG_MSG("\nshort Qset: ");
		for (i = 0; i < pPortCtrl->rxqNum; i++)
			DBG_MSG(" %4d",
				mvBmRxqToQsetShortGet(
					mvPp2LogicRxqToPhysRxq(port, i)));

		DBG_MSG("\n");
	}
	if (pPortCtrl->pTxQueue) {
		for (txp = 0; txp < pPortCtrl->txpNum; txp++) {
			DBG_MSG("\nTXP %2d, TXQ:", txp);
			for (txq = 0; txq < pPortCtrl->txqNum; txq++)
				DBG_MSG(" %4d", txq);

			DBG_MSG("\n long Qset: ");
			for (txq = 0; txq < pPortCtrl->txqNum; txq++)
				DBG_MSG(" %4d",
					mvBmTxqToQsetLongGet(
					MV_PPV2_TXQ_PHYS(port, txp, txq)));

			DBG_MSG("\nshort Qset: ");
			for (txq = 0; txq < pPortCtrl->txqNum; txq++)
				DBG_MSG(" %4d",
					mvBmTxqToQsetShortGet(
					MV_PPV2_TXQ_PHYS(port, txp, txq)));

			DBG_MSG("\n");
		}
	}

	DBG_MSG("\n[Link: port=%d, ctrl=%p]\n", port, pPortCtrl);

	if (!MVPP2_IS_PON_PORT(port)) {

		mvGmacLinkStatus(port, &link);

		if (link.linkup) {
			DBG_MSG("link up");
			DBG_MSG(", %s duplex",
				(link.duplex == MV_ETH_DUPLEX_FULL) ?
					"full" : "half");
			DBG_MSG(", speed ");

			if (link.speed == MV_ETH_SPEED_1000)
				DBG_MSG("1 Gbps\n");
			else if (link.speed == MV_ETH_SPEED_100)
				DBG_MSG("100 Mbps\n");
			else
				DBG_MSG("10 Mbps\n");

			DBG_MSG("rxFC - %s, txFC - %s\n",
				(link.rxFc == MV_ETH_FC_DISABLE) ?
					"disabled" : "enabled",
				(link.txFc == MV_ETH_FC_DISABLE) ?
					"disabled" : "enabled");
		} else
			DBG_MSG("link down\n");
	}
}
#endif
static char *mv_pp2x_prs_l2_info_str(unsigned int l2_info)
{
	switch (l2_info << MVPP2_PRS_RI_L2_CAST_OFFS) {
	case MVPP2_PRS_RI_L2_UCAST:
		return "Ucast";
	case MVPP2_PRS_RI_L2_MCAST:
		return "Mcast";
	case MVPP2_PRS_RI_L2_BCAST:
		return "Bcast";
	default:
		return "Unknown";
	}
	return NULL;
}

static char *mv_pp2x_prs_vlan_info_str(unsigned int vlan_info)
{
	switch (vlan_info << MVPP2_PRS_RI_VLAN_OFFS) {
	case MVPP2_PRS_RI_VLAN_NONE:
		return "None";
	case MVPP2_PRS_RI_VLAN_SINGLE:
		return "Single";
	case MVPP2_PRS_RI_VLAN_DOUBLE:
		return "Double";
	case MVPP2_PRS_RI_VLAN_TRIPLE:
		return "Triple";
	default:
		return "Unknown";
	}
	return NULL;
}

void mv_pp2x_rx_desc_print(struct mv_pp2x *priv, struct mv_pp2x_rx_desc *desc)
{
	int i;
	u32 *words = (u32 *) desc;

	DBG_MSG("RX desc - %p: ", desc);
	for (i = 0; i < 8; i++)
		DBG_MSG("%8.8x ", *words++);
	DBG_MSG("\n");

	DBG_MSG("pkt_size=%d, L3_offs=%d, IP_hlen=%d, ",
	       desc->data_size,
	       (desc->status & MVPP2_RXD_L3_OFFSET_MASK) >>
			MVPP2_RXD_L3_OFFSET_OFFS,
	       (desc->status & MVPP2_RXD_IP_HLEN_MASK) >>
			MVPP2_RXD_IP_HLEN_OFFS);

	DBG_MSG("L2=%s, ",
		mv_pp2x_prs_l2_info_str((desc->rsrvd_parser &
			MVPP2_RXD_L2_CAST_MASK) >> MVPP2_RXD_L2_CAST_OFFS));

	DBG_MSG("VLAN=");
	DBG_MSG("%s, ",
		mv_pp2x_prs_vlan_info_str((desc->rsrvd_parser &
			MVPP2_RXD_VLAN_INFO_MASK) >> MVPP2_RXD_VLAN_INFO_OFFS));

	DBG_MSG("L3=");
	if (MVPP2_RXD_L3_IS_IP4(desc->status))
		DBG_MSG("IPv4 (hdr=%s), ",
			MVPP2_RXD_IP4_HDR_ERR(desc->status) ? "bad" : "ok");
	else if (MVPP2_RXD_L3_IS_IP4_OPT(desc->status))
		DBG_MSG("IPv4 Options (hdr=%s), ",
			MVPP2_RXD_IP4_HDR_ERR(desc->status) ? "bad" : "ok");
	else if (MVPP2_RXD_L3_IS_IP4_OTHER(desc->status))
		DBG_MSG("IPv4 Other (hdr=%s), ",
			MVPP2_RXD_IP4_HDR_ERR(desc->status) ? "bad" : "ok");
	else if (MVPP2_RXD_L3_IS_IP6(desc->status))
		DBG_MSG("IPv6, ");
	else if (MVPP2_RXD_L3_IS_IP6_EXT(desc->status))
		DBG_MSG("IPv6 Ext, ");
	else
		DBG_MSG("Unknown, ");

	if (desc->status & MVPP2_RXD_IP_FRAG_MASK)
		DBG_MSG("Frag, ");

	DBG_MSG("L4=");
	if (MVPP2_RXD_L4_IS_TCP(desc->status))
		DBG_MSG("TCP (csum=%s)", (desc->status &
			MVPP2_RXD_L4_CHK_OK_MASK) ? "Ok" : "Bad");
	else if (MVPP2_RXD_L4_IS_UDP(desc->status))
		DBG_MSG("UDP (csum=%s)", (desc->status &
			MVPP2_RXD_L4_CHK_OK_MASK) ? "Ok" : "Bad");
	else
		DBG_MSG("Unknown");

	DBG_MSG("\n");

	DBG_MSG("Lookup_ID=0x%x, cpu_code=0x%x\n",
		(desc->rsrvd_parser &
			MVPP2_RXD_LKP_ID_MASK) >> MVPP2_RXD_LKP_ID_OFFS,
		(desc->rsrvd_parser &
			MVPP2_RXD_CPU_CODE_MASK) >> MVPP2_RXD_CPU_CODE_OFFS);

	if (priv->pp2_version == PPV22) {
		DBG_MSG("buf_phys_addr = 0x%llx\n",
			desc->u.pp22.buf_phys_addr_key_hash &
			DMA_BIT_MASK(40));
		DBG_MSG("buf_virt_addr = 0x%llx\n",
			desc->u.pp22.buf_cookie_bm_qset_cls_info &
			DMA_BIT_MASK(40));
	}
}

static void mv_pp2x_bm_queue_map_dump(struct mv_pp2x_hw *hw, int queue)
{
	unsigned int regVal, shortQset, longQset;

	DBG_MSG("-------- queue #%d --------\n", queue);

	mv_pp2x_write(hw, MVPP2_BM_PRIO_IDX_REG, queue);
	regVal = mv_pp2x_read(hw, MVPP2_BM_CPU_QSET_REG);

	shortQset = ((regVal & (MVPP2_BM_CPU_SHORT_QSET_MASK)) >>
		    MVPP2_BM_CPU_SHORT_QSET_OFFS);
	longQset = ((regVal & (MVPP2_BM_CPU_LONG_QSET_MASK)) >>
		    MVPP2_BM_CPU_LONG_QSET_OFFS);
	DBG_MSG("CPU SHORT QSET = 0x%02x\n", shortQset);
	DBG_MSG("CPU LONG QSET  = 0x%02x\n", longQset);

	regVal = mv_pp2x_read(hw, MVPP2_BM_HWF_QSET_REG);
	shortQset = ((regVal & (MVPP2_BM_HWF_SHORT_QSET_MASK)) >>
		    MVPP2_BM_HWF_SHORT_QSET_OFFS);
	longQset = ((regVal & (MVPP2_BM_HWF_LONG_QSET_MASK)) >>
		    MVPP2_BM_HWF_LONG_QSET_OFFS);
	DBG_MSG("HWF SHORT QSET = 0x%02x\n", shortQset);
	DBG_MSG("HWF LONG QSET  = 0x%02x\n", longQset);
}

static bool mv_pp2x_bm_priority_en(struct mv_pp2x_hw *hw)
{
	return ((mv_pp2x_read(hw, MVPP2_BM_PRIO_CTRL_REG) == 0) ? false : true);
}

void mv_pp2x_bm_queue_map_dump_all(struct mv_pp2x_hw *hw)
{
	int queue;

	if (!mv_pp2x_bm_priority_en(hw))
		DBG_MSG("Note: The buffers priority algorithms is disabled.\n");

	for (queue = 0; queue <= MVPP2_BM_PRIO_IDX_MAX; queue++)
		mv_pp2x_bm_queue_map_dump(hw, queue);
}
EXPORT_SYMBOL(mv_pp2x_bm_queue_map_dump_all);

static int mv_pp2x_prs_hw_tcam_cnt_dump(struct mv_pp2x_hw *hw,
					int tid, unsigned int *cnt)
{
	unsigned int regVal;

	if (mv_pp2x_range_validate(tid, 0,
	    MVPP2_PRS_TCAM_SRAM_SIZE - 1) == MV_ERROR)
		return MV_ERROR;

	/* write index */
	mv_pp2x_write(hw, MVPP2_PRS_TCAM_HIT_IDX_REG, tid);

	regVal = mv_pp2x_read(hw, MVPP2_PRS_TCAM_HIT_CNT_REG);
	regVal &= MVPP2_PRS_TCAM_HIT_CNT_MASK;

	if (cnt)
		*cnt = regVal;
	else
		DBG_MSG("HIT COUNTER: %d\n", regVal);

	return MV_OK;
}

static int mv_pp2x_prs_sw_sram_ri_dump(struct mv_pp2x_prs_entry *pe)
{
	unsigned int data, mask;
	int i, bitsOffs = 0;
	char bits[100];

	if (mv_pp2x_ptr_validate(pe) == MV_ERROR)
		return MV_ERROR;

	mv_pp2x_prs_sw_sram_ri_get(pe, &data, &mask);
	if (mask == 0)
		return 0;

	DBG_MSG("\n       ");

	DBG_MSG("S_RI=");
	for (i = (MVPP2_PRS_SRAM_RI_CTRL_BITS-1); i > -1 ; i--)
		if (mask & (1 << i)) {
			DBG_MSG("%d", ((data & (1 << i)) != 0));
			bitsOffs += sprintf(bits + bitsOffs, "%d:", i);
		} else
			DBG_MSG("x");

	bits[bitsOffs] = '\0';
	DBG_MSG(" %s", bits);

	return 0;
}

static int mv_pp2x_prs_sw_sram_ai_dump(struct mv_pp2x_prs_entry *pe)
{
	int i, bitsOffs = 0;
	unsigned int data, mask;
	char bits[30];

	if (mv_pp2x_ptr_validate(pe) == MV_ERROR)
		return MV_ERROR;

	mv_pp2x_prs_sw_sram_ai_get(pe, &data, &mask);

	if (mask == 0)
		return 0;

	DBG_MSG("\n       ");

	DBG_MSG("S_AI=");
	for (i = (MVPP2_PRS_SRAM_AI_CTRL_BITS-1); i > -1 ; i--)
		if (mask & (1 << i)) {
			DBG_MSG("%d", ((data & (1 << i)) != 0));
			bitsOffs += sprintf(bits + bitsOffs, "%d:", i);
		} else
			DBG_MSG("x");
	bits[bitsOffs] = '\0';
	DBG_MSG(" %s", bits);
	return 0;
}

int mv_pp2x_prs_sw_dump(struct mv_pp2x_prs_entry *pe)
{
	u32 op, type, lu, done, flowid;
	int	shift, offset, i;

	if (mv_pp2x_ptr_validate(pe) == MV_ERROR)
		return MV_ERROR;

	/* hw entry id */
	DBG_MSG("[%4d] ", pe->index);

	i = MVPP2_PRS_TCAM_WORDS - 1;
	DBG_MSG("%1.1x ", pe->tcam.word[i--] & 0xF);

	while (i >= 0)
		DBG_MSG("%4.4x ", (pe->tcam.word[i--]) & 0xFFFF);

	DBG_MSG("| ");

	/*DBG_MSG(PRS_SRAM_FMT, PRS_SRAM_VAL(pe->sram.word)); */
	DBG_MSG("%4.4x %8.8x %8.8x %8.8x", pe->sram.word[3] & 0xFFFF,
		 pe->sram.word[2],  pe->sram.word[1],  pe->sram.word[0]);

	DBG_MSG("\n       ");

	i = MVPP2_PRS_TCAM_WORDS - 1;
	DBG_MSG("%1.1x ", (pe->tcam.word[i--] >> 16) & 0xF);

	while (i >= 0)
		DBG_MSG("%4.4x ", ((pe->tcam.word[i--]) >> 16)  & 0xFFFF);

	DBG_MSG("| ");

	mv_pp2x_prs_sw_sram_shift_get(pe, &shift);
	DBG_MSG("SH=%d ", shift);

	mv_pp2x_prs_sw_sram_offset_get(pe, &type, &offset, &op);
	if (offset != 0 || ((op >> MVPP2_PRS_SRAM_OP_SEL_SHIFT_BITS) != 0))
		DBG_MSG("UDFT=%u UDFO=%d ", type, offset);

	DBG_MSG("op=%u ", op);

	mv_pp2x_prs_sw_sram_next_lu_get(pe, &lu);
	DBG_MSG("LU=%u ", lu);

	mv_pp2x_prs_sw_sram_lu_done_get(pe, &done);
	DBG_MSG("%s ", done ? "DONE" : "N_DONE");

	/*flow id generation bit*/
	mv_pp2x_prs_sw_sram_flowid_gen_get(pe, &flowid);
	DBG_MSG("%s ", flowid ? "FIDG" : "N_FIDG");

	if ((pe->tcam.word[MVPP2_PRS_TCAM_INV_WORD] & MVPP2_PRS_TCAM_INV_MASK))
		DBG_MSG(" [inv]");

	if (mv_pp2x_prs_sw_sram_ri_dump(pe))
		return MV_ERROR;

	if (mv_pp2x_prs_sw_sram_ai_dump(pe))
		return MV_ERROR;

	DBG_MSG("\n");

	return 0;

}
EXPORT_SYMBOL(mv_pp2x_prs_sw_dump);

int mv_pp2x_prs_hw_dump(struct mv_pp2x_hw *hw)
{
	int index;
	struct mv_pp2x_prs_entry pe;


	DBG_MSG("%s\n", __func__);

	for (index = 0; index < MVPP2_PRS_TCAM_SRAM_SIZE; index++) {
		pe.index = index;
		mv_pp2x_prs_hw_read(hw, &pe);
		if ((pe.tcam.word[MVPP2_PRS_TCAM_INV_WORD] &
			MVPP2_PRS_TCAM_INV_MASK) ==
			MVPP2_PRS_TCAM_ENTRY_VALID) {
			mv_pp2x_prs_sw_dump(&pe);
			mv_pp2x_prs_hw_tcam_cnt_dump(hw, index, NULL);
			DBG_MSG("-----------------------------------------\n");
		}
	}

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_prs_hw_dump);

int mv_pp2x_prs_hw_regs_dump(struct mv_pp2x_hw *hw)
{
	int i;
	char reg_name[100];

	mv_pp2x_print_reg(hw, MVPP2_PRS_INIT_LOOKUP_REG,
			  "MVPP2_PRS_INIT_LOOKUP_REG");
	mv_pp2x_print_reg(hw, MVPP2_PRS_INIT_OFFS_REG(0),
			  "MVPP2_PRS_INIT_OFFS_0_3_REG");
	mv_pp2x_print_reg(hw, MVPP2_PRS_INIT_OFFS_REG(4),
			  "MVPP2_PRS_INIT_OFFS_4_7_REG");
	mv_pp2x_print_reg(hw, MVPP2_PRS_MAX_LOOP_REG(0),
			  "MVPP2_PRS_MAX_LOOP_0_3_REG");
	mv_pp2x_print_reg(hw, MVPP2_PRS_MAX_LOOP_REG(4),
			  "MVPP2_PRS_MAX_LOOP_4_7_REG");

	/*mv_pp2x_print_reg(hw, MVPP2_PRS_INTR_CAUSE_REG,
	 *		     "MVPP2_PRS_INTR_CAUSE_REG");
	 */
	/*mv_pp2x_print_reg(hw, MVPP2_PRS_INTR_MASK_REG,
	 *		     "MVPP2_PRS_INTR_MASK_REG");
	 */
	mv_pp2x_print_reg(hw, MVPP2_PRS_TCAM_IDX_REG,
			  "MVPP2_PRS_TCAM_IDX_REG");

	for (i = 0; i < MVPP2_PRS_TCAM_WORDS; i++) {
		sprintf(reg_name, "MVPP2_PRS_TCAM_DATA_%d_REG", i);
		mv_pp2x_print_reg(hw, MVPP2_PRS_TCAM_DATA_REG(i),
			reg_name);
	}
	mv_pp2x_print_reg(hw, MVPP2_PRS_SRAM_IDX_REG,
			  "MVPP2_PRS_SRAM_IDX_REG");

	for (i = 0; i < MVPP2_PRS_SRAM_WORDS; i++) {
		sprintf(reg_name, "MVPP2_PRS_SRAM_DATA_%d_REG", i);
		mv_pp2x_print_reg(hw, MVPP2_PRS_SRAM_DATA_REG(i),
			reg_name);
	}

	mv_pp2x_print_reg(hw, MVPP2_PRS_EXP_REG,
			  "MVPP2_PRS_EXP_REG");
	mv_pp2x_print_reg(hw, MVPP2_PRS_TCAM_CTRL_REG,
			  "MVPP2_PRS_TCAM_CTRL_REG");

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_prs_hw_regs_dump);


int mv_pp2x_prs_hw_hits_dump(struct mv_pp2x_hw *hw)
{
	int index;
	unsigned int cnt;
	struct mv_pp2x_prs_entry pe;

	for (index = 0; index < MVPP2_PRS_TCAM_SRAM_SIZE; index++) {
		pe.index = index;
		mv_pp2x_prs_hw_read(hw, &pe);
		if ((pe.tcam.word[MVPP2_PRS_TCAM_INV_WORD] &
			MVPP2_PRS_TCAM_INV_MASK) ==
			MVPP2_PRS_TCAM_ENTRY_VALID) {
			mv_pp2x_prs_hw_tcam_cnt_dump(hw, index, &cnt);
			if (cnt == 0)
				continue;
			mv_pp2x_prs_sw_dump(&pe);
			DBG_MSG("INDEX: %d       HITS: %d\n", index, cnt);
			DBG_MSG("-----------------------------------------\n");
		}
	}
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_prs_hw_hits_dump);

int mvPp2ClsC2QosSwDump(struct mv_pp2x_cls_c2_qos_entry *qos)
{
	int int32bit;
	int status = 0;

	if (mv_pp2x_ptr_validate(qos) == MV_ERROR)
		return MV_ERROR;

	DBG_MSG(
	"TABLE	SEL	LINE	PRI	DSCP	COLOR	GEM_ID	QUEUE\n");

	/* table id */
	DBG_MSG("0x%2.2x\t", qos->tbl_id);

	/* table sel */
	DBG_MSG("0x%1.1x\t", qos->tbl_sel);

	/* table line */
	DBG_MSG("0x%2.2x\t", qos->tbl_line);

	/* priority */
	status |= mv_pp2_cls_c2_qos_prio_get(qos, &int32bit);
	DBG_MSG("0x%1.1x\t", int32bit);

	/* dscp */
	status |= mv_pp2_cls_c2_qos_dscp_get(qos, &int32bit);
	DBG_MSG("0x%2.2x\t", int32bit);

	/* color */
	status |= mv_pp2_cls_c2_qos_color_get(qos, &int32bit);
	DBG_MSG("0x%1.1x\t", int32bit);

	/* gem port id */
	status |= mv_pp2_cls_c2_qos_gpid_get(qos, &int32bit);
	DBG_MSG("0x%3.3x\t", int32bit);

	/* queue */
	status |= mv_pp2_cls_c2_qos_queue_get(qos, &int32bit);
	DBG_MSG("0x%2.2x", int32bit);

	DBG_MSG("\n");

	return status;
}
/*----------------------------------------------------------------------*/

int mv_pp2x_cls_c2_qos_dscp_hw_dump(struct mv_pp2x_hw *hw)
{
	int tbl_id, tbl_line, int32bit;
	struct mv_pp2x_cls_c2_qos_entry qos;

	for (tbl_id = 0; tbl_id < MVPP2_CLS_C2_QOS_DSCP_TBL_NUM; tbl_id++) {

		DBG_MSG("\n------------ DSCP TABLE %d ------------\n", tbl_id);
		DBG_MSG("LINE	DSCP	COLOR	GEM_ID	QUEUE\n");
		for (tbl_line = 0; tbl_line < MVPP2_CLS_C2_QOS_DSCP_TBL_SIZE;
				tbl_line++) {
			mv_pp2x_cls_c2_qos_hw_read(hw, tbl_id,
				1/*DSCP*/, tbl_line, &qos);
			DBG_MSG("0x%2.2x\t", qos.tbl_line);
			mv_pp2_cls_c2_qos_dscp_get(&qos, &int32bit);
			DBG_MSG("0x%2.2x\t", int32bit);
			mv_pp2_cls_c2_qos_color_get(&qos, &int32bit);
			DBG_MSG("0x%1.1x\t", int32bit);
			mv_pp2_cls_c2_qos_gpid_get(&qos, &int32bit);
			DBG_MSG("0x%3.3x\t", int32bit);
			mv_pp2_cls_c2_qos_queue_get(&qos, &int32bit);
			DBG_MSG("0x%2.2x", int32bit);
			DBG_MSG("\n");
		}
	}
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_qos_dscp_hw_dump);

/*----------------------------------------------------------------------*/

int mv_pp2x_cls_c2_qos_prio_hw_dump(struct mv_pp2x_hw *hw)
{
	int tbl_id, tbl_line, int32bit;

	struct mv_pp2x_cls_c2_qos_entry qos;

	for (tbl_id = 0; tbl_id < MVPP2_CLS_C2_QOS_PRIO_TBL_NUM; tbl_id++) {

		DBG_MSG("\n-------- PRIORITY TABLE %d -----------\n", tbl_id);
		DBG_MSG("LINE	PRIO	COLOR	GEM_ID	QUEUE\n");

		for (tbl_line = 0; tbl_line < MVPP2_CLS_C2_QOS_PRIO_TBL_SIZE;
				tbl_line++) {
			mv_pp2x_cls_c2_qos_hw_read(hw, tbl_id,
				0/*PRIO*/, tbl_line, &qos);
			DBG_MSG("0x%2.2x\t", qos.tbl_line);
			mv_pp2_cls_c2_qos_prio_get(&qos, &int32bit);
			DBG_MSG("0x%1.1x\t", int32bit);
			mv_pp2_cls_c2_qos_color_get(&qos, &int32bit);
			DBG_MSG("0x%1.1x\t", int32bit);
			mv_pp2_cls_c2_qos_gpid_get(&qos, &int32bit);
			DBG_MSG("0x%3.3x\t", int32bit);
			mv_pp2_cls_c2_qos_queue_get(&qos, &int32bit);
			DBG_MSG("0x%2.2x", int32bit);
			DBG_MSG("\n");
		}
	}
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_qos_prio_hw_dump);

/*----------------------------------------------------------------------*/

int mv_pp2x_cls_c2_sw_dump(struct mv_pp2x_cls_c2_entry *c2)
{
	int id, sel, type, gemid, low_q, high_q, color, int32bit;

	if (mv_pp2x_ptr_validate(c2) == MV_ERROR)
		return MV_ERROR;

	mv_pp2x_cls_c2_sw_words_dump(c2);
	DBG_MSG("\n");

	/*------------------------------*/
	/*	action_tbl 0x1B30	*/
	/*------------------------------*/

	id = ((c2->sram.regs.action_tbl &
	      (MVPP2_CLS2_ACT_DATA_TBL_ID_MASK)) >>
	       MVPP2_CLS2_ACT_DATA_TBL_ID_OFF);
	sel = ((c2->sram.regs.action_tbl &
	       (MVPP2_CLS2_ACT_DATA_TBL_SEL_MASK)) >>
		MVPP2_CLS2_ACT_DATA_TBL_SEL_OFF);
	type = ((c2->sram.regs.action_tbl &
	       (MVPP2_CLS2_ACT_DATA_TBL_PRI_DSCP_MASK)) >>
		MVPP2_CLS2_ACT_DATA_TBL_PRI_DSCP_OFF);
	gemid = ((c2->sram.regs.action_tbl &
		 (MVPP2_CLS2_ACT_DATA_TBL_GEM_ID_MASK)) >>
		  MVPP2_CLS2_ACT_DATA_TBL_GEM_ID_OFF);
	low_q = ((c2->sram.regs.action_tbl &
		 (MVPP2_CLS2_ACT_DATA_TBL_LOW_Q_MASK)) >>
		  MVPP2_CLS2_ACT_DATA_TBL_LOW_Q_OFF);
	high_q = ((c2->sram.regs.action_tbl &
		  (MVPP2_CLS2_ACT_DATA_TBL_HIGH_Q_MASK)) >>
		   MVPP2_CLS2_ACT_DATA_TBL_HIGH_Q_OFF);
	color = ((c2->sram.regs.action_tbl &
		 (MVPP2_CLS2_ACT_DATA_TBL_COLOR_MASK)) >>
		  MVPP2_CLS2_ACT_DATA_TBL_COLOR_OFF);

	DBG_MSG("FROM_QOS_%s_TBL[%2.2d]:  ", sel ? "DSCP" : "PRI", id);
	if (type)
		DBG_MSG("%s	", sel ? "DSCP" : "PRIO");
	if (color)
		DBG_MSG("COLOR	");
	if (gemid)
		DBG_MSG("GEMID	");
	if (low_q)
		DBG_MSG("LOW_Q	");
	if (high_q)
		DBG_MSG("HIGH_Q	");
	DBG_MSG("\n");

	DBG_MSG("FROM_ACT_TBL:		");
	if (type == 0)
		DBG_MSG("%s	", sel ? "DSCP" : "PRI");
	if (gemid == 0)
		DBG_MSG("GEMID	");
	if (low_q == 0)
		DBG_MSG("LOW_Q	");
	if (high_q == 0)
		DBG_MSG("HIGH_Q	");
	if (color == 0)
		DBG_MSG("COLOR	");
	DBG_MSG("\n\n");

	/*------------------------------*/
	/*	actions 0x1B60		*/
	/*------------------------------*/

	DBG_MSG(
		"ACT_CMD:	COLOR	PRIO	DSCP	GEMID	LOW_Q	HIGH_Q	FWD	POLICER	FID	RSS\n");
	DBG_MSG("		");

	DBG_MSG(
		"%1.1d\t%1.1d\t%1.1d\t%1.1d\t%1.1d\t%1.1d\t%1.1d\t%1.1d\t%1.1d\t%1.1d\t",
		((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_COLOR_MASK) >>
			MVPP2_CLS2_ACT_COLOR_OFF),
		((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_PRI_MASK) >>
			MVPP2_CLS2_ACT_PRI_OFF),
		((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_DSCP_MASK) >>
			MVPP2_CLS2_ACT_DSCP_OFF),
		((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_GEM_MASK) >>
			MVPP2_CLS2_ACT_GEM_OFF),
		((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_QL_MASK) >>
			MVPP2_CLS2_ACT_QL_OFF),
		((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_QH_MASK) >>
			MVPP2_CLS2_ACT_QH_OFF),
		((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_FRWD_MASK) >>
			MVPP2_CLS2_ACT_FRWD_OFF),
		((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_PLCR_MASK) >>
			MVPP2_CLS2_ACT_PLCR_OFF),
		((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_FLD_EN_MASK) >>
			MVPP2_CLS2_ACT_FLD_EN_OFF),
		((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_RSS_MASK) >>
			MVPP2_CLS2_ACT_RSS_OFF));
	DBG_MSG("\n\n");

	/*------------------------------*/
	/*	qos_attr 0x1B64		*/
	/*------------------------------*/
	DBG_MSG(
		"ACT_ATTR:		PRIO	DSCP	GEMID	LOW_Q	HIGH_Q	QUEUE\n");
	DBG_MSG("		");
	/* modify priority */
	int32bit = ((c2->sram.regs.qos_attr &
		    MVPP2_CLS2_ACT_QOS_ATTR_PRI_MASK) >>
		    MVPP2_CLS2_ACT_QOS_ATTR_PRI_OFF);
	DBG_MSG("	%1.1d\t", int32bit);

	/* modify dscp */
	int32bit = ((c2->sram.regs.qos_attr &
		    MVPP2_CLS2_ACT_QOS_ATTR_DSCP_MASK) >>
		    MVPP2_CLS2_ACT_QOS_ATTR_DSCP_OFF);
	DBG_MSG("%2.2d\t", int32bit);

	/* modify gemportid */
	int32bit = ((c2->sram.regs.qos_attr &
		    MVPP2_CLS2_ACT_QOS_ATTR_GEM_MASK) >>
		    MVPP2_CLS2_ACT_QOS_ATTR_GEM_OFF);
	DBG_MSG("0x%4.4x\t", int32bit);

	/* modify low Q */
	int32bit = ((c2->sram.regs.qos_attr &
		    MVPP2_CLS2_ACT_QOS_ATTR_QL_MASK) >>
		    MVPP2_CLS2_ACT_QOS_ATTR_QL_OFF);
	DBG_MSG("%1.1d\t", int32bit);

	/* modify high Q */
	int32bit = ((c2->sram.regs.qos_attr &
		    MVPP2_CLS2_ACT_QOS_ATTR_QH_MASK) >>
		    MVPP2_CLS2_ACT_QOS_ATTR_QH_OFF);
	DBG_MSG("0x%2.2x\t", int32bit);

	/*modify queue*/
	int32bit = ((c2->sram.regs.qos_attr &
		    (MVPP2_CLS2_ACT_QOS_ATTR_QL_MASK |
		    MVPP2_CLS2_ACT_QOS_ATTR_QH_MASK)));
	int32bit >>= MVPP2_CLS2_ACT_QOS_ATTR_QL_OFF;

	DBG_MSG("0x%2.2x\t", int32bit);
	DBG_MSG("\n\n");

	/*------------------------------*/
	/*	hwf_attr 0x1B68		*/
	/*------------------------------*/
	DBG_MSG("HWF_ATTR:		IPTR	DPTR	CHKSM   MTU_IDX\n");
	DBG_MSG("			");

	/* HWF modification instraction pointer */
	int32bit = ((c2->sram.regs.hwf_attr &
		    MVPP2_CLS2_ACT_HWF_ATTR_IPTR_MASK) >>
		    MVPP2_CLS2_ACT_HWF_ATTR_IPTR_OFF);
	DBG_MSG("0x%1.1x\t", int32bit);

	/* HWF modification data pointer */
	int32bit = ((c2->sram.regs.hwf_attr &
		    MVPP2_CLS2_ACT_HWF_ATTR_DPTR_MASK) >>
		    MVPP2_CLS2_ACT_HWF_ATTR_DPTR_OFF);
	DBG_MSG("0x%4.4x\t", int32bit);

	/* HWF modification instraction pointer */
	int32bit = ((c2->sram.regs.hwf_attr &
		    MVPP2_CLS2_ACT_HWF_ATTR_L4CHK_MASK) >>
		    MVPP2_CLS2_ACT_HWF_ATTR_L4CHK_OFF);
	DBG_MSG("%s\t", int32bit ? "ENABLE " : "DISABLE");

	/* mtu index */
	int32bit = ((c2->sram.regs.hwf_attr &
		    MVPP2_CLS2_ACT_HWF_ATTR_MTUIDX_MASK) >>
		    MVPP2_CLS2_ACT_HWF_ATTR_MTUIDX_OFF);
	DBG_MSG("0x%1.1x\t", int32bit);
	DBG_MSG("\n\n");

	/*------------------------------*/
	/*	CLSC2_ATTR2 0x1B6C	*/
	/*------------------------------*/
	DBG_MSG("RSS_ATTR:		RSS_EN		DUP_COUNT	DUP_PTR\n");
	DBG_MSG("			%d		%d		%d\n",
		((c2->sram.regs.rss_attr &
			MVPP2_CLS2_ACT_DUP_ATTR_RSSEN_MASK) >>
			MVPP2_CLS2_ACT_DUP_ATTR_RSSEN_OFF),
		((c2->sram.regs.rss_attr &
			MVPP2_CLS2_ACT_DUP_ATTR_DUPCNT_MASK) >>
			MVPP2_CLS2_ACT_DUP_ATTR_DUPCNT_OFF),
		((c2->sram.regs.rss_attr &
			MVPP2_CLS2_ACT_DUP_ATTR_DUPID_MASK) >>
			MVPP2_CLS2_ACT_DUP_ATTR_DUPID_OFF));
	DBG_MSG("\n");

	/*------------------------------*/
	/*	seq_attr 0x1B70		*/
	/*------------------------------*/
	/*PPv2.1 new feature MAS 3.14*/
	DBG_MSG("SEQ_ATTR:		ID	MISS\n");
	DBG_MSG("			0x%2.2x    0x%2.2x\n",
		((c2->sram.regs.seq_attr &
			MVPP21_CLS2_ACT_SEQ_ATTR_ID_MASK) >>
			MVPP21_CLS2_ACT_SEQ_ATTR_ID),
		((c2->sram.regs.seq_attr &
			MVPP21_CLS2_ACT_SEQ_ATTR_MISS_MASK) >>
			MVPP21_CLS2_ACT_SEQ_ATTR_MISS_OFF));
	DBG_MSG("\n\n");

	return MV_OK;
}

/*----------------------------------------------------------------------*/
int	mv_pp2x_cls_c2_hw_dump(struct mv_pp2x_hw *hw)
{
	int index;
	unsigned cnt;

	struct mv_pp2x_cls_c2_entry c2;

	memset(&c2, 0, sizeof(c2));

	for (index = 0; index < MVPP2_CLS_C2_TCAM_SIZE; index++) {
		mv_pp2x_cls_c2_hw_read(hw, index, &c2);
		if (c2.inv == 0) {
			mv_pp2x_cls_c2_sw_dump(&c2);
			mv_pp2x_cls_c2_hit_cntr_read(hw, index, &cnt);
			DBG_MSG("HITS: %d\n", cnt);
			DBG_MSG("-----------------------------------------\n");
		}
	}
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_hw_dump);

/*----------------------------------------------------------------------*/

void mv_pp2x_pp2_basic_print(struct platform_device *pdev, struct mv_pp2x *priv)
{
	DBG_MSG("%s\n", __func__);

	DBG_MSG("num_present_cpus(%d) num_act_cpus(%d) num_online_cpus(%d)\n",
		num_present_cpus(), num_active_cpus(), num_online_cpus());
	DBG_MSG("cpu_map(%x)\n", priv->cpu_map);

	DBG_MSG("pdev->name(%s) pdev->id(%d)\n", pdev->name, pdev->id);
	DBG_MSG("dev.init_name(%s) dev.id(%d)\n",
		pdev->dev.init_name, pdev->dev.id);
	DBG_MSG("dev.kobj.name(%s)\n", pdev->dev.kobj.name);
	DBG_MSG("dev->bus.name(%s) pdev.dev->bus.dev_name(%s)\n",
		pdev->dev.bus->name, pdev->dev.bus->dev_name);

	DBG_MSG("Device dma_coherent(%d)\n", pdev->dev.archdata.dma_coherent);

	DBG_MSG("pp2_ver(%d)\n", priv->pp2_version);
	DBG_MSG("queue_mode(%d)\n", priv->pp2_cfg.queue_mode);
	DBG_MSG("first_bm_pool(%d)\n", priv->pp2_cfg.first_bm_pool);
	DBG_MSG("cell_index(%d) num_ports(%d)\n",
		priv->pp2_cfg.cell_index, priv->num_ports);
	DBG_MSG("hw->base(%p)\n", priv->hw.base);
	if (priv->pp2_version == PPV22) {
		DBG_MSG("gop_addr: xmib(%p) smi(%p) xsmi(%p)\n",
			priv->hw.gop.gop_110.xmib.base,
			priv->hw.gop.gop_110.smi_base,
			priv->hw.gop.gop_110.xsmi_base);
		DBG_MSG("gop_addr: mspg(%p) xpcs(%p) ptp(%p)\n",
			priv->hw.gop.gop_110.mspg_base,
			priv->hw.gop.gop_110.xpcs_base,
			priv->hw.gop.gop_110.ptp.base);
		DBG_MSG("gop_addr: rfu1(%p)\n",
			priv->hw.gop.gop_110.rfu1_base);
	}
	DBG_MSG("uc_filter_max(%d), mc_filter_max(%d)\n",
		priv->pp2_cfg.uc_filter_max, priv->pp2_cfg.mc_filter_max);
}
EXPORT_SYMBOL(mv_pp2x_pp2_basic_print);

void mv_pp2x_pp2_port_print(struct mv_pp2x_port *port)
{
	int i;

	DBG_MSG("%s port_id(%d)\n", __func__, port->id);
	DBG_MSG("\t ifname(%s)\n", port->dev->name);
	DBG_MSG("\t first_rxq(%d)\n", port->first_rxq);
	DBG_MSG("\t num_irqs(%d)\n", port->num_irqs);
	for (i = 0; i < port->num_irqs; i++)
		DBG_MSG("\t\t irq%d(%d)\n", i, port->of_irqs[i]);
	DBG_MSG("\t pkt_size(%d)\n", port->pkt_size);
	DBG_MSG("\t flags(%llx)\n", port->flags);
	DBG_MSG("\t tx_ring_size(%d)\n", port->tx_ring_size);
	DBG_MSG("\t rx_ring_size(%d)\n", port->rx_ring_size);
	DBG_MSG("\t time_coal(%d)\n", port->tx_time_coal);
	DBG_MSG("\t pool_long(%p)\n", port->pool_long);
	DBG_MSG("\t pool_short(%p)\n", port->pool_short);
	DBG_MSG("\t first_rxq(%d)\n", port->first_rxq);
	DBG_MSG("\t num_rx_queues(%d)\n", port->num_rx_queues);
	DBG_MSG("\t num_tx_queues(%d)\n", port->num_tx_queues);
	DBG_MSG("\t num_qvector(%d)\n", port->num_qvector);

	for (i = 0; i < port->num_qvector; i++) {
		DBG_MSG("\t qvector_index(%d)\n", i);
#if !defined(CONFIG_MV_PP2_POLLING)
		DBG_MSG("\t\t irq(%d) irq_name:%s\n",
			port->q_vector[i].irq, port->q_vector[i].irq_name);
#endif
		DBG_MSG("\t\t qv_type(%d)\n",
			port->q_vector[i].qv_type);
		DBG_MSG("\t\t sw_thread_id	(%d)\n",
			port->q_vector[i].sw_thread_id);
		DBG_MSG("\t\t sw_thread_mask(%d)\n",
			port->q_vector[i].sw_thread_mask);
		DBG_MSG("\t\t first_rx_queue(%d)\n",
			port->q_vector[i].first_rx_queue);
		DBG_MSG("\t\t num_rx_queues(%d)\n",
			port->q_vector[i].num_rx_queues);
		DBG_MSG("\t\t pending_cause_rx(%d)\n",
			port->q_vector[i].pending_cause_rx);
	}
	DBG_MSG("\t GOP ind(%d) phy_mode(%d) phy_addr(%d)\n",
		port->mac_data.gop_index, port->mac_data.phy_mode,
		port->mac_data.phy_addr);
	DBG_MSG("\t GOP force_link(%d) autoneg(%d) duplex(%d) speed(%d)\n",
		port->mac_data.force_link, port->mac_data.autoneg,
		port->mac_data.duplex, port->mac_data.speed);
#if !defined(CONFIG_MV_PP2_POLLING)
	DBG_MSG("\t GOP link_irq(%d) irq_name:%s\n", port->mac_data.link_irq,
		port->mac_data.irq_name);
#endif
	DBG_MSG("\t GOP phy_dev(%p) phy_node(%p)\n", port->mac_data.phy_dev,
		port->mac_data.phy_node);
}
EXPORT_SYMBOL(mv_pp2x_pp2_port_print);

void mv_pp2x_pp2_ports_print(struct mv_pp2x *priv)
{
	int i;
	struct mv_pp2x_port *port;

	for (i = 0; i < priv->num_ports; i++) {
		if (!priv->port_list[i]) {
			pr_emerg("\t port_list[%d]= NULL!\n", i);
			continue;
		}
		port = priv->port_list[i];
		mv_pp2x_pp2_port_print(port);
	}
}
EXPORT_SYMBOL(mv_pp2x_pp2_ports_print);

int mv_pp2x_cls_hw_lkp_print(struct mv_pp2x_hw *hw, int lkpid, int way)
{
	unsigned int uint32bit;
	int int32bit;
	struct mv_pp2x_cls_lookup_entry lkp;

	if (mv_pp2x_range_validate(way, 0, WAY_MAX) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(lkpid, 0,
				   MVPP2_CLS_FLOWS_TBL_SIZE) == MV_ERROR)
		return MV_ERROR;

	mv_pp2x_cls_hw_lkp_read(hw, lkpid, way, &lkp);

	DBG_MSG(" 0x%2.2x  %1.1d\t", lkp.lkpid, lkp.way);
	mv_pp2x_cls_sw_lkp_rxq_get(&lkp, &int32bit);
	DBG_MSG("0x%2.2x\t", int32bit);
	mv_pp2x_cls_sw_lkp_en_get(&lkp, &int32bit);
	DBG_MSG("%1.1d\t", int32bit);
	mv_pp2x_cls_sw_lkp_flow_get(&lkp, &int32bit);
	DBG_MSG("0x%3.3x\t", int32bit);
	mv_pp2x_cls_sw_lkp_mod_get(&lkp, &int32bit);
	DBG_MSG(" 0x%2.2x\t", int32bit);
	mv_pp2x_cls_hw_lkp_hit_get(hw, lkp.lkpid, way, &uint32bit);
	DBG_MSG(" 0x%8.8x\n", uint32bit);
	DBG_MSG("\n");

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_hw_lkp_print);

/*----------------------------------------------------------------------*/

int mv_pp2x_cls_sw_flow_dump(struct mv_pp2x_cls_flow_entry *fe)
{
	int	int32bit_1, int32bit_2, i;
	int	fieldsArr[MVPP2_CLS_FLOWS_TBL_FIELDS_MAX];
	int	status = MV_OK;

	if (mv_pp2x_ptr_validate(fe) == MV_ERROR)
		return MV_ERROR;

	DBG_MSG(
		"INDEX: F[0] F[1] F[2] F[3] PRT[T  ID] ENG LAST LKP_TYP  PRIO\n");

	/*index*/
	DBG_MSG("0x%3.3x  ", fe->index);

	/*filed[0] filed[1] filed[2] filed[3]*/
	status |= mv_pp2x_cls_sw_flow_hek_get(fe, &int32bit_1, fieldsArr);

	for (i = 0 ; i < MVPP2_CLS_FLOWS_TBL_FIELDS_MAX; i++)
		if (i < int32bit_1)
			DBG_MSG("0x%2.2x ", fieldsArr[i]);
		else
			DBG_MSG(" NA  ");

	/*port_type port_id*/
	status |= mv_pp2x_cls_sw_flow_port_get(fe, &int32bit_1, &int32bit_2);
	DBG_MSG("[%1d  0x%3.3x]  ", int32bit_1, int32bit_2);

	/* engine_num last_bit*/
	status |= mv_pp2x_cls_sw_flow_engine_get(fe, &int32bit_1, &int32bit_2);
	DBG_MSG("%1d   %1d    ", int32bit_1, int32bit_2);

	/* lookup_type priority*/
	status |= mv_pp2x_cls_sw_flow_extra_get(fe, &int32bit_1, &int32bit_2);
	DBG_MSG("0x%2.2x    0x%2.2x", int32bit_1, int32bit_2);

	DBG_MSG("\n");
	DBG_MSG("\n");
	DBG_MSG("       PPPEO   VLAN   MACME   UDF7   SELECT SEQ_CTRL\n");
	DBG_MSG("         %1u      %1u      %1u       %1u      %1lu      %1u\n",
		(fe->data[0] & MVPP2_FLOW_PPPOE_MASK) >> MVPP2_FLOW_PPPOE,
		(fe->data[0] & MVPP2_FLOW_VLAN_MASK) >> MVPP2_FLOW_VLAN,
		(fe->data[0] & MVPP2_FLOW_MACME_MASK) >> MVPP2_FLOW_MACME,
		(fe->data[0] & MVPP2_FLOW_UDF7_MASK) >> MVPP2_FLOW_UDF7,
		(fe->data[0] & MVPP2_FLOW_PORT_ID_SEL_MASK) >>
		 MVPP2_FLOW_PORT_ID_SEL,
		(fe->data[1] & MVPP2_FLOW_SEQ_CTRL_MASK) >>
		 MVPP2_FLOW_SEQ_CTRL);
	DBG_MSG("\n");

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_sw_flow_dump);

/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*	additional cls debug APIs					*/
/*----------------------------------------------------------------------*/

int mv_pp2x_cls_hw_regs_dump(struct mv_pp2x_hw *hw)
{
	int i = 0;
	char reg_name[100];

	mv_pp2x_print_reg(hw, MVPP2_CLS_MODE_REG,
			  "MVPP2_CLS_MODE_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS_PORT_WAY_REG,
			  "MVPP2_CLS_PORT_WAY_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS_LKP_INDEX_REG,
			  "MVPP2_CLS_LKP_INDEX_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS_LKP_TBL_REG,
			  "MVPP2_CLS_LKP_TBL_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS_FLOW_INDEX_REG,
			  "MVPP2_CLS_FLOW_INDEX_REG");

	mv_pp2x_print_reg(hw, MVPP2_CLS_FLOW_TBL0_REG,
			  "MVPP2_CLS_FLOW_TBL0_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS_FLOW_TBL1_REG,
			  "MVPP2_CLS_FLOW_TBL1_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS_FLOW_TBL2_REG,
			  "MVPP2_CLS_FLOW_TBL2_REG");

	mv_pp2x_print_reg(hw, MVPP2_CLS_PORT_SPID_REG,
			  "MVPP2_CLS_PORT_SPID_REG");

	for (i = 0; i < MVPP2_CLS_SPID_UNI_REGS; i++) {
		sprintf(reg_name, "MVPP2_CLS_SPID_UNI_%d_REG", i);
		mv_pp2x_print_reg(hw, (MVPP2_CLS_SPID_UNI_BASE_REG + (4 * i)),
				  reg_name);
	}
	for (i = 0; i < MVPP2_CLS_GEM_VIRT_REGS_NUM; i++) {
		/* indirect access */
		mv_pp2x_write(hw, MVPP2_CLS_GEM_VIRT_INDEX_REG, i);
		sprintf(reg_name, "MVPP2_CLS_GEM_VIRT_%d_REG", i);
		mv_pp2x_print_reg(hw, MVPP2_CLS_GEM_VIRT_REG, reg_name);
	}
	for (i = 0; i < MVPP2_CLS_UDF_BASE_REGS; i++)	{
		sprintf(reg_name, "MVPP2_CLS_UDF_REG_%d_REG", i);
		mv_pp2x_print_reg(hw, MVPP2_CLS_UDF_REG(i), reg_name);
	}
	for (i = 0; i < 16; i++) {
		sprintf(reg_name, "MVPP2_CLS_MTU_%d_REG", i);
		mv_pp2x_print_reg(hw, MVPP2_CLS_MTU_REG(i), reg_name);
	}
	for (i = 0; i < MVPP2_MAX_PORTS; i++) {
		sprintf(reg_name, "MVPP2_CLS_OVER_RXQ_LOW_%d_REG", i);
		mv_pp2x_print_reg(hw, MVPP2_CLS_OVERSIZE_RXQ_LOW_REG(i),
				  reg_name);
	}
	for (i = 0; i < MVPP2_MAX_PORTS; i++) {
		sprintf(reg_name, "MVPP2_CLS_SWFWD_P2HQ_%d_REG", i);
		mv_pp2x_print_reg(hw, MVPP2_CLS_SWFWD_P2HQ_REG(i), reg_name);
	}

	mv_pp2x_print_reg(hw, MVPP2_CLS_SWFWD_PCTRL_REG,
			  "MVPP2_CLS_SWFWD_PCTRL_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS_SEQ_SIZE_REG,
			  "MVPP2_CLS_SEQ_SIZE_REG");

	for (i = 0; i < MVPP2_MAX_PORTS; i++) {
		sprintf(reg_name, "MVPP2_CLS_PCTRL_%d_REG", i);
		mv_pp2x_print_reg(hw, MV_PP2_CLS_PCTRL_REG(i), reg_name);
	}

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_hw_regs_dump);

/*----------------------------------------------------------------------*/

int mv_pp2x_cls_hw_flow_dump(struct mv_pp2x_hw *hw)
{
	int index;
	unsigned int cnt;

	struct mv_pp2x_cls_flow_entry fe;

	for (index = 0; index < MVPP2_CLS_FLOWS_TBL_SIZE ; index++) {
		mv_pp2x_cls_hw_flow_read(hw, index, &fe);
		mv_pp2x_cls_sw_flow_dump(&fe);
		mv_pp2x_cls_hw_flow_hit_get(hw, index, &cnt);
		DBG_MSG("HITS = %d\n", cnt);
		DBG_MSG("-------------------------------------------------\n");
	}
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_hw_flow_dump);

/*----------------------------------------------------------------------*/
/*PPv2.1 new counters MAS 3.20*/
int mv_pp2x_cls_hw_flow_hits_dump(struct mv_pp2x_hw *hw)
{
	int index;
	unsigned int cnt;
	struct mv_pp2x_cls_flow_entry fe;

	for (index = 0; index < MVPP2_CLS_FLOWS_TBL_SIZE ; index++) {
		mv_pp2x_cls_hw_flow_hit_get(hw, index, &cnt);
		if (cnt != 0) {
			mv_pp2x_cls_hw_flow_read(hw, index, &fe);
			mv_pp2x_cls_sw_flow_dump(&fe);
			DBG_MSG("HITS = %d\n", cnt);
			DBG_MSG("\n");
		}
	}

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_hw_flow_hits_dump);

/*----------------------------------------------------------------------*/
/*PPv2.1 new counters MAS 3.20*/
int mv_pp2x_cls_hw_lkp_hits_dump(struct mv_pp2x_hw *hw)
{
	int index, way, entry_ind;
	unsigned int cnt;

	DBG_MSG("< ID  WAY >:	HITS\n");
	for (index = 0; index < MVPP2_CLS_LKP_TBL_SIZE ; index++)
		for (way = 0; way < 2 ; way++)	{
			entry_ind = (way << MVPP2_CLS_LKP_INDEX_WAY_OFFS) |
				index;
			mv_pp2x_cls_hw_lkp_hit_get(hw, index, way,  &cnt);
			if (cnt != 0)
				DBG_MSG(" 0x%2.2x  %1.1d\t0x%8.8x\n",
					index, way, cnt);
	}
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_hw_lkp_hits_dump);

/*----------------------------------------------------------------------*/
int mv_pp2x_cls_sw_lkp_dump(struct mv_pp2x_cls_lookup_entry *lkp)
{
	int int32bit;
	int status = 0;

	if (mv_pp2x_ptr_validate(lkp) == MV_ERROR)
		return MV_ERROR;

	DBG_MSG("< ID  WAY >:	RXQ	EN	FLOW	MODE_BASE\n");

	/* id */
	DBG_MSG(" 0x%2.2x  %1.1d\t", lkp->lkpid, lkp->way);

	/*rxq*/
	status |= mv_pp2x_cls_sw_lkp_rxq_get(lkp, &int32bit);
	DBG_MSG("0x%2.2x\t", int32bit);

	/*enabe bit*/
	status |= mv_pp2x_cls_sw_lkp_en_get(lkp, &int32bit);
	DBG_MSG("%1.1d\t", int32bit);

	/*flow*/
	status |= mv_pp2x_cls_sw_lkp_flow_get(lkp, &int32bit);
	DBG_MSG("0x%3.3x\t", int32bit);

	/*mode*/
	status |= mv_pp2x_cls_sw_lkp_mod_get(lkp, &int32bit);
	DBG_MSG(" 0x%2.2x\t", int32bit);

	DBG_MSG("\n");

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_sw_lkp_dump);

int mv_pp2x_cls_hw_lkp_dump(struct mv_pp2x_hw *hw)
{
	int index, way, int32bit, ind;
	unsigned int uint32bit;

	struct mv_pp2x_cls_lookup_entry lkp;

	DBG_MSG("< ID  WAY >:	RXQ	EN	FLOW	MODE_BASE  HITS\n");
	for (index = 0; index < MVPP2_CLS_LKP_TBL_SIZE ; index++)
		for (way = 0; way < 2 ; way++)	{
			ind = (way << MVPP2_CLS_LKP_INDEX_WAY_OFFS) | index;
			mv_pp2x_cls_hw_lkp_read(hw, index, way, &lkp);
			DBG_MSG(" 0x%2.2x  %1.1d\t", lkp.lkpid, lkp.way);
			mv_pp2x_cls_sw_lkp_rxq_get(&lkp, &int32bit);
			DBG_MSG("0x%2.2x\t", int32bit);
			mv_pp2x_cls_sw_lkp_en_get(&lkp, &int32bit);
			DBG_MSG("%1.1d\t", int32bit);
			mv_pp2x_cls_sw_lkp_flow_get(&lkp, &int32bit);
			DBG_MSG("0x%3.3x\t", int32bit);
			mv_pp2x_cls_sw_lkp_mod_get(&lkp, &int32bit);
			DBG_MSG(" 0x%2.2x\t", int32bit);
			mv_pp2x_cls_hw_lkp_hit_get(hw, index, way, &uint32bit);
			DBG_MSG(" 0x%8.8x\n", uint32bit);
			DBG_MSG("\n");
		}
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_hw_lkp_dump);

/*----------------------------------------------------------------------*/

int mv_pp2x_cls_c2_sw_words_dump(struct mv_pp2x_cls_c2_entry *c2)
{
	int i;

	if (mv_pp2x_ptr_validate(c2) == MV_ERROR)
		return MV_ERROR;

	/* TODO check size */
	/* hw entry id */
	DBG_MSG("[0x%3.3x] ", c2->index);

	i = MVPP2_CLS_C2_TCAM_WORDS - 1;

	while (i >= 0)
		DBG_MSG("%4.4x ", (c2->tcam.words[i--]) & 0xFFFF);

	DBG_MSG("| ");

	DBG_MSG("%8.8x %8.8x %8.8x %8.8x %8.8x", c2->sram.words[4],
		c2->sram.words[3], c2->sram.words[2], c2->sram.words[1],
		c2->sram.words[0]);

	/*tcam inValid bit*/
	DBG_MSG(" %s", (c2->inv == 1) ? "[inv]" : "[valid]");

	DBG_MSG("\n        ");

	i = MVPP2_CLS_C2_TCAM_WORDS - 1;

	while (i >= 0)
		DBG_MSG("%4.4x ", ((c2->tcam.words[i--] >> 16)  & 0xFFFF));

	DBG_MSG("\n");

	return MV_OK;
}

/*----------------------------------------------------------------------*/
int mv_pp2x_cls_c2_hit_cntr_dump(struct mv_pp2x_hw *hw)
{
	int i;
	unsigned int cnt;

	for (i = 0; i < MVPP2_CLS_C2_TCAM_SIZE; i++) {
		mv_pp2x_cls_c2_hit_cntr_read(hw, i, &cnt);
		if (cnt != 0)
			DBG_MSG("INDEX: 0x%8.8X	VAL: 0x%8.8X\n", i, cnt);
	}
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_hit_cntr_dump);

/*----------------------------------------------------------------------*/

int mv_pp2x_cls_c2_regs_dump(struct mv_pp2x_hw *hw)
{
	int i;
	char reg_name[100];

	mv_pp2x_print_reg(hw, MVPP2_CLS2_TCAM_IDX_REG,
			  "MVPP2_CLS2_TCAM_IDX_REG");

	for (i = 0; i < MVPP2_CLS_C2_TCAM_WORDS; i++) {
		printk(reg_name, "MVPP2_CLS2_TCAM_DATA_%d_REG", i);
		mv_pp2x_print_reg(hw, MVPP2_CLS2_TCAM_DATA_REG(i), reg_name);
	}

	mv_pp2x_print_reg(hw, MVPP2_CLS2_TCAM_INV_REG,
			  "MVPP2_CLS2_TCAM_INV_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS2_ACT_DATA_REG,
			  "MVPP2_CLS2_ACT_DATA_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS2_DSCP_PRI_INDEX_REG,
			  "MVPP2_CLS2_DSCP_PRI_INDEX_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS2_QOS_TBL_REG,
			  "MVPP2_CLS2_QOS_TBL_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS2_ACT_REG,
			  "MVPP2_CLS2_ACT_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS2_ACT_QOS_ATTR_REG,
			  "MVPP2_CLS2_ACT_QOS_ATTR_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS2_ACT_HWF_ATTR_REG,
			  "MVPP2_CLS2_ACT_HWF_ATTR_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS2_ACT_DUP_ATTR_REG,
			  "MVPP2_CLS2_ACT_DUP_ATTR_REG");
	mv_pp2x_print_reg(hw, MVPP22_CLS2_ACT_SEQ_ATTR_REG,
			  "MVPP22_CLS2_ACT_SEQ_ATTR_REG");
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_regs_dump);

int mv_pp22_rss_hw_dump(struct mv_pp2x_hw *hw)
{
	int tbl_id, tbl_line;

	struct mv_pp22_rss_entry rss_entry;

	memset(&rss_entry, 0, sizeof(struct mv_pp22_rss_entry));

	rss_entry.sel = MVPP22_RSS_ACCESS_TBL;

	for (tbl_id = 0; tbl_id < MVPP22_RSS_TBL_NUM; tbl_id++) {
		DBG_MSG("\n-------- RSS TABLE %d-----------\n", tbl_id);
		DBG_MSG("HASH	QUEUE	WIDTH\n");

		for (tbl_line = 0; tbl_line < MVPP22_RSS_TBL_LINE_NUM;
			tbl_line++) {
			rss_entry.u.entry.tbl_id = tbl_id;
			rss_entry.u.entry.tbl_line = tbl_line;
			mv_pp22_rss_tbl_entry_get(hw, &rss_entry);
			DBG_MSG("0x%2.2x\t", rss_entry.u.entry.tbl_line);
			DBG_MSG("0x%2.2x\t", rss_entry.u.entry.rxq);
			DBG_MSG("0x%2.2x", rss_entry.u.entry.width);
			DBG_MSG("\n");
		}
	}
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp22_rss_hw_dump);



