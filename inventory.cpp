
#include "game.h"

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


bool InsertItem(Item* item, const int xy[2]);
bool RemoveItem(int index, const float pos[3], float yaw);