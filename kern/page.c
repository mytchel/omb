/*
 *
 * Copyright (c) 2017 Mytchel Hammond <mytch@lackname.org>
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <head.h>

static struct section sections[MAX_SECTIONS] = { 0 };
static int nextid = 1;

int
section_new(proc_t creator, size_t len, int flags)
{
	section_t s;
	int i;
	
	s = nil;
	for (i = 0;
	     i < sizeof(sections)/sizeof(*sections);
	     i++) {
	
		if (cas(&sections[i].creator, nil, creator)) {
			s = &sections[i];
			break;
		}
	}
	
	if (s == nil) {
		return ERR;
	}
	
	s->id = nextid++;
	s->len = len;
	s->flags = flags;
	s->granted = nil;
	
	return s->id;	
}

void
section_free(int id)
{
	int i;
	
	for (i = 0;
	     i < sizeof(sections)/sizeof(*sections);
	     i++) {
		if (sections[i].id == id) {
			break;
		}
	}
	
	if (i == sizeof(sections)/sizeof(*sections)) {
		return;
	}
	
	sections[i].creator = nil;
}

section_t
section_find(int id)
{
	int i;
	
	for (i = 0;
	     i < sizeof(sections)/sizeof(*sections);
	     i++) {
	     
		if (sections[i].id == id) {
			return &sections[i];
		}
	}
	
	return nil;
}

bool
page_list_add(page_list_t list, int s_id,
              reg_t pa, reg_t va,
              size_t off, size_t len)
{
	int i;
	
	while (true) {
		for (i = 0; i < list->len; i++) {
			if (cas(&list->pages[i].section_id, 
			        (void *) 0, (void *) s_id)) {
			
				list->pages[i].pa = pa;
				list->pages[i].va = va;
				list->pages[i].off = off;
				list->pages[i].len = len;
				return true;
			}
		}
		
		if (list->next != nil) {
			list = list->next;
		} else { 
			return false;
		}
	}
}

page_t
page_list_find(page_list_t list, int s_id, size_t off)
{
	int i;
	
	while (list != nil) {
		for (i = 0; i < list->len; i++) {
			if (list->pages[i].section_id == s_id &&
			    list->pages[i].off == off) {
				return &list->pages[i];
			}
		}
		
		list = list->next;
	}
	
	return nil;
}

page_t
page_list_find_pa(page_list_t list, reg_t pa)
{
	int i;
	
	while (list != nil) {
		for (i = 0; i < list->len; i++) {
			if (list->pages[i].pa == pa) {
				return &list->pages[i];
			}
		}
		
		list = list->next;
	}
	
	return nil;
}

bool
page_list_remove(page_list_t list, int s_id, size_t off,
                 reg_t *va, reg_t *pa, size_t *len)
{
	int i;
	
	while (list != nil) {
		for (i = 0; i < list->len; i++) {
			if (list->pages[i].section_id == s_id &&
			    list->pages[i].off == off) {
				
				*va = list->pages[i].va;
				*pa = list->pages[i].pa;
				*len = list->pages[i].len;
				memset(&list->pages[i], 0, sizeof(struct page));
				return true;
			}
		}
		
		list = list->next;
	}
	
	return false;
}
