
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

void Inventory::FocusNext(int dx, int dy)
{
	if (focus<0 || my_items<=1 || !dx && !dy || dx && dy)
		return;

	int i = focus;
	Item* item = my_item[i].item;
	int x0 = my_item[i].xy[0];
	int x1 = x0 + (item->proto->sprite_2d->atlas->width + 1) / 4;
	int y0 = my_item[i].xy[1];
	int y1 = y0 + (item->proto->sprite_2d->atlas->height + 1) / 4;

	if (dx > 0)
	{
		int y[2] = { y0,y1 };
		int x = x1;

		int best_i = -1;
		int best_d = -1;

		for (int i = 0; i < my_items; i++)
		{
			if (i == focus)
				continue;
			Item* item = my_item[i].item;
			int x0 = my_item[i].xy[0];
			int x1 = x0 + (item->proto->sprite_2d->atlas->width + 1) / 4;
			int y0 = my_item[i].xy[1];
			int y1 = y0 + (item->proto->sprite_2d->atlas->height + 1) / 4;

			if (x1 > x)
			{

				int d;

				int c = y0 + y1 - (y[0] + y[1]);
				if (c < 0)
					c = -c;

				if (c < y1 - y0 + y[1] - y[0])
				{
					// overlapped projection: good
					d = (x0 - x)*(x0 - x);
				}
				else
				{
					// far corner
					if (y1 <= y[0])
						d = (x1 - x)*(x1 - x) + (y1 - y[0])*(y1 - y[0]);
					else
						d = (x1 - x)*(x1 - x) + (y0 - y[1])*(y0 - y[1]);
				}

				// add off-axis center displacement
				d *= 4;
				d += (y[0] + y[1] - (y0 + y1)) * (y[0] + y[1] - (y0 + y1));

				if (d < best_d || best_d < 0)
				{
					best_d = d;
					best_i = i;
				}
			}

			if (best_i >= 0)
			{
				SetFocus(best_i);
			}
		}
	}
	else
	if (dx < 0)
	{
		int y[2] = { y0,y1 };
		int x = x0;

		int best_i = -1;
		int best_d = -1;

		for (int i = 0; i < my_items; i++)
		{
			if (i == focus)
				continue;
			Item* item = my_item[i].item;
			int x0 = my_item[i].xy[0];
			int x1 = x0 + (item->proto->sprite_2d->atlas->width + 1) / 4;
			int y0 = my_item[i].xy[1];
			int y1 = y0 + (item->proto->sprite_2d->atlas->height + 1) / 4;

			if (x0 < x)
			{
				int d;

				int c = y0 + y1 - (y[0] + y[1]);
				if (c < 0)
					c = -c;

				if (c < y1-y0 + y[1]-y[0])
				{
					// overlapped projection: good
					d = (x0 - x)*(x0 - x);
				}
				else
				{
					// far corner
					if (y1 <= y[0])
						d = (x0 - x)*(x0 - x) + (y1 - y[0])*(y1 - y[0]);
					else
						d = (x0 - x)*(x0 - x) + (y0 - y[1])*(y0 - y[1]);
				}

				// add off-axis center displacement
				d *= 4;
				d += (y[0] + y[1] - (y0 + y1)) * (y[0] + y[1] - (y0 + y1));

				if (d < best_d || best_d < 0)
				{
					best_d = d;
					best_i = i;
				}
			}

			if (best_i >= 0)
			{
				SetFocus(best_i);
			}
		}
	}
	else
	if (dy > 0)
	{
		int x[2] = { x0,x1 };
		int y = y0;
	}
	else
	if (dy < 0)
	{
		int x[2] = { x0,x1 };
		int y = y1;
	}


	for (int i = 0; i < my_items; i++)
	{
		if (i == focus)
			continue;
		Item* item = my_item[i].item;
		int x0 = my_item[i].xy[0];
		int x1 = x0 + (item->proto->sprite_2d->atlas->width + 1) / 4;
		int y0 = my_item[i].xy[1];
		int y1 = y0 + (item->proto->sprite_2d->atlas->height + 1) / 4;

	}


#if 0
	int best_dot = -1;
	int index = -1;
	int num = 0;

	int ccw_clip[2] = { dx + dy, dy - dx };
	int cw_clip[2] = { dx - dy, dy + dx };

	for (int i = 0; i < my_items; i++)
	{
		if (i == focus)
			continue;
		Item* item = my_item[i].item;
		int x0 = my_item[i].xy[0];
		int x1 = x0 + (item->proto->sprite_2d->atlas->width + 1) / 4;
		int y0 = my_item[i].xy[1];
		int y1 = y0 + (item->proto->sprite_2d->atlas->height + 1) / 4;

		// from now coords are doubled, find_pos is already doubled!
		int cx = (x0 + x1) - find_pos[0];
		int cy = (y0 + y1) - find_pos[1];

		// int dot = cx * dx + cy * dy;
		// if (dot > 0)
		if (cx * dx + cy * dy > 0)
		{
			int dot = cx * cx + cy * cy;
			// +/- 45 angle trimming
			int cw = cx * cw_clip[0] + cy * cw_clip[1];
			int ccw = cx * ccw_clip[0] + cy * ccw_clip[1];

			if (cw < 0)
				dot += 8*cw*cw;
			if (ccw < 0)
				dot += 8*ccw*ccw;

			if (dot == best_dot)
				num++;
			else
			if (dot < best_dot || best_dot < 0)
			{
				num = 1;
				index = i;
				best_dot = dot;
			}
		}
	}

	if (!num)
		return;

	animate_scroll = true;
	smooth_scroll = scroll;


	if (num == 1)
	{
		SetFocus(index);
		if (!dx)
			find_pos[0] = push_pos[0];
		if (!dy)
			find_pos[1] = push_pos[1];

		return;
	}

	int best_tod = -1;
	index = -1;

	// for multiple objects find one with smallest abs( dot(center-find_pos,T(dir)) )
	for (int i = 0; i < my_items; i++)
	{
		if (i == focus)
			continue;

		Item* item = my_item[i].item;
		int x0 = my_item[i].xy[0];
		int x1 = x0 + (item->proto->sprite_2d->atlas->width + 1) / 4;
		int y0 = my_item[i].xy[1];
		int y1 = y0 + (item->proto->sprite_2d->atlas->height + 1) / 4;

		// from now coords are doubled, find_pos is already doubled!
		int cx = (x0 + x1) - find_pos[0];
		int cy = (y0 + y1) - find_pos[1];

		if (cx * dx + cy * dy > 0)
		{
			int dot = cx * cx + cy * cy;
			// +/- 45 angle trimming
			int cw = cx * cw_clip[0] + cy * cw_clip[1];
			int ccw = cx * ccw_clip[0] + cy * ccw_clip[1];

			if (cw < 0)
				dot += 8*cw*cw;
			if (ccw < 0)
				dot += 8*ccw*ccw;

			if (dot == best_dot)
			{
				int tod = cx * dy + cy * dx;
				if (tod < 0)
					tod = -tod;

				if (tod < best_tod || best_tod < 0)
				{
					index = i;
					best_tod = tod;
				}
			}
		}
	}

	SetFocus(index);

	if (!dx)
		find_pos[0] = push_pos[0];
	if (!dy)
		find_pos[1] = push_pos[1];
#endif
}

void Inventory::SetFocus(int index)
{
	if (index >= 0)
	{
		Item* item = my_item[index].item;
		int x0 = my_item[index].xy[0];
		int x1 = x0 + (item->proto->sprite_2d->atlas->width + 1) / 4;
		int y0 = my_item[index].xy[1];
		int y1 = y0 + (item->proto->sprite_2d->atlas->height + 1) / 4;
		find_pos[0] = (x0 + x1); // doubled!
		find_pos[1] = (y0 + y1); // doubled!
	}
	else
	{
		find_pos[0] = 0;
		find_pos[1] = 0;
	}

	focus = index;
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
		find_pos[0] = (x0 + x1); // doubled!
		find_pos[1] = (y0 + y1); // doubled!
	}

	animate_scroll = true;
	smooth_scroll = scroll;


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

	// find closest item in y, if multiple in x?
	if (focus > index || focus == my_items)
		SetFocus(focus - 1);
	else
		SetFocus(focus);

	for (int i = index; i < my_items; i++)
		my_item[i] = my_item[i + 1];
	my_item[my_items].item = 0;

	animate_scroll = true;
	smooth_scroll = scroll;

	return true;
}