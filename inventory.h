#pragma once

struct ItemProto;
struct Item;

struct Inventory
{
	// max inventory dims as 4x4 cells blocks (incl. 1 border)
	static const int width = 8;   // fit upto 4 7x7 cells items
	static const int height = 20; // fit upto 10 7x7 cells items

	Item* first;
	Item* last;

	int weight; // sum(Item* i=first..last) { i.count * i.proto->weight }
	int area; // sum(Item* i=first..last) { (i.proto->sprite_2d.width+1)*(i.proto->sprite_2d.height+1) / 16 }

	void Pack();
	bool FindFree(int dw, int dh, int xy[2]) const;

	bool Move(Item* item, const int to_xy[2]);

	// items must be first inserted to world (by LoadWorld)
	// then they can be moved to inventory and back to world (per game object separately)

};

struct ItemProto // loaded from items.txt file
{
	int kind; // 'W'eapon, 'S'hield, 'H'elmet, 'A'rmor, 'R'ing, ... 'C'onsumable
	int sub_kind; // (ie: for kind=='W' sub_kind==1 is sword, for kind=='C' sub_kind==1 is hp_potion )

	// use command:
	// will check for kind=W/S/H/A and we have player-sprite with given sub_kind (currently only 0/1)
	// other wearables like 'R'ing 'B'race 'N'ecklace are just accepted (no player-sprite is required)
	// for consumables 'C' it will destroy item

	int weight;

	// pointer to pointer, so we can have const static ItemProto array and load sprites later
	Sprite* sprite_3d;
	Sprite* sprite_2d; // if null item cannot be picked up

	const char* desc;

	// extras ?
	// ...
};

struct Item
{
	ItemProto* proto;

	Item* prev;
	Item* next;

	int count;
	int xy[2];
};

enum PLAYER_WEAPON_INDEX
{
	WEAPON_NONE = 0,
	SWORD,
	// --------
	MACE,
	HAMMER, // big ace too
	AXE, // only lumber
	CROSSBOW,
	FLAIL,
};

enum PLAYER_SHIELD_INDEX
{
	SHIELD_NONE = 0,
	SHIELD_NORMAL,
};

enum PLAYER_HELMET_INDEX
{
	HELMET_NONE = 0,
	// -----------
	HELMET_NORMAL,
};

enum PLAYER_ARMOR_INDEX
{
	ARMOR_NONE = 0,
	// -----------
	ARMOR_NORMAL,
};

enum PLAYER_FOOD_INDEX
{
	FOOD_NONE = 0,
	MEAT,
	EGG,
	CHEESE,
	BREAD,
	BEET,
	CUCUMBER,
	CARROT,
	APPLE,
	CHERRY,
	PLUM,
};

enum PLAYER_DRINK_INDEX
{
	DRINK_NONE = 0,
	MILK,
	WATER,
	WINE
};

enum PLAYER_POTION_INDEX
{
	POTION_NONE = 0,
	RED,  // HP
	BLUE, // MP
	GREEN,
	PINK,
	CYAN,
	GOLD,
	GREY
};

enum PLAYER_RING_INDEX
{
	RING_WHITE,
	RING_CYAN,
	RING_GOLD,
	RING_PINK
};