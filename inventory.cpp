
#include "game.h"

Item* CreateItem(ItemProto* proto, size_t size)
{
	assert(size >= sizeof(Item) && proto);

	Item* item = (Item*)malloc(size);

	memset(item, 0, size);

	item->proto = proto;

	item->prev = 0;
	item->next = 0;

	return item;
}