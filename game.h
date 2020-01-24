#pragma once

enum WEARABLE_ITEM
{
	NONE,
	LEFT_HAND, // weappon(1)
	RIGHT_HAND, // shield(1)
	HEAD, // helm(1)
	FINGER, // ring(10)
	WRIST,  // bracelet(2)
	NECK,   // necklace(1)
	BODY, // armor(1)
	BODY_EXT, // cloak(1)
	FEET, // shoes(1)
	PALMS, // globes(1)
};

struct Item
{
	ITEM_KIND kind;
	char name[16];

	Item* next;
	Item* prev;

	bool consumable;
	// effects when consumed
	int effect_add_cur_hp;
	int effect_add_cur_mp;
	
	WEARABLE_ITEM wearable;
	int visual_index;
	bool active;
	// effects when active
	int effect_add_max_hp;
	int effect_add_max_mp;

	// attack_speed, hit_propability
	// protection_rate, slowdown_rate

	Sprite* inventory_sprite;
	int inventory_sprite_pos[2];
	int inventory_sprite_size[2];

	// character's inventory
	int inventory_pos[2];
};

struct Character // player or humanlike
{
	// recolor?
	Sprite* sprite;

	int anim;
	int frame;
	float pos[3];
	float yaw;

	// inventory
	Item* first_item;
	Item* last_item;
	// inventory window state
	// ....

	// abilities & magic
	// ...

	// character
	int level;
	int max_hp;
	int max_mp;

	// current
	int cur_hp;
	int cur_mp;
};
