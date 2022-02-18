
#include "game.h"
#include <string.h>

extern World* world;
extern Sprite* inventory_sprite;
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

void Inventory::UpdateLayout(int render_width, int render_height, int scene_shift, int bars_pos)
{
	int descent = bars_pos - 5;
	if (descent < 0)
		descent = 0;
	Sprite::Frame* sf = inventory_sprite->atlas;
	layout_width = 39;
	layout_max_height = 7 + 4*height+1 + 5;
	layout_height = render_height - 4 - descent;
	if (layout_height > layout_max_height)
		layout_height = layout_max_height;
	if (layout_height < sf->height)
		layout_height = sf->height;
	int diff = layout_height - sf->height;

	int dy = diff / 3;
	diff -= 3 * dy;

	for (int r = 0; r < 3; r++)
	{
		if (r < diff)
			layout_reps[r] = 2 + dy;
		else
			layout_reps[r] = 1 + dy;
	}

	layout_max_scroll = layout_max_height - layout_height;

	layout_x = scene_shift - layout_width;
	layout_y = (render_height - 4 - descent - layout_height) / 2;

	layout_frame[0] = layout_x + 3;
	layout_frame[1] = layout_y + 7;
	layout_frame[2] = layout_x + 3 + width * 4;
	layout_frame[3] = layout_y + layout_height - 6;
}

void Inventory::FocusNext(int dx, int dy)
{
	if (focus<0 || my_items<=1 || !dx && !dy || dx && dy)
		return;

	//int ccw_clip[2] = { dx + dy, dy - dx };
	//int cw_clip[2] = { dx - dy, dy + dx };

	int i = focus;
	Item* item = my_item[i].item;
	int x0 = my_item[i].xy[0];
	int x1 = x0 + (item->proto->sprite_2d->atlas->width + 1) / 4;
	int y0 = my_item[i].xy[1];
	int y1 = y0 + (item->proto->sprite_2d->atlas->height + 1) / 4;

	int fx0 = 2*x0;
	int fx1 = 2*x1;
	int fy0 = 2*y0;
	int fy1 = 2*y1;

	bool major_x = dx*dx>dy*dy;

	int fx,fy;
	if (major_x)
	{
		fx = 2*(dx>0 ? x1 : x0);
		fy = y0+y1;
	}
	else
	{
		fx = x0+x1;
		fy = 2*(dy>0 ? y1 : y0);
	}

	int best_e = 0xffff;
	int best_i = -1;

	for (int i = 0; i < my_items; i++)
	{
		if (i == focus)
			continue;

		Item* item = my_item[i].item;
		int x0 = my_item[i].xy[0];
		int x1 = x0 + (item->proto->sprite_2d->atlas->width + 1) / 4;
		int y0 = my_item[i].xy[1];
		int y1 = y0 + (item->proto->sprite_2d->atlas->height + 1) / 4;

		int cx,cy;
		int px,py;
		if (major_x)
		{
			if (fy < 2*y0)
				py = 2*y0;
			else
			if (fy > 2*y1)
				py = 2*y1;
			else
				py = fy;

			cx = px = 2*(dx>0 ? x0 : x1);
			cy = y0+y1;	

			if (cy<fy0)
				cy = fy0;
			if (cy>fy1)
				cy = fy1;
		}
		else
		{
			if (fx < 2*x0)
				px = 2*x0;
			else
			if (fx > 2*x1)
				px = 2*x1;
			else
				px = fx;
			
			cx = x0+x1;
			cy = py = 2*(dy>0 ? y0 : y1);

			if (cx<fx0)
				cx = fx0;
			if (cx>fx1)
				cx = fx1;
		}

		// from now coords are doubled, find_pos is already doubled!
		int vx = px - fx;
		int vy = py - fy;

		cx -= fx;
		cy -= fy;

		// int dot = cx * dx + cy * dy;
		// if (dot > 0)

		//int cw = vx * cw_clip[0] + vy * cw_clip[1];
		//int ccw = vx * ccw_clip[0] + vy * ccw_clip[1];

		// +/- 45 angle trimming
		//if (cw >=0 && ccw >= 0)

		if (vx * dx + vy * dy >=0)
		{
			int e = major_x ? vx * vx + 4 * vy * vy + cy * cy: 4 * vx * vx + vy * vy + cx * cx;

			if (e < best_e)
			{
				best_i = i;
				best_e = e;
			}
		}
	}

	if (best_i<0)
		return;

	animate_scroll = true;
	smooth_scroll = scroll;

	SetFocus(best_i);
}

