/*
** Silokatana - Fast key-value store and storage engine
**
** Copyright (c) 2013, stnmrshx (stnmrshx@gmail.com)
** All rights reserved.
** Redistribution and use in source and binary forms, with or without modification, 
** are permitted provided that the following conditions are met: 
** 
** 1. Redistributions of source code must retain the above copyright notice, this
**    list of conditions and the following disclaimer. 
** 2. Redistributions in binary form must reproduce the above copyright notice,
**    this list of conditions and the following disclaimer in the documentation
**    and/or other materials provided with the distribution. 
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
** ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
**/
#include <stdio.h>
#include <stdlib.h>
#include "../include/level.h"

struct level *level_creat()
{
	return  malloc(sizeof(struct level));
}

void level_set_head(struct level *level, struct level_node *n)
{
	if (n == NULL)
		return;

	level->count++;

	if (level->first == NULL) {
		level->first = n;
	} else {
		n->pre = NULL;
		n->nxt = level->first;
		level->first = n;
	}
}

void level_remove_link(struct level *level, struct level_node *n)
{
	if (n == NULL)
		return;

	level->count--;

	if (n->pre == NULL) {
		if (n->nxt != NULL) {
			level->first = n->nxt;
			n->nxt = NULL;
		}
	} else {
		if (n->nxt == NULL) {
			level->last = n->pre;
			n->pre = NULL;
		} else {
			n->pre->nxt = n->nxt;
			n->nxt = NULL;
			n->pre = NULL;
		}
	}

}	

void level_free_node(struct level *level, struct level_node *n)
{
	if (n) {
		level->used_size -= n->size;
		level_remove_link(level, n);

		if (n->sk.data)
			free(n->sk.data);
		if (n->sv.data)
			free(n->sv.data);
		free(n);
	}
}

void level_free_last(struct level *level, struct ht *h)
{
	struct level_node *n;

	n = level->last;
	ht_remove(h, n->sk.data);
	level_free_node(level, n);
}