
#include "platform.h"

// this is screen manager
// handling multiple layers (creation, destruction, order, focus, position and blending effects)
// it also receives input stream and dispatches it to given layer input handler
// in xterm mode it is responsible for displaying mouse cursor on top of screen

#include "render.h"

#if 0

struct ScreenCB
{
	void(*touch)();
	void(*mouse)();
	void(*keyb)(); // (focused screen only) 
	void(*pad)();  // (focused screen only) 
};

struct Layer;

struct Screen
{
	Layer* parent; // if null it is screen!
	void* cookie;

	// children
	Layer* head;
	Layer* tail;

	// buf should be cleared or prerendered with some scene or left dirty if children overlaps it fully
	void Merge(AnsiCell* buf, int width, int height);
};

struct Layer : Screen
{
	bool visible;

	// siblings
	Layer* prev;
	Layer* next;
};


void Screen::Merge(AnsiCell* buf, int width, int height)
{
	ClipRect cr = { 0,0, width,height };
	Layer* lay = head;
	while (lay)
	{
		cr.x1 = max(0, lay->x, )
		lay->Merge(buf, &cr, lay->x, lay->y);
		lay = lay->next;
	}
}

void Layer::Merge(AnsiCell* buf, int width, int height, int src_x, int src_y, int dst_x, int dst_y, int )
{
	cr->x1 += x;
	cr->y1 += x;
	cr->x2 += x;
	cr->y2 += x;
}

// for mouse & touch
Screen* HitTest(Screen* root, int x, int y)
{
	// 1. locate topmost screen for root
	// 2. traverse down until non transparent bg or fg is found
}

#endif
