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

int tcp_freeze_status_global = 0;

EXPORT_SYMBOL(tcp_freeze_status_global);

#ifdef CONFIG_TCP_FREEZE_DEBUG
# define DEBUG(...) printk(KERN_DEBUG __VA_ARGS__)
#else
# define DEBUG(...) /* debug statement */
#endif

struct tcp_sock_entry
{
	struct tcp_sock *tp;
	struct tcp_sock_entry *next;
	struct tcp_sock_entry *prev;
};


struct tcp_freeze_list
{
	struct tcp_sock_entry *head;
	int size;
};

void init_tcp_freeze_list(struct tcp_freeze_list *tflist)
{
	tflist->head = NULL;
	tflist->size = 0;
}

struct tcp_freeze_list  gtcp_freeze_list = { NULL, 0 };

void add_to_tcp_freeze_list(struct tcp_freeze_list *tflist, struct tcp_sock *tp)
{
	struct tcp_sock_entry *newtse = (struct tcp_sock_entry *) kmalloc(sizeof(struct tcp_sock_entry), GFP_KERNEL);
	if(!newtse)
	{
		printk(KERN_ERR "%s(): failed to allocate memory for tcp_sock_entry\n", __func__);
		return ;
	}

	newtse->tp = tp;

	if(tflist->size ==0)
	{

		newtse->prev = NULL;
		newtse->next = NULL;
		tflist->head = newtse;

		tflist->size = 1;
	}
	else
	{
		struct tcp_sock_entry *tmp = tflist->head ;
		while(tmp && tmp->next) tmp = tmp->next;
		newtse->next =  NULL;
		newtse->prev = tmp;;
		tmp->next = newtse;
		tflist->size++;
	}
}

int rem_tail_from_tcp_freeze_list(struct tcp_freeze_list *tflist) /* Ugly */
{
		struct tcp_sock_entry *tmp = tflist->head;

		if(!tmp || !tflist->size)
			return -1;

		while(tmp && tmp->next)
			tmp = tmp->next;

		if(tmp && tmp->prev)
			tmp->prev->next = NULL;

		if (tflist->size <= 1)
			tflist->head=NULL;

		tflist->size--;

		if(tmp)
			kfree(tmp);

		return 0;
}

void clear_tcp_freeze_list(struct tcp_freeze_list *tflist)
{
     while(tflist->head && tflist->size)
     {
	    if( rem_tail_from_tcp_freeze_list(tflist) == -1 )
		break;
     }

     if( tflist->size !=0 )
	    printk(KERN_ERR "%s(): tflist->size !=0, size = %d\n",
			    __func__,
			    tflist->size);
}

void unfreeze_all_tcp_socks(struct tcp_freeze_list *tflist)
{
	int count =0;
	struct tcp_sock_entry *tmp;
	tmp = tflist->head ;

	DEBUG("FreezeTCP::unfreeze_all_tcp_socks(): frozen socket list size = %d \n", tflist->size);
	while(tmp && (count < tflist->size))
	{
		if(tmp->tp)
		tcp_set_freeze(tmp->tp, 0);
		tmp = tmp->next;
		count++;
	}

}


int tcp_set_freeze(struct tcp_sock *tp, int val)
{
	DEBUG("TCP: %s(%p, %d)\n", __func__, tp, val);
	if (!val && tcp_get_freeze(tp)) {
	        DEBUG("TCP: Unfreezing socket...\n");
	        tp->frozen = 0;
	        /* Send ZWA */
	        tcp_send_ack((struct sock *) tp);
		tcp_send_ack((struct sock *) tp);
		tcp_send_ack((struct sock *) tp);
	} else if (val == 1 && !tcp_get_freeze(tp)) {
	        DEBUG("TCP: Freezing socket...\n");
	        tp->frozen = 1;
	        /* Send dupacks 
	         * TODO: configurable number of AVK via sysctl */
	        tcp_send_ack((struct sock *) tp);
	        tcp_send_ack((struct sock *) tp);
		add_to_tcp_freeze_list(&gtcp_freeze_list, tp);
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

int tcp_set_global_freeze_status(const char *val)
{
	tcp_freeze_status_global  = (int)simple_strtol(val,NULL,10);
	DEBUG("%s(): global_tcp_freeze_status set to %d\n", __func__,
			tcp_freeze_status_global);
	unfreeze_all_tcp_socks(&gtcp_freeze_list);
	clear_tcp_freeze_list(&gtcp_freeze_list);
	return 0;
}

void tcp_get_global_freeze_status(char *val)
{
	strncpy(val, (char *)(&tcp_freeze_status_global),
			sizeof(tcp_freeze_status_global));
}


EXPORT_SYMBOL(tcp_set_freeze);
EXPORT_SYMBOL(tcp_get_freeze);
EXPORT_SYMBOL(tcp_set_global_freeze_status)
EXPORT_SYMBOL(tcp_get_global_freeze_status)
