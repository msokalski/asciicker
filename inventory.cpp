
#include "game.h"
#include <string.h>

extern World* world;
const ItemProto* item_proto_lib;

Item* CreateItem()
{
	Item* item = (Item*)malloc(sizeof(Item));
	memset(item, 0, sizeof(Item));
	return item;
}

void DestroyItem(Item* item)
{
	if (item->inst)
		DeleteInst(item->inst);
	free(item);
}


bool Inventory::InsertItem(Item* item, int xy[2])
{
	assert(item && item->inst && item->purpose == Item::WORLD);
	if (my_items < max_items)
	{
		DeleteInst(item->inst);

		item->inst = 0;
		item->purpose = Item::OWNED;

		my_item[my_items].xy[0] = xy[0];
		my_item[my_items].xy[1] = xy[1];
		my_item[my_items].item = item;
		my_item[my_items].in_use = false;


		// set bitmask
		int x0 = my_item[my_items].xy[0];
		int x1 = x0 + (item->proto->sprite_2d->atlas->width + 1) / 4;
		int y0 = my_item[my_items].xy[1];
		int y1 = y0 + (item->proto->sprite_2d->atlas->height + 1) / 4;
		for (int y = y0; y < y1; y++)
		{
			for (int x = x0; x < x1; x++)
			{
				int i = x + y * width;
				bitmask[i >> 3] |= 1 << (i & 7);
			}
		}

		my_items++;
		focus = my_items - 1;
	}

	return true;
}

bool Inventory::RemoveItem(int index, float pos[3], float yaw)
{
	assert(index >= 0 && index < my_items && my_item[index].item &&
		my_item[index].item->purpose == Item::OWNED && !my_item[index].item->inst);

	int flags = INST_USE_TREE | INST_VISIBLE;

	Item* item = my_item[index].item;

	// clear bitmask
	int x0 = my_item[index].xy[0];
	int x1 = x0 + (item->proto->sprite_2d->atlas->width + 1) / 4;
	int y0 = my_item[index].xy[1];
	int y1 = y0 + (item->proto->sprite_2d->atlas->height + 1) / 4;
	for (int y = y0; y < y1; y++)
	{
		for (int x = x0; x < x1; x++)
		{
			int i = x + y * width;
			bitmask[i >> 3] &= ~(1 << (i&7));
		}
	}

	if (pos)
	{
		item->purpose = Item::WORLD;
		item->inst = CreateInst(world, item, flags, pos, yaw);
		assert(item->inst);
	}
	else
	{
		DestroyItem(item);
	}

	my_items--; // fill gap, preserve order!

	if (focus > index || focus == my_items)
		focus--;

	for (int i = index; i < my_items; i++)
		my_item[i] = my_item[i + 1];
	my_item[my_items].item = 0;

	return true;
}