void Inventory::SetFocus(int index)
{
	focus = index;
}

bool Inventory::InsertItem(Item* item, int xy[2])
{
	animate_scroll = true;
	smooth_scroll = scroll;

	if (my_items >= max_items)
		return true;

	if (item->purpose == Item::OWNED)
	{
		Character* h = (Character*)GetInstSpriteData(item->inst);
		ItemOwner* io = 0;
		if (h->req.kind == SpriteReq::HUMAN)
			io = (ItemOwner*)(NPC_Human*)h;
		else
			io = (ItemOwner*)(NPC_Creature*)h;

		// find it and transfer ownership
		for (int i = 0; i < io->items; i++)
		{
			if (io->has[i].item == item)
			{
				if (io->has[i].in_use)
				{
					assert(h->req.kind == SpriteReq::HUMAN);

					Human* hh = (Human*)h;
					// modify req (only humans)
					switch (item->proto->kind)
					{
						case 'W':
							hh->SetWeapon(0);
							break;
						case 'S':
							hh->SetShield(0);
							break;
						case 'H':
							hh->SetHelmet(0);
							break;
						case 'A':
							hh->SetArmor(0);
							break;
					}

					int reps[4] = { 0,0,0,0 };
					Sprite* sprite = GetSprite(&hh->req, hh->clr);
					UpdateSpriteInst(world, item->inst, sprite, hh->pos, hh->dir, hh->anim, hh->frame, reps);
				}

				item->inst = 0;

				my_item[my_items].story_id = io->has[i].story_id;
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

				// remove from corpse
				io->items--;
				for (; i < io->items; i++)
					io->has[i] = io->has[i + 1];

				return true;
			}
		}

		assert(0);
	}

	assert(item && item->inst && item->purpose == Item::WORLD);

	// here we should send 'pickitem' to server
	// it will validate if it was on our exclusive list
	// and will remove it from there and from world
	// it will also remember we own it

	my_item[my_items].story_id = GetInstStoryID(item->inst);
	my_item[my_items].xy[0] = xy[0];
	my_item[my_items].xy[1] = xy[1];
	my_item[my_items].item = item;
	my_item[my_items].in_use = false;

	DeleteInst(item->inst);

	item->inst = 0;
	item->purpose = Item::OWNED;

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

	return true;
}

bool Inventory::RemoveItem(int index, float pos[3], float yaw)
{
	assert(index >= 0 && index < my_items && my_item[index].item &&
		my_item[index].item->purpose == Item::OWNED && !my_item[index].item->inst);

	// here we should also send 'dropitem' to server
	// otherwise item won't show up (even for us) for picking up in the list
	// server removes it from our inventory and inserts it to the world
	// (till we disconnect)

	int flags = INST_USE_TREE | INST_VISIBLE | INST_VOLATILE;

	Item* item = my_item[index].item;

	int story_id = my_item[index].story_id;

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
		item->inst = CreateInst(world, item, flags, pos, yaw, story_id);
		assert(item->inst);

		// from flat to bvh
		AttachInst(world,item->inst);
	}
	else
	{
		if (story_id >= 0)
		{
			// eating story item !!!
			// review your story line design skills!
		}
		DestroyItem(item);
	}

	my_items--; // fill gap, preserve order!

	for (int i = index; i < my_items; i++)
		my_item[i] = my_item[i + 1];
	my_item[my_items].item = 0;

	// find closest item in y, if multiple in x?
	if (focus > index || focus == my_items)
		SetFocus(focus - 1);
	else
		SetFocus(focus);	

	animate_scroll = true;
	smooth_scroll = scroll;

	return true;
}