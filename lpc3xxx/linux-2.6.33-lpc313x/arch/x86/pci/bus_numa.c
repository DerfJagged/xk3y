#include <linux/init.h>
#include <linux/pci.h>

#include "bus_numa.h"

int pci_root_num;
struct pci_root_info pci_root_info[PCI_ROOT_NR];
int found_all_numa_early;

void x86_pci_root_bus_res_quirks(struct pci_bus *b)
{
	int i;
	int j;
	struct pci_root_info *info;

	/* don't go for it if _CRS is used already */
	if (b->resource[0] != &ioport_resource ||
	    b->resource[1] != &iomem_resource)
		return;

	if (!pci_root_num)
		return;

	/* for amd, if only one root bus, don't need to do anything */
	if (pci_root_num < 2 && found_all_numa_early)
		return;

	for (i = 0; i < pci_root_num; i++) {
		if (pci_root_info[i].bus_min == b->number)
			break;
	}

	if (i == pci_root_num)
		return;

	printk(KERN_DEBUG "PCI: peer root bus %02x res updated from pci conf\n",
			b->number);

	info = &pci_root_info[i];
	for (j = 0; j < info->res_num; j++) {
		struct resource *res;
		struct resource *root;

		res = &info->res[j];
		b->resource[j] = res;
		if (res->flags & IORESOURCE_IO)
			root = &ioport_resource;
		else
			root = &iomem_resource;
		insert_resource(root, res);
	}
}

void __devinit update_res(struct pci_root_info *info, size_t start,
			      size_t end, unsigned long flags, int merge)
{
	int i;
	struct resource *res;

	if (start > end)
		return;

	if (!merge)
		goto addit;

	/* try to merge it with old one */
	for (i = 0; i < info->res_num; i++) {
		size_t final_start, final_end;
		size_t common_start, common_end;

		res = &info->res[i];
		if (res->flags != flags)
			continue;

		common_start = max((size_t)res->start, start);
		common_end = min((size_t)res->end, end);
		if (common_start > common_end + 1)
			continue;

		final_start = min((size_t)res->start, start);
		final_end = max((size_t)res->end, end);

		res->start = final_start;
		res->end = final_end;
		return;
	}

addit:

	/* need to add that */
	if (info->res_num >= RES_NUM)
		return;

	res = &info->res[info->res_num];
	res->name = info->name;
	res->flags = flags;
	res->start = start;
	res->end = end;
	res->child = NULL;
	info->res_num++;
}
