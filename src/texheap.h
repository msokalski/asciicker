#pragma once
#include "gl.h"

#define TEXHEAP

#define TEXHEAP_MAX_NUMTEX 4

struct TexHeap;
struct TexAlloc;
struct TexPage;

struct TexData
{
	GLenum format;
	GLenum type;
	void* data;
};

struct TexAlloc
{
	// read-write
	void* user; 

	// read-only
	TexPage* page;
	int x, y;

	void Update(int first, int count, const TexData[]);
	TexAlloc* Free();
};

struct TexPage
{
	// read only
	void* user; // data at the pointer can be modified
	TexHeap* heap;
	TexPage* next;
	TexPage* prev;
	GLuint tex[TEXHEAP_MAX_NUMTEX];
	TexAlloc* alloc[1]; // [heap->cap_x*heap->cap_y]
};

struct TexDesc
{
	int item_w;
	int item_h;
	GLenum ifmt;
};

struct TexHeap
{
	void Create(int page_cap_x, int page_cap_y, int numtex, const TexDesc* texdesc, int page_userbytes);
	void Destroy();

	TexAlloc* Alloc(const TexData data[]);

	// spatial optimizer support
	// simply swap TexAlloc pointers and TexAlloc::user data
	// then update both allocs

	// const read-only

	int cap_x;
	int cap_y;

	// read only
	int user; // num of extra bytes allocated with every page for user
	int allocs;

	TexPage* head;
	TexPage* tail;

	/*
	int item_w;
	int item_h;
	GLenum ifmt;
	*/

	int num;
	TexDesc tex[TEXHEAP_MAX_NUMTEX];
};
