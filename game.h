#pragma once

#include "physics.h"
#include "render.h"
#include "sprite.h"
#include "world.h"
#include "terrain.h"

#include "inventory.h"
#include "network.h"

extern char player_name[];

struct SpriteReq
{
	enum KIND
	{
		HUMAN = 0,
		WOLF = 1
	};

	KIND kind;

	// only human
	int mount;
	int action;
	int armor;
	int helmet;
	int shield;
	int weapon;
};

struct Character
{
	// recolor?
	Sprite* sprite;

	int anim;
	int frame;
	float pos[3];
	float dir;

	uint64_t action_stamp;

	bool SetActionNone(uint64_t stamp);
	bool SetActionAttack(uint64_t stamp);
	bool SetActionFall(uint64_t stamp);
	bool SetActionStand(uint64_t stamp);
	bool SetActionDead(uint64_t stamp);

	Character* prev;
	Character* next;

	SpriteReq req; // kind of character is inside (human / wolf)!!!

	Inst* inst; // only server players
	int clr;
	int stuck;
	int around;
	float unstuck[2][3]; // [0]unstuck, [1]candid
	void* data; // npc physics
	Character* master;
	Character* target; // can be 0, master or any enemy
	bool jump; // helper if got stuck
	bool enemy; // buddy otherwise!
};

struct TalkBox;

Sprite* GetSprite(const SpriteReq* req, int clr = 0);

struct Human : Character
{
	char name[32];

	int level;

	int max_xp; // to next level = C1 * pow(C2,C3*level)
	int cur_xp; // 0..max_xp-1

	int pr; // 0-transparent <0-evil >0-good ...

	int max_hp;
	int cur_hp;

	int max_mp;
	int cur_mp;

	int max_speed;
	int cur_speed;

	int max_power;
	int cur_power;

	// 0-6
	int prot_hit;
	int prot_fire;
	int nutr_vits; 
	int nutr_mins;
	int nutr_prots;
	int nutr_fats;
	int nutr_carbs;
	int nutr_water;

	// -------------

	bool SetWeapon(int w);
	bool SetShield(int s);
	bool SetHelmet(int h);
	bool SetArmor(int a);
	bool SetMount(int m);

	TalkBox* talk_box;

	struct Talk
	{
		uint64_t stamp;
		TalkBox* box;
		float pos[3];
	};

	int talks;
	Talk talk[3];

	bool shooting;
	uint64_t shoot_stamp;
	float shoot_from[3];
	float shoot_to[3];
};

struct Server
{
	bool Proc(const uint8_t* ptr, int size); // called directly by JS (implemented in game.cpp)

	void Proc(); // does nothing on JS, native apps calls above func for all queued commands
	bool Send(const uint8_t* data, int size); // implemented by game_app/game_web

	void Log(const char* str);

	int max_clients;
	Human* others; // [max_clients]

	Human* head;
	Human* tail;

	uint64_t stamp;
	uint64_t last_lag;
	int lag_ms;
	bool lag_wait;

	// pose->pad with hold new/del/upd flags
};

extern Server* server; // global!

struct Game
{
	// terrain & world are global
	// World* world;
	// Terrain* terrain;

	uint64_t stamp;

	static const int fps_window_size = 100;
	int fps_window_pos;
	uint64_t fps_window[fps_window_size];

	int font_size[2];
	int render_size[2];

	bool perspective;

	//bool player_hit; // helper for detecting clicks on the player sprite
	bool show_keyb; // activated together with talk_box by clicking on character
	int keyb_hide;  // show / hide animator (vertical position)

	bool show_inventory;
	int scene_shift;
	bool show_buts; // true only if no popup is visible
	int bars_pos; // used to hide buts (0..7)

	// time relaxated KEY_UP/DOWN emulation by KEY_PRESSes
	uint64_t PressStamp;
	int PressKey;

