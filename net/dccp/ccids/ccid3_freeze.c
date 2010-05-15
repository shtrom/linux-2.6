/*
 * DCCP freezing mechanisms
 * Copyright (C) 2010 Nicta, Olivier Mehani <olivier.mehani@nicta.com.au>
 * 
 * Freezing mechanisms specific to CCID3.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "../dccp.h"
#include "ccid3.h"

#ifdef CONFIG_IP_DCCP_CCID3_FREEZE_DEBUG
int ccid3_freeze_debug;
#endif

int ccid3_rx_freeze(struct ccid3_hc_rx_sock *hcrx, u64 seqno)
{
	int ret = 0;
	if (ccid3_rx_freeze_get(hcrx) != TFRC_FREEZE_RSTATE_NORMAL &&
			ccid3_rx_freeze_get(hcrx) != TFRC_FREEZE_RSTATE_SIGNAL_FREEZE &&
			ccid3_rx_freeze_get(hcrx) != TFRC_FREEZE_RSTATE_SIGNAL_UNFREEZE) {
		DCCP_WARN("CCID3 receiver asked to freeze while in state %d\n",\
				ccid3_rx_freeze_get(hcrx));
		ret = 1;
	} 

	ccid3_freeze_pr_debug("signaling sender to freeze");
	ccid3_rx_freeze_set(hcrx, TFRC_FREEZE_RSTATE_SIGNAL_FREEZE, seqno);
	
	return ret;
}

int ccid3_rx_unfreeze(struct ccid3_hc_rx_sock *hcrx, u64 seqno)
{
	if (ccid3_rx_freeze_get(hcrx) != TFRC_FREEZE_RSTATE_SIGNAL_FREEZE &&
			ccid3_rx_freeze_get(hcrx) != TFRC_FREEZE_RSTATE_SIGNAL_UNFREEZE) {
		DCCP_WARN("CCID3 receiver asked to unfreeze while already in state %d\n",\
				ccid3_rx_freeze_get(hcrx));
		return 2;
	}

	ccid3_freeze_pr_debug("signaling sender to unfreeze");
	ccid3_rx_freeze_set(hcrx, TFRC_FREEZE_RSTATE_SIGNAL_UNFREEZE, seqno);
	
	return 0;
}

int ccid3_tx_freeze(struct ccid3_hc_tx_sock *hctx, u64 seqno)
{
	if (ccid3_tx_freeze_get(hctx) == TFRC_FREEZE_SSTATE_FROZEN)
		return 1;

	if (ccid3_tx_freeze_get(hctx) != TFRC_FREEZE_SSTATE_NORMAL) {
		DCCP_WARN("CCID3 sender asked to freeze while in state %d, keeping previous values\n",\
				ccid3_tx_freeze_get(hctx));
		ccid3_tx_freeze_set(hctx, TFRC_FREEZE_SSTATE_FROZEN, seqno);
		return 0;
	}

	ccid3_freeze_pr_debug("freezing, saving Xrecv=%d, p=%d", hctx->tx_x_recv, hctx->tx_p);
	hctx->tx_freeze_x_recv = hctx->tx_x_recv;
	hctx->tx_freeze_p = hctx->tx_p;
	ccid3_tx_freeze_set(hctx, TFRC_FREEZE_SSTATE_FROZEN, seqno);
	
	return 0;
}

int ccid3_tx_unfreeze(struct ccid3_hc_tx_sock *hctx, u64 seqno)
{
	ktime_t now = ktime_get_real();

	if (ccid3_tx_freeze_get(hctx) == TFRC_FREEZE_SSTATE_RESTORING)
		return 1;

	if (ccid3_tx_freeze_get(hctx) != TFRC_FREEZE_SSTATE_FROZEN) {
		DCCP_WARN("CCID3 sender asked to unfreeze while in state %d\n",\
				ccid3_tx_freeze_get(hctx));
		return 2;
	}

	ccid3_freeze_pr_debug("unfreezing, going into restoring mode (Xrecv=%u, p=%u)",
			hctx->tx_freeze_x_recv, hctx->tx_freeze_p);
	hctx->tx_x_recv = hctx->tx_freeze_x_recv;
	hctx->tx_p = hctx->tx_freeze_p;
	hctx->tx_x = hctx->tx_freeze_x_recv;
	hctx->tx_t_nom = now;
	ccid3_tx_freeze_set(hctx, TFRC_FREEZE_SSTATE_RESTORING, seqno);

	return 0;
}

int ccid3_tx_receiver_unfrozen(struct ccid3_hc_tx_sock *hctx, u64 seqno)
{
	if (ccid3_tx_freeze_get(hctx) == TFRC_FREEZE_SSTATE_PROBING)
		return 1;

	if (ccid3_tx_freeze_get(hctx) != TFRC_FREEZE_SSTATE_RESTORING) {
		DCCP_WARN("CCID3 receiver unfrozen while in state %d\n",\
				ccid3_tx_freeze_get(hctx));
		return 2;
	}

	ccid3_freeze_pr_debug("receiver unfrozen, probing");
	hctx->tx_x_recv = hctx->tx_freeze_x_recv;
	ccid3_tx_freeze_set(hctx, TFRC_FREEZE_SSTATE_PROBING, seqno);

	return 0;
}

int ccid3_rx_end_probing(struct ccid3_hc_rx_sock *hcrx, u64 seqno)
{
	if (ccid3_rx_freeze_get(hcrx) != TFRC_FREEZE_RSTATE_PROBED) {
		DCCP_WARN("CCID3 ending probing while in state %d\n",\
				ccid3_rx_freeze_get(hcrx));
		return 2;
	}

	ccid3_freeze_pr_debug("ending probing phase: clearing state");

	tfrc_lh_cleanup(&hcrx->rx_li_hist);
	tfrc_rx_hist_resume_rtt_sampling(&hcrx->rx_hist);

	ccid3_rx_freeze_set(hcrx, TFRC_FREEZE_RSTATE_NORMAL, seqno);

	return 0;
}

int ccid3_rx_sender_restoring(struct ccid3_hc_rx_sock *hcrx, u64 seqno)
{
	ktime_t now = ktime_get_real();

	if (ccid3_rx_freeze_get(hcrx) == TFRC_FREEZE_RSTATE_RESTORATION)
		return 0;

	if (ccid3_rx_freeze_get(hcrx) != TFRC_FREEZE_RSTATE_NORMAL &&
			ccid3_rx_freeze_get(hcrx) != TFRC_FREEZE_RSTATE_SIGNAL_UNFREEZE) {
		DCCP_WARN("CCID3 sender restoring while in state %d\n",\
				ccid3_rx_freeze_get(hcrx));
		return 2;
	}

	ccid3_freeze_pr_debug("sender restoring, will stop after one RTT estimate (%dus)",
			hcrx->rx_hist.rtt_estimate);
	hcrx->rx_freeze_restoration_end = ktime_add_us(now, hcrx->rx_hist.rtt_estimate);
	ccid3_rx_freeze_set(hcrx, TFRC_FREEZE_RSTATE_RESTORATION, seqno);
	
	return 0;
}

int ccid3_rx_check_restoration_finished(struct ccid3_hc_rx_sock *hcrx, u64 seqno)
{
	ktime_t now = ktime_get_real();

	if (ccid3_rx_freeze_get(hcrx) != TFRC_FREEZE_RSTATE_RESTORATION) {
		DCCP_WARN("CCID3 checking restoration in state %d\n",\
				ccid3_rx_freeze_get(hcrx));
	} else {
		if(ktime_compare(now, hcrx->rx_freeze_restoration_end) > 0) {
			ccid3_freeze_pr_debug("restoration finished after %ldus",
					(long)ktime_to_us(ktime_sub(now,
							hcrx->rx_freeze_restoration_end)));
			hcrx->rx_freeze_restoration_end = ktime_set(0, 0);	
			ccid3_rx_freeze_set(hcrx, TFRC_FREEZE_RSTATE_RECOVERY1, seqno);
			return 1;
		}
	}
	return 0;
}

int ccid3_rx_sender_probing(struct ccid3_hc_rx_sock *hcrx, u64 seqno)
{
	if (ccid3_rx_freeze_get(hcrx) == TFRC_FREEZE_RSTATE_PROBED)
		return 1;

	if (ccid3_rx_freeze_get(hcrx) != TFRC_FREEZE_RSTATE_RECOVERY1) {
		DCCP_WARN("CCID3 sender probing while in state %d\n",\
				ccid3_rx_freeze_get(hcrx));
	} else {
		ccid3_freeze_pr_debug("sender probing");
		ccid3_rx_freeze_set(hcrx, TFRC_FREEZE_RSTATE_PROBED, seqno);
	}
	return 0;
}

#ifdef CONFIG_IP_DCCP_CCID3_FREEZE_DEBUG
module_param(ccid3_freeze_debug, bool, 0644);
MODULE_PARM_DESC(ccid3_freeze_debug, "Enable debug messages for Freeze-TFRC");
#endif
