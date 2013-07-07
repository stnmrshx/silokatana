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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../include/shiki.h"

#define MAXHITS (3)
#define PRIME	(16785407)
#define RATIO	(0.618)

struct shiki *shiki_new(size_t buffer_size)
{
	size_t size_level_new;
	size_t size_level_old;
	struct shiki *lru;

	size_level_new = buffer_size * RATIO;
	size_level_old = buffer_size - size_level_new;
	
	lru = calloc(1, sizeof(struct shiki));
	lru->ht = ht_new(PRIME);

	lru->level_old.allow_size = size_level_old;
	lru->level_new.allow_size = size_level_new;

	if (buffer_size > 1023)
		lru->buffer = 1;

	return lru;
}

void _shiki_set_node(struct shiki *lru, struct level_node *n)
{
	if (n != NULL) {
		if (n->hits == -1) {
			if (lru->level_new.used_size >= lru->level_new.allow_size) {
				level_free_last(&lru->level_new, lru->ht);
			}
			level_remove_link(&lru->level_new, n);
			level_set_head(&lru->level_new, n);
		} else {
			if (lru->level_old.used_size >= lru->level_old.allow_size) {
				level_free_last(&lru->level_old, lru->ht);
			}

			n->hits++;
			level_remove_link(&lru->level_old, n);
			if (n->hits > MAXHITS) {
				level_set_head(&lru->level_new, n);
				n->hits = -1;
				lru->level_old.used_size -= n->size;
				lru->level_new.used_size += n->size;
			} else
				level_set_head(&lru->level_old, n);
		}
	}
}

void shiki_set(struct shiki *lru, struct slice *sk, struct slice *sv)
{
	size_t size;
	struct level_node *n;

	if (lru->buffer == 0)
		return;

	size = (sk->len + sv->len);
	n = (struct level_node*)ht_get(lru->ht, sk->data);
	if (n == NULL) {
		lru->level_old.used_size += size;
		lru->level_old.count++;

		n = calloc(1, sizeof(struct level_node));
		n->sk.data = malloc(sk->len);
		n->sk.len = sk->len;
		memset(n->sk.data, 0, sk->len);
		memcpy(n->sk.data, sk->data, sk->len);

		n->sv.data = malloc(sv->len);
		n->sv.len = sv->len;
		memset(n->sv.data, 0, sv->len);
		memcpy(n->sv.data, sv->data, sv->len);

		n->size = size;
		n->hits = 1;
		n->pre = NULL;
		n->nxt = NULL;

		ht_set(lru->ht, n->sk.data, n);
	}
	_shiki_set_node(lru, n);
}

struct slice *shiki_get(struct shiki *lru, struct slice *sk)
{
	struct level_node *n;

	if (lru->buffer == 0)
		return NULL;

	n = (struct level_node*)ht_get(lru->ht, sk->data);
	if (n != NULL) {
		_shiki_set_node(lru, n);
		return &n->sv;
	}

	return NULL;
}

void shiki_remove(struct shiki *lru, struct slice *sk)
{
	struct level_node *n;

	if (lru->buffer == 0)
		return;

	n = (struct level_node *)ht_get(lru->ht, sk->data);
	if (n != NULL) {
		ht_remove(lru->ht, sk->data);

		if (n->hits == -1) 
			level_free_node(&lru->level_new, n);
		else 
			level_free_node(&lru->level_old, n);
	}
}

void shiki_free(struct shiki *lru)
{
	ht_free(lru->ht);
	free(lru);
}