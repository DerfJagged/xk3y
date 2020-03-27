/*
 * Copyright (C) 2007-2009 B.A.T.M.A.N. contributors:
 *
 * Marek Lindner, Simon Wunderlich
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 *
 */

#include "main.h"
#include "hard-interface.h"
#include "log.h"
#include "soft-interface.h"
#include "send.h"
#include "translation-table.h"
#include "routing.h"
#include "hash.h"
#include "compat.h"

#define MIN(x, y) ((x) < (y) ? (x) : (y))

static char avail_ifs;
static char active_ifs;

static void hardif_free_interface(struct rcu_head *rcu);

static struct batman_if *get_batman_if_by_name(char *name)
{
	struct batman_if *batman_if;

	rcu_read_lock();
	list_for_each_entry_rcu(batman_if, &if_list, list) {
		if (strncmp(batman_if->dev, name, IFNAMSIZ) == 0)
			goto out;
	}

	batman_if = NULL;

out:
	rcu_read_unlock();
	return batman_if;
}

int hardif_min_mtu(void)
{
	struct batman_if *batman_if;
	/* allow big frames if all devices are capable to do so
	 * (have MTU > 1500 + BAT_HEADER_LEN) */
	int min_mtu = ETH_DATA_LEN;

	rcu_read_lock();
	list_for_each_entry_rcu(batman_if, &if_list, list) {
		if ((batman_if->if_active == IF_ACTIVE) ||
		    (batman_if->if_active == IF_TO_BE_ACTIVATED))
			min_mtu = MIN(batman_if->net_dev->mtu - BAT_HEADER_LEN,
				      min_mtu);
	}
	rcu_read_unlock();

	return min_mtu;
}

static void check_known_mac_addr(uint8_t *addr)
{
	struct batman_if *batman_if;
	char mac_string[ETH_STR_LEN];

	rcu_read_lock();
	list_for_each_entry_rcu(batman_if, &if_list, list) {
		if ((batman_if->if_active != IF_ACTIVE) &&
		    (batman_if->if_active != IF_TO_BE_ACTIVATED))
			continue;

		if (!compare_orig(batman_if->net_dev->dev_addr, addr))
			continue;

		addr_to_string(mac_string, addr);
		debug_log(LOG_TYPE_WARN, "The newly added mac address (%s) already exists on: %s\n",
		          mac_string, batman_if->dev);
		debug_log(LOG_TYPE_WARN, "It is strongly recommended to keep mac addresses unique to avoid problems!\n");
	}
	rcu_read_unlock();
}

/* adjusts the MTU if a new interface with a smaller MTU appeared. */
void update_min_mtu(void)
{
	int min_mtu;

	min_mtu = hardif_min_mtu();
	if (soft_device->mtu != min_mtu)
		soft_device->mtu = min_mtu;
}

/* checks if the interface is up. (returns 1 if it is) */
static int hardif_is_interface_up(char *dev)
{
	struct net_device *net_dev;

	/**
	 * if we already have an interface in our interface list and
	 * the current interface is not the primary interface and
	 * the primary interface is not up and
	 * the primary interface has never been up - don't activate any
	 * secondary interface !
	 */

	rcu_read_lock();
	if ((!list_empty(&if_list)) &&
	    strncmp(((struct batman_if *)if_list.next)->dev, dev, IFNAMSIZ) &&
	    !(((struct batman_if *)if_list.next)->if_active == IF_ACTIVE) &&
	    !(((struct batman_if *)if_list.next)->if_active == IF_TO_BE_ACTIVATED) &&
	    (!main_if_was_up())) {
		rcu_read_unlock();
		goto end;
	}
	rcu_read_unlock();

#ifdef __NET_NET_NAMESPACE_H
	net_dev = dev_get_by_name(&init_net, dev);
#else
	net_dev = dev_get_by_name(dev);
#endif
	if (!net_dev)
		goto end;

	if (!(net_dev->flags & IFF_UP))
		goto failure;

	dev_put(net_dev);
	return 1;

failure:
	dev_put(net_dev);
end:
	return 0;
}

/* deactivates the interface. */
void hardif_deactivate_interface(struct batman_if *batman_if)
{
	if (batman_if->if_active != IF_ACTIVE)
		return;

	if (batman_if->raw_sock)
		sock_release(batman_if->raw_sock);

	/**
	 * batman_if->net_dev has been acquired by dev_get_by_name() in
	 * proc_interfaces_write() and has to be unreferenced.
	 */

	if (batman_if->net_dev)
		dev_put(batman_if->net_dev);

	batman_if->raw_sock = NULL;
	batman_if->net_dev = NULL;

	batman_if->if_active = IF_INACTIVE;
	active_ifs--;

	debug_log(LOG_TYPE_NOTICE, "Interface deactivated: %s\n",
	          batman_if->dev);
}

