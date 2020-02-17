
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

		my_items++;
	}

	return true;
}

bool Inventory::RemoveItem(int index, float pos[3], float yaw)
{
	assert(index >= 0 && index < my_items && my_item[index].item &&
		my_item[index].item->purpose == Item::OWNED && !my_item[index].item->inst);

	my_items--;
	if (index < my_items)
		my_item[index] = my_item[my_items];

	int flags = INST_USE_TREE | INST_VISIBLE;

	Item* item = my_item[index].item;
	item->purpose = Item::WORLD;
	item->inst = CreateInst(world, item, flags, pos, yaw);

	assert(item->inst);
	return true;
}