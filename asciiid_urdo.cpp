#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "asciiid_urdo.h"

#if 0
// todo: switch to file storage
// - each command record: {[cmd_id][DATA][rec_size]}
// - stream contains group open/close:  ....{open}{}{}{open}{}{}{}<-undo|redo->{}{}{close}{}{close}....
// - we just need to track current stack depth

struct URDO
{
	enum CMD
	{
		CMD_GROUP_OPEN,
		CMD_GROUP_CLOSE,
		CMD_PATCH_UPDATE,
		CMD_PATCH_DIAG
	} cmd;

	// followed by data and record size in bytes (int)

	void Do(bool un);
	static URDO* Alloc(CMD c);
	void Free();
};

// note: no need to have PurgeUndo()

static void PurgeRedo()
{
	// truncate file at current pos
	// write 'closes' for all open groups
}

void URDO_Purge()
{
	// reset stack & file size
	// move file pointer to 0 and flush
}

bool URDO_CanUndo()
{
	// true if we're not at the file head
}

bool URDO_CanRedo()
{
	// true if we're not at the file tail
}

size_t URDO_Bytes()
{
	// file size (keep track)
}

void URDO_Undo(int max_depth)
{
}

void URDO_Redo(int max_depth)
{
}

void URDO_Open()
{
	// write CMD
	// write sizeof(REC)
}

void URDO_Close()
{
	// write CMD
	// write sizeof(REC)
}

void URDO_Patch(Patch* p)
{
	// write CMD
	// write Patch*
	// write GetTerrainHeightMap(p)
	// write GetTerrainDiag(p)
	// write sizeof(REC)
}

void URDO_Diag(Patch* p)
{
	// write CMD
	// write Patch*
	// write GetTerrainDiag(p)
	// write sizeof(REC)
}

#endif

struct URDO
{
	URDO* next;
	URDO* prev;

	enum CMD
	{
		CMD_GROUP,
		CMD_PATCH_UPDATE,
		CMD_PATCH_DIAG
	} cmd;

	void Do(bool un);
	static URDO* Alloc(CMD c);
	void Free();
};

struct URDO_Group : URDO
{
	URDO* group_head;
	URDO* group_tail;

	static void Open();
	static void Close();
	void Do(bool un);
};

struct URDO_PatchUpdate : URDO
{
	Patch* patch;
	uint16_t height[HEIGHT_CELLS + 1][HEIGHT_CELLS + 1];
	uint16_t diag;

	static void Open(Patch* p); // alloc and copy original
	void Do(bool un);
};

struct URDO_PatchDiag : URDO
{
	Patch* patch;
	uint16_t diag;

	static void Open(Patch* p);
	void Do(bool un);
};

static size_t bytes = 0;
static URDO* undo = 0;
static URDO* redo = 0;

static int group_open = 0;
static int stack_depth = 0;
static URDO_Group* stack[64];

void URDO::Do(bool un)
{
	switch (cmd)
	{
		case CMD_GROUP: ((URDO_Group*)this)->Do(un); break;
		case CMD_PATCH_UPDATE: ((URDO_PatchUpdate*)this)->Do(un); break;
		case CMD_PATCH_DIAG: ((URDO_PatchDiag*)this)->Do(un); break;
		default:
			assert(0);
	}
}

void URDO::Free()
{
	switch (cmd)
	{
		case CMD_GROUP: 
		{
			URDO* u = ((URDO_Group*)this)->group_head;
			while (u)
			{
				URDO* n = u->next;
				u->Free();
				u = n;
			}

			bytes -= sizeof(URDO_Group);
			break;
		}
		case CMD_PATCH_UPDATE: 
			bytes -= sizeof(URDO_PatchUpdate); 
			break;
		case CMD_PATCH_DIAG:
			bytes -= sizeof(URDO_PatchDiag);
			break;

		default:
			assert(0);
	}

	free(this);
}

URDO* URDO::Alloc(CMD c)
{
	size_t s = 0;
	switch (c)
	{
		case CMD_GROUP: s = sizeof(URDO_Group); break;
		case CMD_PATCH_UPDATE: s = sizeof(URDO_PatchUpdate); break;
		case CMD_PATCH_DIAG: s = sizeof(URDO_PatchDiag); break;
		default:
			assert(0);
	}

	URDO* urdo = (URDO*)malloc(s);
	bytes += s;

	if (stack_depth)
	{
		URDO_Group* p = stack[stack_depth - 1];
		if (!p->group_head)
			p->group_head = urdo;
		p->group_tail = urdo;
	}

	urdo->prev = undo;
	urdo->next = 0;
	urdo->cmd = c;
	if (undo)
		undo->next = urdo;
	undo = urdo;

	return urdo;
}

static void PurgeUndo()
{
	while (1)
	{
		URDO* head = redo;
		if (head)
			head->prev = 0;

		while (undo)
		{
			URDO* prev = undo->prev;
			undo->Free();
			undo = prev;
		}

		if (stack_depth)
		{
			URDO_Group* g = stack[--stack_depth];
			g->group_head = head;
			if (!head)
			{
				undo = g->prev;
				redo = g->next;
				
				g->group_tail = 0;

				g->Free();
				if (redo)
					redo->prev = undo;
				if (undo)
					undo->next = redo;
			}
			else
			{
				redo = g;
				undo = g->prev;
			}
		}
		else
			break;
	}
}

