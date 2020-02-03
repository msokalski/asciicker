#pragma once

#include "physics.h"
#include "render.h"
#include "sprite.h"
#include "mesh.h"
#include "terrain.h"

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

enum ITEM_KIND
{
	OTHER
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

struct Character
{
	// recolor?
	Sprite* sprite;

	int anim;
	int frame;
	float pos[3];
	float dir;

	int max_hp;
	int cur_hp;
};

struct Human : Character
{
	bool wolf_mount;

	// inventory
	Item* first_item;
	Item* last_item;
	// inventory window state
	// ....

	// abilities & magic
	// ...

	// character
	int max_hp;
	int max_mp;

	// current
	int cur_hp;
	int cur_mp;
};

struct Game
{
	// terrain & world are global
	// World* world;
	// Terrain* terrain;

	uint64_t stamp;

	int font_size[2];
	int render_size[2];

	bool show_keyb;
	int keyb_cap[11]; // cap down by 10 touches and mouse
	int keyb_pos[2];
	int keyb_mul;

	int water;
	float prev_yaw;

	Renderer* renderer;
	Physics* physics;

	Human player;

	int npcs;
	Character* npc;

	// accumulated input state
	struct Input 
	{
		uint8_t key[32]; // keyb state
		uint8_t but; // mouse buttons
		int wheel;   // relative mouse wheel
		int pos[2];  // mouse pos
		int size[2]; // window size (in pixels)
		int drag;    // which button initiated drag and is still down since then
		int drag_from[2]; // where drag has started
		bool jump;
		bool shot;

		struct Touch
		{
			int pos[2];
			bool contact;
		};

		Touch touch[10]; // [0] is primary touch

		bool IsKeyDown(int k)
		{
			return (key[k >> 3] & (1 << (k & 7))) != 0;
		}

		void ClearKey(int k)
		{
			key[k >> 3] &= ~(1 << (k & 7));
		}
	};

	Input input;

	enum GAME_KEYB
	{
		KEYB_DOWN,
		KEYB_UP,
		KEYB_CHAR,
		KEYB_PRESS, // non-char terminal input with modifiers
	};

	enum GAME_MOUSE
	{
		MOUSE_MOVE,
		MOUSE_LEFT_BUT_DOWN,
		MOUSE_LEFT_BUT_UP,
		MOUSE_RIGHT_BUT_DOWN,
		MOUSE_RIGHT_BUT_UP,
		MOUSE_WHEEL_DOWN,
		MOUSE_WHEEL_UP
	};

	enum GAME_TOUCH
	{
		TOUCH_MOVE,
		TOUCH_BEGIN,
		TOUCH_END
	};

	// just accumulates input
	void OnKeyb(GAME_KEYB keyb, int key);
	void OnMouse(GAME_MOUSE mouse, int x, int y);
	void OnTouch(GAME_TOUCH touch, int id, int x, int y);
	void OnFocus(bool set);
	void OnSize(int w, int h, int fw, int fh);

	// update physics with accumulated input then render state to output
	void Render(uint64_t _stamp, AnsiCell* ptr, int width, int height);
	void ScreenToCell(int p[2]) const;
};

Game* CreateGame(int water, float pos[3], float yaw, float dir, uint64_t stamp);
void DeleteGame(Game* g);

void LoadSprites();
void FreeSprites();
