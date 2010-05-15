/*
 *  Copyright (c) 2005-7 The University of Waikato, Hamilton, New Zealand.
 *  Copyright (c) 2007   The University of Aberdeen, Scotland, UK
 *
 *  An implementation of the DCCP protocol
 *
 *  This code has been developed by the University of Waikato WAND
 *  research group. For further information please see http://www.wand.net.nz/
 *  or e-mail Ian McDonald - ian.mcdonald@jandi.co.nz
 *
 *  This code also uses code from Lulea University, rereleased as GPL by its
 *  authors:
 *  Copyright (c) 2003 Nils-Erik Mattsson, Joacim Haggmark, Magnus Erixzon
 *
 *  Changes to meet Linux coding standards, to make it meet latest ccid3 draft
 *  and to make it work as a loadable module in the DCCP stack written by
 *  Arnaldo Carvalho de Melo <acme@conectiva.com.br>.
 *
 *  Copyright (c) 2005 Arnaldo Carvalho de Melo <acme@conectiva.com.br>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef _DCCP_CCID3_H_
#define _DCCP_CCID3_H_

#include <linux/ktime.h>
#include <linux/list.h>
#include <linux/types.h>
#include <linux/tfrc.h>
#include "lib/tfrc.h"
#include "../ccid.h"

/* Two seconds as per RFC 3448 4.2 */
#define TFRC_INITIAL_TIMEOUT	   (2 * USEC_PER_SEC)

/* Maximum backoff interval t_mbi (RFC 3448, 4.3) */
#define TFRC_T_MBI		   (64 * USEC_PER_SEC)

/*
 * The t_delta parameter (RFC 3448, 4.6): delays of less than %USEC_PER_MSEC are
 * rounded down to 0, since sk_reset_timer() here uses millisecond granularity.
 * Hence we can use a constant t_delta = %USEC_PER_MSEC when HZ >= 500. A coarse
 * resolution of HZ < 500 means that the error is below one timer tick (t_gran)
 * when using the constant t_delta  =  t_gran / 2  =  %USEC_PER_SEC / (2 * HZ).
 */
#if (HZ >= 500)
# define TFRC_T_DELTA		   USEC_PER_MSEC
#else
# define TFRC_T_DELTA		   (USEC_PER_SEC / (2 * HZ))
#endif

#ifdef CONFIG_IP_DCCP_CCID3_FREEZE_DEBUG
extern int ccid3_freeze_debug;
#define ccid3_freeze_pr_debug(format, a...)	DCCP_PR_DEBUG(ccid3_freeze_debug, format, ##a)
#else
#define ccid3_freeze_pr_debug(format, a...)
#endif

#ifdef CONFIG_IP_DCCP_CCID3_FREEZE
/* Variation in p to exit the probing state */
# define FREEZE_DELTA_P		10

struct ccid3_hc_rx_freeze_options {
	u64 ccid3or_seqno:48,
	    ccid3or_sender_freeze:16;
};
#endif

enum ccid3_options {
	TFRC_OPT_LOSS_EVENT_RATE = 192,
	TFRC_OPT_LOSS_INTERVALS	 = 193,
	TFRC_OPT_RECEIVE_RATE	 = 194,
#ifdef CONFIG_IP_DCCP_CCID3_FREEZE
	TFRC_OPT_UNFROZEN,
	TFRC_OPT_RESTORING	 = 129,
	TFRC_OPT_PROBING,
#endif
};

#ifdef CONFIG_IP_DCCP_CCID3_FREEZE
/* Freeze-TFRC sender states */
enum ccid3_hc_tx_freeze_states {
	TFRC_FREEZE_SSTATE_NORMAL = 0,
	TFRC_FREEZE_SSTATE_FROZEN,
	TFRC_FREEZE_SSTATE_RESTORING,
	TFRC_FREEZE_SSTATE_PROBING,
};
#endif

/**
 * struct ccid3_hc_tx_sock - CCID3 sender half-connection socket
 * @tx_x:		  Current sending rate in 64 * bytes per second
 * @tx_x_recv:		  Receive rate in 64 * bytes per second
 * @tx_x_calc:		  Calculated rate in bytes per second
 * @tx_rtt:		  Estimate of current round trip time in usecs
 * @tx_r_sqmean:	  Estimate of long-term RTT (RFC 5348, 4.5)
 * @tx_p:		  Current loss event rate (0-1) scaled by 1000000
 * @tx_s:		  Packet size in bytes
 * @tx_t_rto:		  Nofeedback Timer setting in usecs
 * @tx_t_ipi:		  Interpacket (send) interval (RFC 3448, 4.6) in usecs
 * @tx_feedback:	  Whether feedback has been received or not
 * @tx_last_win_count:	  Last window counter sent
 * @tx_t_last_win_count:  Timestamp of earliest packet
 *			  with last_win_count value sent
 * @tx_no_feedback_timer: Handle to no feedback timer
 * @tx_t_ld:		  Time last doubled during slow start
 * @tx_t_nom:		  Nominal send time of next packet
 * @tx_hist:		  Packet history
 * @tx_freeze_seqno:	  Last sequence when the freeze state has been changed
 * @tx_freeze_state:	  Freeze-TFRC state
 * @tx_freeze_x_recv:	  Xrecv before Freezing
 */
struct ccid3_hc_tx_sock {
	u64				tx_x;
	u64				tx_x_recv;
	u32				tx_x_calc;
	u32				tx_rtt;
	u16				tx_r_sqmean;
	u32				tx_p;
	u32				tx_t_rto;
	u32				tx_t_ipi;
	u16				tx_s;
	bool				tx_feedback:1;
	u8				tx_last_win_count;
	ktime_t				tx_t_last_win_count;
	struct timer_list		tx_no_feedback_timer;
	ktime_t				tx_t_ld;
	ktime_t				tx_t_nom;
	struct tfrc_tx_hist_entry	*tx_hist;
#ifdef	CONFIG_IP_DCCP_CCID3_FREEZE
	enum ccid3_hc_tx_freeze_states	tx_freeze_state;
	u64				tx_freeze_seqno;
	u64				tx_freeze_x_recv;
	u32				tx_freeze_p;
#endif
};

