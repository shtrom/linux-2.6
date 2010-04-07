/*
 * Freeze-TCP support for Linux
 * Copyright (C) 2009 Nicta, Olivier Mehani <olivier.mehani@nicta.com.au>
 * 
 * net/ipv4/tcp_freeze.c
 *
 * Helper functions to manipulate the frozen flag of the TCP socket structure
 * and send the appropriate messages.
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
#include <linux/kernel.h>
#include <net/tcp.h>
#include <linux/tcp.h>


#ifdef CONFIG_TCP_FREEZE_DEBUG
# define DEBUG(...) printk(KERN_DEBUG __VA_ARGS__)
#else
# define DEBUG(...) /* debug statement */
#endif

int tcp_set_freeze(struct tcp_sock *tp, int val)
{
	DEBUG("TCP: %s(%p, %d)\n", __func__, tp, val);
	if (!val && tcp_get_freeze(tp)) {
	        DEBUG("TCP: Unfreezing socket...\n");
	        tp->frozen = 0;
	        /* Send ZWA */
	        tcp_send_ack((struct sock *) tp);
	} else if (val == 1 && !tcp_get_freeze(tp)) {
	        DEBUG("TCP: Freezing socket...\n");
	        tp->frozen = 1;
	        /* Send dupacks 
	         * TODO: configurable number of AVK via sysctl */
	        tcp_send_ack((struct sock *) tp);
	        tcp_send_ack((struct sock *) tp);
	} else {
	        DEBUG("TCP: Invalid command\n");
	        return -EINVAL;
	}

	return 0;
}

int tcp_get_freeze(struct tcp_sock *tp)
{
	/* DEBUG("TCP: %s(%p), %d\n", __func__, tp, tp->frozen); */
	return tp->frozen;
}

EXPORT_SYMBOL(tcp_set_freeze);
EXPORT_SYMBOL(tcp_get_freeze);
