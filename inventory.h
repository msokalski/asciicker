#pragma once

struct ItemProto;
struct Item;

extern const ItemProto* item_proto_lib;

Item* CreateItem();
void DestroyItem(Item* item);

struct Inventory
{
	// max inventory dims as 4x4 cells blocks (incl. 1 border)
	static const int width = 8;   // fit upto 4 7x7 cells items
	static const int height = 20; // fit upto 10 7x7 cells items
	static const int max_items = 256;

	struct MyItem
	{
		Item* item;
		int xy[2];
		// some ui states if needed
		// ...
	};

	int my_items;
	MyItem my_item[max_items];

	bool InsertItem(Item* item, int xy[2]); 
	bool RemoveItem(int index, float pos[3], float yaw);

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
	const ItemProto* proto;

	Inst* inst;  // EDIT / WORLD : instance (OWNED has it NULL)

	int count;

	// item instances:
	// - if process is editor: private items for editor (saved/loaded with a3d file)
	// - items for players (just 1 clone from a3d items for all players execpt below)
	// - items created for each player inventory individually (if a3d item has seed flag)

	enum PURPOSE
	{
		// when created, item purpose is unspecified
		UNSPECIFIED = 0,

		// EDITOR / FILE only, 
		// render in editor only
		// note: loding world by game (without editor) will switch it immediately to WORLD
		//       loading by editor will make this item clone (with WORLD , available to test-players)
		// item must be attached to BHV (for editor)
		EDIT = 1,

		// game item, inside someone's inventory
		// dont't render in game (owner's inventory only)
		// item must be detached from BHV (travels with player) inst=0
		OWNED = 2,

		// game item, no owner, render it in game for all players
		// item must be attached to BHV (for players)
		WORLD = 3, 
	};

	// note: changing purpose may need adjusting World::insts counter!
	PURPOSE purpose;
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
	POTION_RED,  // HP
	POTION_BLUE, // MP
	POTION_GREEN,
	POTION_PINK,
	POTION_CYAN,
	POTION_GOLD,
	POTION_GREY
};

enum PLAYER_RING_INDEX
{
	RING_WHITE,
	RING_CYAN,
	RING_GOLD,
	RING_PINK
};