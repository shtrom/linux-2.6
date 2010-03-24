/*
 * DCCP freezing mechanisms
 * Copyright (C) 2010 Nicta, Olivier Mehani <olivier.mehani@nicta.com.au>
 * 
 * $Id: gplnotice.ab.c 4 2007-06-19 17:36:57Z shtrom $
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
	/* Send DCCPO_FREEZE */
	return 0;
}

int ccid3_rx_unfreeze(struct ccid3_hc_rx_sock *hcrx)
{
	/* Send DCCPO_UNFREEZE */
	return 0;
}

int ccid3_tx_freeze(struct ccid3_hc_tx_sock *hctx)
{
	if (ccid3_tx_freeze_get(hctx) != TFRC_FREEZE_SSTATE_NORMAL) {
		DCCP_WARN("Freezing while not in normal state\n");
	} else {
		hctx->tx_freeze_x_recv = hctx->tx_x_recv;
	}
	ccid3_tx_freeze_set(hctx, TFRC_FREEZE_SSTATE_FROZEN);
	return 0;
}

int ccid3_tx_unfreeze(struct ccid3_hc_tx_sock *hctx)
{
	printk(KERN_DEBUG "%s not implemented\n", __FUNCTION__);
	return 0;
}
