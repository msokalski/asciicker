#pragma once
#include "gl.h"

#define TEXHEAP

struct TexHeap;
struct TexAlloc;
struct TexPage;

struct TexAlloc
{
	// read-write
	void* user; 

	// read-only
	TexPage* page;
	int x, y;

	void Update(GLenum format, GLenum type, void* data);
	void Free();
};

struct TexPage
{
	// read only
	void* user; // data at the pointer can be modified
	TexHeap* heap;
	TexPage* next;
	TexPage* prev;
	GLuint tex;
	TexAlloc* alloc[1]; // [heap->cap_x*heap->cap_y]
};

struct TexHeap
{
	void Create(int page_cap_x, int page_cap_y, int alloc_w, int alloc_h, GLenum internal_format, int page_user_bytes);
	void Destroy();

	TexAlloc* Alloc(GLenum format, GLenum type, void* data);

	// spatial optimizer support
	// simply swap TexAlloc pointers and TexAlloc::user data
	// then update both allocs

	// const read-only
	int item_w;
	int item_h;
	int cap_x;
	int cap_y;

	// read only
	int user; // num of extra bytes allocated with every page for user
	int allocs;
	GLenum ifmt;
	TexPage* head;
	TexPage* tail;
};