	int TalkBox_blink;
	int KeybAutoRepCap;
	char KeybAutoRepChar;
	uint64_t KeybAuroRepDelayStamp;

	int keyb_pos[2];
	int keyb_mul;
	uint8_t keyb_key[32]; // simulated key presses by touch/mouse

	int water;
	float prev_yaw;
	float yaw_vel;

	Renderer* renderer;
	Physics* physics;

	Item** items_inrange;
	int items_count;
	int items_xarr[10];
	int items_ylo;
	int items_yhi;

	Human player;
	Inventory inventory;
	bool DropItem(int index);
	bool PickItem(Item* item);
	bool CheckDrop(int c/*contact index*/, int xy[2]=0, AnsiCell* ptr=0, int w=0, int h=0);
	int CheckPick(const int pos[2]);

	struct TalkMem
	{
		char buf[256];
		int len;
	};

	TalkMem talk_mem[4];

	Inst* player_inst;

	void CancelItemContacts();
	void ExecuteItem(int my_item);

	void StartContact(int id, int x, int y, int b);
	void MoveContact(int id, int x, int y);
	void EndContact(int id, int x, int y);

	int GetContact(int id);

	struct ConsumeAnim
	{
		int pos[2];
		Sprite* sprite;
		uint64_t stamp;
	};

	int consume_anims;
	ConsumeAnim consume_anim[16];

	// accumulated input state
	struct Input 
	{
		int last_hit_char;
		uint8_t key[32]; // keyb state

		// we split touch input to multiple separate mice with left button only
		struct Contact
		{
			enum
			{
				NONE,
				KEYBCAP,
				PLAYER,
				TORQUE, // can be abs (right mouse but) or (timer touch on margin)
				FORCE,

				ITEM_LIST_CLICK,
				ITEM_LIST_DRAG,
				ITEM_GRID_CLICK,
				ITEM_GRID_DRAG,
				ITEM_GRID_SCROLL,
			};

			int action;

			int drag;    // which button initiated drag and is still down since then OR ZERO if there is no contact!!!
			int pos[2];  // mouse pos
			int drag_from[2]; // where drag has started

			Item* item;
			int my_item;
			int keyb_cap; // if touch starts at some cap
			bool player_hit; // if touch started at player/talkbox
			int margin; // -1: if touch started at left margin, +1 : if touch started at right margin, 0 otherwise
			float start_yaw; // absolute by mouse
			int scroll;
		};

		Contact contact[4]; // 0:mouse, 1:primary_touch 2:secondary_touch 3:unused

		uint8_t but; // real mouse buttons currently down
		int wheel;   // relative mouse wheel (only from real mouse)
		int size[2]; // window size (in pixels)
		bool jump;

		bool shoot;
		int shoot_xy[2];

		bool shot; // screenshot!

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
		MOUSE_MIDDLE_BUT_DOWN,
		MOUSE_MIDDLE_BUT_UP,
		MOUSE_WHEEL_DOWN,
		MOUSE_WHEEL_UP
	};

	enum GAME_TOUCH
	{
		TOUCH_MOVE,
		TOUCH_BEGIN,
		TOUCH_END,
		TOUCH_CANCEL
	};

	// just accumulates input
	void OnKeyb(GAME_KEYB keyb, int key);
	void OnMouse(GAME_MOUSE mouse, int x, int y);
	void OnTouch(GAME_TOUCH touch, int id, int x, int y);
	void OnFocus(bool set);
	void OnSize(int w, int h, int fw, int fh);
	void OnMessage(const uint8_t* msg, int len);

	// update physics with accumulated input then render state to output
	void Render(uint64_t _stamp, AnsiCell* ptr, int width, int height);
	void ScreenToCell(int p[2]) const;
};

Game* CreateGame(int water, float pos[3], float yaw, float dir, uint64_t stamp);
void DeleteGame(Game* g);

void LoadSprites();
void FreeSprites();
