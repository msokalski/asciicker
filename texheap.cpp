
#include <malloc.h>
#include "texheap.h"

void TexHeap::Create(int page_cap_x, int page_cap_y, int alloc_w, int alloc_h, GLenum internal_format, int page_user_bytes)
{
	allocs = 0;
	item_w = alloc_w;
	item_h = alloc_h;
	cap_x = page_cap_x;
	cap_y = page_cap_y;
	ifmt = internal_format;
	user = page_user_bytes;
	head = 0;
	tail = 0;
}

void TexHeap::Destroy()
{
	if (!head)
		return;

	int cap = cap_x * cap_y;
	TexPage* p = head;
	while (p!=tail)
	{
		TexPage* n = p->next;
		glDeleteTextures(1, &p->tex);
		for (int i = 0; i < cap; i++)
			free(p->alloc[i]);
		free(p);
		p = n;
	}

	cap = allocs % cap;
	glDeleteTextures(1, &p->tex);
	for (int i = 0; i < cap; i++)
		free(p->alloc[i]);
	free(p);
}

TexAlloc* TexHeap::Alloc(GLenum format, GLenum type, void* data)
{
	int cap = cap_x * cap_y;
	TexPage* page = tail;
	if (allocs % cap == 0)
	{
		int alloc_bytes = sizeof(TexPage) + sizeof(TexAlloc*)*(cap - 1);
		int page_bytes = alloc_bytes + user;
		page = (TexPage*)malloc(page_bytes);
		if (!page)
			return 0;

		page->user = (char*)page + alloc_bytes;
		memset(page->user, 0, user);

		page->heap = this;
		page->prev = tail;
		page->next = 0;
		if (tail)
			tail->next = page;
		else
			head = page;
		tail = page;
			
		glCreateTextures(GL_TEXTURE_2D, 1, &page->tex);
		glTextureStorage2D(page->tex, 1, ifmt, cap_x * item_w, cap_y * item_h);
		glTextureParameteri(page->tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(page->tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(page->tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(page->tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	int on_page = allocs % cap;

	TexAlloc* a = (TexAlloc*)malloc(sizeof(TexAlloc));
	page->alloc[on_page] = a;
	a->page = page;
	a->x = on_page % cap_x;
	a->y = on_page / cap_x;

	allocs++;

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTextureSubImage2D(page->tex, 0, a->x * item_w, a->y * item_h, item_w, item_h, format, type, data);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	return a;
}

void TexAlloc::Free()
{
	TexHeap* h = page->heap;
	int cap = h->cap_x * h->cap_y;
	int on_page = y * h->cap_x + x;

	// not last alloc on last page?
	if (page != h->tail || on_page != h->allocs % cap)
	{
		TexAlloc* last = h->tail->alloc[h->allocs % cap];
		glCopyImageSubData(
			last->page->tex, GL_TEXTURE_2D, 0, last->x * h->item_w, last->y * h->item_h, 0,
			page->tex, GL_TEXTURE_2D, 0, x * h->item_w, y * h->item_h, 0,
			h->item_w, h->item_h, 1);
		page->alloc[on_page] = last;
		last->page = page;
		last->x = x;
		last->y = y;
	}

	h->allocs--;
	free(this);

	// now check if we can delete last page 
	if (h->allocs % cap == 0)
	{
		glDeleteTextures(1, &h->tail->tex);
		if (h->tail->prev)
			h->tail->prev->next = 0;
		else
			h->head = 0;
		h->tail = h->tail->prev;
		free(h->tail);
	}
}

void TexAlloc::Update(GLenum format, GLenum type, void* data)
{
	TexHeap* h = page->heap;
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTextureSubImage2D(page->tex, 0, x * h->item_w, y * h->item_h, h->item_w, h->item_h, format, type, data);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}