static inline struct ccid3_hc_tx_sock *ccid3_hc_tx_sk(const struct sock *sk)
{
	struct ccid3_hc_tx_sock *hctx = ccid_priv(dccp_sk(sk)->dccps_hc_tx_ccid);
	BUG_ON(hctx == NULL);
	return hctx;
}


enum ccid3_fback_type {
	CCID3_FBACK_NONE = 0,
	CCID3_FBACK_INITIAL,
	CCID3_FBACK_PERIODIC,
	CCID3_FBACK_PARAM_CHANGE
};

#ifdef CONFIG_IP_DCCP_CCID3_FREEZE
/* Freeze-TFRC receiver states */
enum ccid3_hc_rx_freeze_states {
	TFRC_FREEZE_RSTATE_NORMAL = 0,
	TFRC_FREEZE_RSTATE_SIGNAL_FREEZE,
	TFRC_FREEZE_RSTATE_SIGNAL_UNFREEZE,
	TFRC_FREEZE_RSTATE_RESTORATION,
	TFRC_FREEZE_RSTATE_RECOVERY1,
	TFRC_FREEZE_RSTATE_RECOVERY2,
	TFRC_FREEZE_RSTATE_PROBED,
};
#endif

/**
 * struct ccid3_hc_rx_sock - CCID3 receiver half-connection socket
 * @rx_last_counter:	     Tracks window counter (RFC 4342, 8.1)
 * @rx_feedback:	     The type of the feedback last sent
 * @rx_x_recv:		     Receiver estimate of send rate (RFC 3448, sec. 4.3)
 * @rx_tstamp_last_feedback: Time at which last feedback was sent
 * @rx_hist:		     Packet history (loss detection + RTT sampling)
 * @rx_li_hist:		     Loss Interval database
 * @rx_pinv:		     Inverse of Loss Event Rate (RFC 4342, sec. 8.5)
 * @rx_freeze_options:       Freeze-TFRC-specific options received
 * @tx_freeze_seqno:	     Last sequence when the freeze state has been changed
 * @rx_freeze_state:	     Freeze-TFRC state
 * @rx_freeze_restoration_end: Time at which restoration should end
 */
struct ccid3_hc_rx_sock {
	u8				rx_last_counter:4;
	enum ccid3_fback_type		rx_feedback:4;
	u32				rx_x_recv;
	ktime_t				rx_tstamp_last_feedback;
	struct tfrc_rx_hist		rx_hist;
	struct tfrc_loss_hist		rx_li_hist;
#define rx_pinv				rx_li_hist.i_mean
#ifdef	CONFIG_IP_DCCP_CCID3_FREEZE
	struct ccid3_hc_rx_freeze_options rx_freeze_options;
	u64				rx_freeze_seqno;
	enum ccid3_hc_rx_freeze_states	rx_freeze_state;
	ktime_t 			rx_freeze_restoration_end;
#endif
};

static inline struct ccid3_hc_rx_sock *ccid3_hc_rx_sk(const struct sock *sk)
{
	struct ccid3_hc_rx_sock *hcrx = ccid_priv(dccp_sk(sk)->dccps_hc_rx_ccid);
	BUG_ON(hcrx == NULL);
	return hcrx;
}

#ifdef CONFIG_IP_DCCP_CCID3_FREEZE
int ccid3_rx_freeze(struct ccid3_hc_rx_sock *hcrx, u64 seqno);
int ccid3_rx_unfreeze(struct ccid3_hc_rx_sock *hcrx, u64 seqno);
int ccid3_rx_sender_restoring(struct ccid3_hc_rx_sock *hcrx, u64 seqno);
int ccid3_rx_check_restoration_finished(struct ccid3_hc_rx_sock *hcrx, u64 seqno);
int ccid3_rx_sender_probing(struct ccid3_hc_rx_sock *hcrx, u64 seqno);
int ccid3_rx_end_probing(struct ccid3_hc_rx_sock *hcrx, u64 seqno);

static inline void ccid3_rx_freeze_set(struct ccid3_hc_rx_sock *hcrx,
		enum ccid3_hc_rx_freeze_states val, u64 seqno) {
	ccid3_freeze_pr_debug("changing to mode %d", val);
	hcrx->rx_freeze_state = val;
	hcrx->rx_freeze_seqno = seqno;
}
static inline enum ccid3_hc_rx_freeze_states ccid3_rx_freeze_get(const struct ccid3_hc_rx_sock *hcrx)
{
	return hcrx->rx_freeze_state;
}

int ccid3_tx_freeze(struct ccid3_hc_tx_sock *hctx, u64 seqno);
int ccid3_tx_unfreeze(struct ccid3_hc_tx_sock *hctx, u64 seqno);
int ccid3_tx_receiver_unfrozen(struct ccid3_hc_tx_sock *hctx, u64 seqno);

static inline void ccid3_tx_freeze_set(struct ccid3_hc_tx_sock *hctx,
		enum ccid3_hc_tx_freeze_states val, u64 seqno){
	hctx->tx_freeze_state = val;
	hctx->tx_freeze_seqno = seqno;
}
static inline enum ccid3_hc_tx_freeze_states ccid3_tx_freeze_get(const struct ccid3_hc_tx_sock *hctx)
{
	return hctx->tx_freeze_state;
}
#endif

#endif /* _DCCP_CCID3_H_ */
