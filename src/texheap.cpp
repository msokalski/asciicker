
#include <stdlib.h>
#include <string.h>
#include "texheap.h"
#include "gl45_emu.h"

void TexHeap::Create(int page_cap_x, int page_cap_y, int numtex, const TexDesc* texdesc, int page_user_bytes)
{
	allocs = 0;
	cap_x = page_cap_x;
	cap_y = page_cap_y;
	user = page_user_bytes;
	head = 0;
	tail = 0;

	// item_w = alloc_w;
	// item_h = alloc_h;
	// ifmt = internal_format;

	num = numtex;
	memcpy(tex, texdesc, num * sizeof(TexDesc));
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
		glDeleteTextures(num, p->tex);
		for (int i = 0; i < cap; i++)
			free(p->alloc[i]);
		free(p);
		p = n;
	}

	cap = allocs % cap;
	glDeleteTextures(num, p->tex);
	for (int i = 0; i < cap; i++)
		free(p->alloc[i]);
	free(p);
}

TexAlloc* TexHeap::Alloc(const TexData data[])
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
		
		gl3CreateTextures(GL_TEXTURE_2D, num, page->tex);
		for (int t = 0; t < num; t++)
		{
			gl3TextureStorage2D(page->tex[t], 1, tex[t].ifmt, cap_x * tex[t].item_w, cap_y * tex[t].item_h);
			gl3TextureParameteri2D(page->tex[t], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			gl3TextureParameteri2D(page->tex[t], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			gl3TextureParameteri2D(page->tex[t], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			gl3TextureParameteri2D(page->tex[t], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
	}

	int on_page = allocs % cap;

	TexAlloc* a = (TexAlloc*)malloc(sizeof(TexAlloc));
	page->alloc[on_page] = a;
	a->page = page;
	a->x = on_page % cap_x;
	a->y = on_page / cap_x;

	allocs++;

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	for (int t = 0; t < num; t++)
	{
		gl3TextureSubImage2D(page->tex[t], 0, a->x * tex[t].item_w, a->y * tex[t].item_h,
			tex[t].item_w, tex[t].item_h, data[t].format, data[t].type, data[t].data);
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	return a;
}

TexAlloc* TexAlloc::Free() // return last alloc which must be re-updated (GL3 only)
{
	TexHeap* h = page->heap;
	int cap = h->cap_x * h->cap_y;
	int on_page = y * h->cap_x + x;

	TexAlloc* last = 0;

	// not last alloc on last page?
	if (page != h->tail || on_page != (h->allocs-1) % cap)
	{
		last = h->tail->alloc[(h->allocs-1) % cap];

		#if !USE_GL3
		for (int t = 0; t < h->num; t++)
		{
			gl3CopyImageSubData(
				last->page->tex[t], GL_TEXTURE_2D, 0, last->x * h->tex[t].item_w, last->y * h->tex[t].item_h, 0,
				page->tex[t], GL_TEXTURE_2D, 0, x * h->tex[t].item_w, y * h->tex[t].item_h, 0,
				h->tex[t].item_w, h->tex[t].item_h, 1);
		}
		#endif

		page->alloc[on_page] = last;
		last->page = page;
		last->x = x;
		last->y = y;

		#if !USE_GL3
		last = 0;
		#endif
	}

	h->allocs--;
	free(this);

	// now check if we can delete last page 
	if (h->allocs % cap == 0)
	{
		glDeleteTextures(h->num, h->tail->tex);
		if (h->tail->prev)
			h->tail->prev->next = 0;
		else
			h->head = 0;
		h->tail = h->tail->prev;
		free(h->tail);
	}

	return last;
}

void TexAlloc::Update(int first, int count, const TexData data[])
{
	TexHeap* h = page->heap;
	int end = first + count;
	end = h->num < end ? h->num : end;
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	for (int t = first; t < end; t++)
	{
		gl3TextureSubImage2D(page->tex[t], 0, x * h->tex[t].item_w, y * h->tex[t].item_h,
			h->tex[t].item_w, h->tex[t].item_h, data[t-first].format, data[t-first].type, data[t-first].data);
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}