static void PurgeRedo()
{
	while (1)
	{
		URDO* tail = undo;
		if (tail)
			tail->next = 0;

		while (redo)
		{
			URDO* next = redo->next;
			redo->Free();
			redo = next;
		}

		if (stack_depth)
		{
			URDO_Group* g = stack[--stack_depth];
			g->group_tail = tail;
			if (!tail)
			{
				redo = g->next;
				undo = g->prev;

				g->group_head = 0;

				g->Free();
				if (undo)
					undo->next = redo;
				if (redo)
					redo->prev = undo;
			}
			else
			{
				undo = g;
				redo = g->next;
			}
		}
		else
			break;
	}
}

void URDO_Purge()
{
	assert(!group_open);

	PurgeUndo();
	PurgeRedo();
}

bool URDO_CanUndo()
{
	if (group_open)
		return false;
	if (undo)
		return true;
	int d = stack_depth;
	while (d)
	{
		if (stack[--d]->prev)
			return true;
	}
	return false;
}

bool URDO_CanRedo()
{
	if (group_open)
		return false;
	if (redo)
		return true;
	int d = stack_depth;
	while (d)
	{
		if (stack[--d]->next)
			return true;
	}
	return false;
}

size_t URDO_Bytes()
{
	return bytes;
}

void URDO_Undo(int max_depth)
{
	assert(!group_open);

	while (!undo && stack_depth)
	{
		URDO_Group* g = stack[--stack_depth];
		undo = g->prev;
		redo = g;
	}

	while (undo && undo->cmd == URDO::CMD_GROUP && stack_depth < max_depth)
	{
		URDO_Group* g = (URDO_Group*)undo;
		stack[stack_depth++] = g;
		undo = g->group_tail;
		redo = 0;
	}

	if (stack_depth <= max_depth && undo)
	{
		undo->Do(true);
		redo = undo;
		undo = undo->prev;
		return;
	}

	while (stack_depth > max_depth)
	{
		while (undo)
		{
			undo->Do(true);
			redo = undo;
			undo = undo->prev;
		}

		URDO_Group* g = stack[--stack_depth];
		undo = g->prev;
		redo = g;
	}
}

void URDO_Redo(int max_depth)
{
	assert(!group_open);

	while (!redo && stack_depth)
	{
		URDO_Group* g = stack[--stack_depth];
		redo = g->next;
		undo = g;
	}

	while (redo && redo->cmd == URDO::CMD_GROUP && stack_depth < max_depth)
	{
		URDO_Group* g = (URDO_Group*)redo;
		stack[stack_depth++] = g;
		redo = g->group_head;
		undo = 0;
	}

	if (stack_depth <= max_depth && redo)
	{
		redo->Do(false);
		undo = redo;
		redo = redo->next;
		return;
	}

	while (stack_depth > max_depth)
	{
		while (redo)
		{
			redo->Do(true);
			undo = redo;
			redo = redo->next;
		}

		URDO_Group* g = stack[--stack_depth];
		redo = g->next;
		undo = g;
	}
}

void URDO_Open()
{
	assert(group_open<64);
	URDO_Group::Open();
}

void URDO_Close()
{
	assert(group_open>0);
	URDO_Group::Close();
}

void URDO_Patch(Patch* p)
{
	URDO_PatchUpdate::Open(p);
}

void URDO_Diag(Patch* p)
{
	URDO_PatchDiag::Open(p);
}

void URDO_Group::Open()
{
	if (!group_open)
		PurgeRedo();
	group_open++;

	URDO_Group* g = (URDO_Group*)Alloc(CMD_GROUP);

	stack[stack_depth++] = g;

	g->group_head = 0;
	g->group_tail = 0;

	undo = 0;
	redo = 0;
}

void URDO_Group::Close()
{
	group_open--;

	// here we swap global with group lists
	URDO_Group* g = stack[--stack_depth];

	if (!g->group_head)
	{
		// delete empty group
		undo = g->prev;
		if (g->prev)
			g->prev->next = 0;
		g->Free();
		return;
	}

	g->group_tail = undo;
	undo = g;
}

void URDO_Group::Do(bool un)
{
	if (un)
	{
		URDO* urdo = group_tail;
		while (urdo)
		{
			urdo->Do(true);
			urdo = urdo->prev;
		}
	}
	else
	{
		URDO* urdo = group_head;
		while (urdo)
		{
			urdo->Do(false);
			urdo = urdo->next;
		}
	}
}

void URDO_PatchUpdate::Open(Patch* p)
{
	if (!group_open)
		PurgeRedo();

	URDO_PatchUpdate* urdo = (URDO_PatchUpdate*)Alloc(CMD_PATCH_UPDATE);

	urdo->patch = p;
	memcpy(urdo->height, GetTerrainHeightMap(p), sizeof(uint16_t)*(HEIGHT_CELLS+1)*(HEIGHT_CELLS+1));
	urdo->diag = GetTerrainDiag(p);
}

void URDO_PatchUpdate::Do(bool un)
{
	uint16_t* t = GetTerrainHeightMap(patch);
	uint16_t* u = (uint16_t*)height;
	for (int i = 0; i < (HEIGHT_CELLS + 1)*(HEIGHT_CELLS + 1); i++)
	{
		uint16_t s = t[i];
		t[i] = u[i];
		u[i] = s;
	}

	uint16_t d = diag;
	diag = GetTerrainDiag(patch);

	UpdateTerrainHeightMap(patch);
	SetTerrainDiag(patch,d);
}

void URDO_PatchDiag::Open(Patch* p)
{
	if (!group_open)
		PurgeRedo();

	URDO_PatchDiag* urdo = (URDO_PatchDiag*)Alloc(CMD_PATCH_DIAG);

	urdo->patch = p;
	urdo->diag = GetTerrainDiag(p);
}

void URDO_PatchDiag::Do(bool un)
{
	uint16_t d = diag;
	diag = GetTerrainDiag(patch);
	SetTerrainDiag(patch, d);
}

