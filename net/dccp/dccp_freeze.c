/*
 * DCCP freezing mechanisms
 * Copyright (C) 2010 Nicta, Olivier Mehani <olivier.mehani@nicta.com.au>
 * 
 * Generic freezing mechanisms.
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
#include <linux/dccp.h>

#include "ccid.h"
#include "dccp.h"

int dccp_freeze_setup_state(struct sock *sk, int senderonly, enum dccp_freeze_states state) {
	int errrx = 0, errtx;
	struct dccp_sock *dp = dccp_sk(sk);

	if (senderonly == 0) 
		errrx = ccid_hc_rx_setsockopt(dp->dccps_hc_rx_ccid, sk,
				DCCP_SOCKOPT_CCID_RX_FREEZE, (u32 *) state, sizeof(state));

	errtx = ccid_hc_tx_setsockopt(dp->dccps_hc_tx_ccid, sk,
			DCCP_SOCKOPT_CCID_TX_FREEZE, (u32 *) state, sizeof(state));

	if (errrx || errtx)
		DCCP_WARN("Could not fully change freeze state to %d: errrx=%d, errtx=%d\n",
				state, errrx, errtx);

	return 0;
}
