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

int ccid3_rx_freeze(struct ccid3_hc_rx_sock *hcrx)
{
	if (ccid3_rx_freeze_get(hcrx) != TFRC_FREEZE_RSTATE_NORMAL) {
		DCCP_WARN("CCID3 receiver asked to freeze while already in state %d\n",\
				ccid3_rx_freeze_get(hcrx));
	} else {
		dccp_pr_debug("signalling sender to freeze");
		ccid3_rx_freeze_set(hcrx, TFRC_FREEZE_RSTATE_SIGNAL_FREEZE);
	}
	return 0;
}

int ccid3_rx_unfreeze(struct ccid3_hc_rx_sock *hcrx)
{
	if (ccid3_rx_freeze_get(hcrx) != TFRC_FREEZE_RSTATE_SIGNAL_FREEZE) {
		DCCP_WARN("CCID3 receiver asked to unfreeze while already in state %d\n",\
				ccid3_rx_freeze_get(hcrx));
	} else {
		dccp_pr_debug("signalling sender to unfreeze");
		ccid3_rx_freeze_set(hcrx, TFRC_FREEZE_RSTATE_SIGNAL_UNFREEZE);
	}
	return 0;
}

int ccid3_tx_freeze(struct ccid3_hc_tx_sock *hctx)
{
	if (ccid3_tx_freeze_get(hctx) == TFRC_FREEZE_SSTATE_FROZEN)
		return 0;

	if (ccid3_tx_freeze_get(hctx) != TFRC_FREEZE_SSTATE_NORMAL) {
		DCCP_WARN("CCID3 sender asked to freeze while already in state %d\n",\
				ccid3_tx_freeze_get(hctx));
	} else {
		dccp_pr_debug("freezing");
		hctx->tx_freeze_x_recv = hctx->tx_x_recv;
		ccid3_tx_freeze_set(hctx, TFRC_FREEZE_SSTATE_FROZEN);
	}
	return 0;
}

int ccid3_tx_unfreeze(struct ccid3_hc_tx_sock *hctx)
{
	if (ccid3_tx_freeze_get(hctx) == TFRC_FREEZE_SSTATE_RESTORING)
		return 0;

	if (ccid3_tx_freeze_get(hctx) != TFRC_FREEZE_SSTATE_FROZEN) {
		DCCP_WARN("CCID3 sender asked to unfreeze while in state %d\n",\
				ccid3_tx_freeze_get(hctx));
	} else {
		dccp_pr_debug("unfreezing");
		hctx->tx_x_recv = hctx->tx_freeze_x_recv;
		ccid3_tx_freeze_set(hctx, TFRC_FREEZE_SSTATE_RESTORING);
	}
	return 0;
}

int ccid3_tx_receiver_unfrozen(struct ccid3_hc_tx_sock *hctx) {
	if (ccid3_tx_freeze_get(hctx) == TFRC_FREEZE_SSTATE_PROBING)
		return 0;

	if (ccid3_tx_freeze_get(hctx) != TFRC_FREEZE_SSTATE_RESTORING) {
		DCCP_WARN("CCID3 receiver unfrozen while in state %d\n",\
				ccid3_tx_freeze_get(hctx));
	} else {
		dccp_pr_debug("receiver unfrozen, probing");
		hctx->tx_x_recv = hctx->tx_freeze_x_recv;
		ccid3_tx_freeze_set(hctx, TFRC_FREEZE_SSTATE_PROBING);
	}
	return 0;
}

int ccid3_rx_sender_restoring(struct ccid3_hc_rx_sock *hcrx, ktime_t now) {
	if (ccid3_rx_freeze_get(hcrx) == TFRC_FREEZE_RSTATE_RESTORATION)
		return 0;

	if (ccid3_rx_freeze_get(hcrx) != TFRC_FREEZE_RSTATE_NORMAL &&
			ccid3_rx_freeze_get(hcrx) != TFRC_FREEZE_RSTATE_SIGNAL_UNFREEZE) {
		DCCP_WARN("CCID3 sender restoring while in state %d\n",\
				ccid3_rx_freeze_get(hcrx));
	} else {
		dccp_pr_debug("sender restoring");
		ccid3_rx_freeze_set(hcrx, TFRC_FREEZE_RSTATE_RESTORATION);
		hcrx->rx_freeze_restoration_start = now;
	}
	return 0;
}

int ccid3_rx_check_restoration_finished(struct ccid3_hc_rx_sock *hcrx, ktime_t now) {
	ktime_t restoration_duration;
	if (ccid3_rx_freeze_get(hcrx) != TFRC_FREEZE_RSTATE_RESTORATION) {
		DCCP_WARN("CCID3 checking restoration in state %d\n",\
				ccid3_rx_freeze_get(hcrx));
	} else {
		restoration_duration = ktime_sub(now, hcrx->rx_freeze_restoration_start);
		if(ktime_to_us(restoration_duration) > hcrx->rx_rtt) {
			dccp_pr_debug("restoration finished");
			hcrx->rx_freeze_restoration_start = ktime_set(0, 0);	
			ccid3_rx_freeze_set(hcrx, TFRC_FREEZE_RSTATE_RECOVERY1);
			return 1;
		}
	}
	return 0;
}

int ccid3_rx_sender_probing(struct ccid3_hc_rx_sock *hcrx) {
	if (ccid3_rx_freeze_get(hcrx) == TFRC_FREEZE_RSTATE_PROBED)
		return 0;

	if (ccid3_rx_freeze_get(hcrx) != TFRC_FREEZE_RSTATE_RECOVERY1) {
		DCCP_WARN("CCID3 sender restoring while in state %d\n",\
				ccid3_rx_freeze_get(hcrx));
	} else {
		dccp_pr_debug("sender probing");
		ccid3_rx_freeze_set(hcrx, TFRC_FREEZE_RSTATE_PROBED);
	}
	return 0;
}