/* (re)activate given interface. */
static void hardif_activate_interface(struct batman_if *batman_if)
{
	struct sockaddr_ll bind_addr;
	int retval;

	if (batman_if->if_active != IF_INACTIVE)
		return;

#ifdef __NET_NET_NAMESPACE_H
	batman_if->net_dev = dev_get_by_name(&init_net, batman_if->dev);
#else
	batman_if->net_dev = dev_get_by_name(batman_if->dev);
#endif
	if (!batman_if->net_dev)
		goto dev_err;

	retval = sock_create_kern(PF_PACKET, SOCK_RAW,
				  __constant_htons(ETH_P_BATMAN),
				  &batman_if->raw_sock);

	if (retval < 0) {
		debug_log(LOG_TYPE_WARN, "Can't create raw socket: %i\n",
			  retval);
		goto sock_err;
	}

	bind_addr.sll_family = AF_PACKET;
	bind_addr.sll_ifindex = batman_if->net_dev->ifindex;
	bind_addr.sll_protocol = 0;	/* is set by the kernel */

	retval = kernel_bind(batman_if->raw_sock,
			     (struct sockaddr *)&bind_addr, sizeof(bind_addr));

	if (retval < 0) {
		debug_log(LOG_TYPE_WARN, "Can't create bind raw socket: %i\n",
			  retval);
		goto bind_err;
	}

	check_known_mac_addr(batman_if->net_dev->dev_addr);

	batman_if->raw_sock->sk->sk_user_data =
		batman_if->raw_sock->sk->sk_data_ready;
	batman_if->raw_sock->sk->sk_data_ready = batman_data_ready;

	addr_to_string(batman_if->addr_str, batman_if->net_dev->dev_addr);

	memcpy(((struct batman_packet *)(batman_if->packet_buff))->orig,
	       batman_if->net_dev->dev_addr, ETH_ALEN);
	memcpy(((struct batman_packet *)(batman_if->packet_buff))->prev_sender,
	       batman_if->net_dev->dev_addr, ETH_ALEN);

	batman_if->if_active = IF_TO_BE_ACTIVATED;
	active_ifs++;

	/* save the mac address if it is our primary interface */
	if (batman_if->if_num == 0)
		set_main_if_addr(batman_if->net_dev->dev_addr);

	debug_log(LOG_TYPE_NOTICE, "Interface activated: %s\n",
	          batman_if->dev);

	return;

bind_err:
	sock_release(batman_if->raw_sock);
sock_err:
	dev_put(batman_if->net_dev);
dev_err:
	batman_if->raw_sock = NULL;
	batman_if->net_dev = NULL;
}

static void hardif_free_interface(struct rcu_head *rcu)
{
	struct batman_if *batman_if = container_of(rcu, struct batman_if, rcu);

	kfree(batman_if->packet_buff);
	kfree(batman_if->dev);
	kfree(batman_if);
}

/**
 * called by
 *  - echo '' > /proc/.../interfaces
 *  - modprobe -r batman-adv-core
 */
/* removes and frees all interfaces */
void hardif_remove_interfaces(void)
{
	struct batman_if *batman_if = NULL;

	avail_ifs = 0;

	/* no lock needed - we don't delete somewhere else */
	list_for_each_entry(batman_if, &if_list, list) {

		list_del_rcu(&batman_if->list);

		/* first deactivate interface */
		if (batman_if->if_active != IF_INACTIVE)
			hardif_deactivate_interface(batman_if);

		call_rcu(&batman_if->rcu, hardif_free_interface);
	}
}

static int resize_orig(struct orig_node *orig_node, int if_num)
{
	void *data_ptr;

	data_ptr = kmalloc((if_num + 1) * sizeof(TYPE_OF_WORD) * NUM_WORDS,
			   GFP_ATOMIC);
	if (!data_ptr) {
		debug_log(LOG_TYPE_WARN, "Can't resize orig: out of memory\n");
		return -1;
	}

	memcpy(data_ptr, orig_node->bcast_own,
	       if_num * sizeof(TYPE_OF_WORD) * NUM_WORDS);
	kfree(orig_node->bcast_own);
	orig_node->bcast_own = data_ptr;

	data_ptr = kmalloc((if_num + 1) * sizeof(uint8_t), GFP_ATOMIC);
	if (!data_ptr) {
		debug_log(LOG_TYPE_WARN, "Can't resize orig: out of memory\n");
		return -1;
	}

	memcpy(data_ptr, orig_node->bcast_own_sum, if_num * sizeof(uint8_t));
	kfree(orig_node->bcast_own_sum);
	orig_node->bcast_own_sum = data_ptr;

	return 0;
}


/* adds an interface the interface list and activate it, if possible */
int hardif_add_interface(char *dev, int if_num)
{
	struct batman_if *batman_if;
	struct batman_packet *batman_packet;
	struct orig_node *orig_node;
	struct hash_it_t *hashit = NULL;

	batman_if = kmalloc(sizeof(struct batman_if), GFP_KERNEL);

	if (!batman_if) {
		debug_log(LOG_TYPE_WARN, "Can't add interface (%s): out of memory\n", dev);
		return -1;
	}

	batman_if->raw_sock = NULL;
	batman_if->net_dev = NULL;

	if ((if_num == 0) && (num_hna > 0))
		batman_if->packet_len = BAT_PACKET_LEN + num_hna * ETH_ALEN;
	else
		batman_if->packet_len = BAT_PACKET_LEN;

	batman_if->packet_buff = kmalloc(batman_if->packet_len, GFP_KERNEL);

	if (!batman_if->packet_buff) {
		debug_log(LOG_TYPE_WARN, "Can't add interface packet (%s): out of memory\n", dev);
		goto out;
	}

	batman_if->if_num = if_num;
	batman_if->dev = dev;
	batman_if->if_active = IF_INACTIVE;
	INIT_RCU_HEAD(&batman_if->rcu);

	debug_log(LOG_TYPE_NOTICE, "Adding interface: %s\n", dev);
	avail_ifs++;

	INIT_LIST_HEAD(&batman_if->list);

	batman_packet = (struct batman_packet *)(batman_if->packet_buff);
	batman_packet->packet_type = BAT_PACKET;
	batman_packet->version = COMPAT_VERSION;
	batman_packet->flags = 0x00;
	batman_packet->ttl = (batman_if->if_num > 0 ? 2 : TTL);
	batman_packet->flags = 0;
	batman_packet->tq = TQ_MAX_VALUE;
	batman_packet->num_hna = 0;

	if (batman_if->packet_len != BAT_PACKET_LEN) {
		unsigned char *hna_buff;
		int hna_len;

		hna_buff = batman_if->packet_buff + BAT_PACKET_LEN;
		hna_len = batman_if->packet_len - BAT_PACKET_LEN;
		batman_packet->num_hna = hna_local_fill_buffer(hna_buff,
							       hna_len);
	}

	atomic_set(&batman_if->seqno, 1);

	/* resize all orig nodes because orig_node->bcast_own(_sum) depend on
	 * if_num */
	spin_lock(&orig_hash_lock);

	while (NULL != (hashit = hash_iterate(orig_hash, hashit))) {
		orig_node = hashit->bucket->data;
		if (resize_orig(orig_node, if_num) == -1) {
			spin_unlock(&orig_hash_lock);
			goto out;
		}
	}

	spin_unlock(&orig_hash_lock);

	if (!hardif_is_interface_up(batman_if->dev))
		debug_log(LOG_TYPE_WARN, "Not using interface %s (retrying later): interface not active\n", batman_if->dev);
	else
		hardif_activate_interface(batman_if);

	list_add_tail_rcu(&batman_if->list, &if_list);

	/* begin sending originator messages on that interface */
	schedule_own_packet(batman_if);
	return 1;

out:
	if (batman_if->packet_buff)
		kfree(batman_if->packet_buff);
	kfree(batman_if);
	kfree(dev);
	return -1;
}

char hardif_get_active_if_num(void)
{
	return active_ifs;
}

static int hard_if_event(struct notifier_block *this,
                            unsigned long event, void *ptr)
{
	struct net_device *dev = (struct net_device *)ptr;
	struct batman_if *batman_if = get_batman_if_by_name(dev->name);

	if (!batman_if)
		goto out;

	switch (event) {
	case NETDEV_GOING_DOWN:
	case NETDEV_DOWN:
	case NETDEV_UNREGISTER:
		hardif_deactivate_interface(batman_if);
		break;
	case NETDEV_UP:
		hardif_activate_interface(batman_if);
		if ((atomic_read(&module_state) == MODULE_INACTIVE) &&
		    (hardif_get_active_if_num() > 0)) {
			activate_module();
		}
		break;
	/* NETDEV_CHANGEADDR - mac address change - what are we doing here ? */
	default:
		/* debug_log(LOG_TYPE_CRIT, "hard_if_event: %s %i\n", dev->name, event); */
		break;
	};

	update_min_mtu();

out:
	return NOTIFY_DONE;
}

struct notifier_block hard_if_notifier = {
        .notifier_call = hard_if_event,
};
