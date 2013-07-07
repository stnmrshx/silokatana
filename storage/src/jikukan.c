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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../include/jikukan.h"
#include "../include/config.h"
#include "../include/debug.h"

#define cmp_lt(a, b) (strcmp(a, b) < 0)
#define cmp_eq(a, b) (strcmp(a, b) == 0)
#define NIL list->hdr

struct pool {
	struct pool *next;
	char *ptr;
	unsigned int rem;
};

struct pool *_pool_new()
{
	unsigned int p_size = 8092 - sizeof(struct pool);
	struct pool *pool = calloc(1, sizeof(struct pool) + p_size);
	pool->ptr = (char*)(pool + 1);
	pool->rem = p_size;
	return pool;
}

void _pool_destroy(struct pool *pool)
{
	while (pool->next != NULL) {
		struct pool *next = pool->next;
		free (pool);
		pool = next;
	}
}

void *_pool_alloc(struct jikukan *list, size_t size)
{
	struct pool *pool;
	void *ptr;

	pool = list->pool;
	if (pool->rem < size) {
		pool = _pool_new();
		pool->next = list->pool;
		list->pool = pool;
	}
	ptr = pool->ptr;
	pool->ptr += size;
	pool->rem -= size;
	return ptr;
}

struct jikukan *jikukan_new(size_t size)
{
	int i;
	struct jikukan *list = calloc(1, sizeof(struct jikukan));

	list->hdr = malloc(sizeof(struct skipnode) + MAXLEVEL*sizeof(struct skipnode *));

	for (i = 0; i <= MAXLEVEL; i++)
		list->hdr->forward[i] = NIL;

	list->size = size;
	list->pool = (struct pool *) list->pool_embedded;
	return list;
}

void jikukan_free(struct jikukan *list)
{
	_pool_destroy(list->pool);
	free(list->hdr);
	free(list);
}

int jikukan_notfull(struct jikukan *list)
{
	if (list->count < list->size)
		return 1;

	return 0;
}

int jikukan_insert(struct jikukan *list, char *key, uint64_t val, OPT opt) 
{
	int i, new_level;
	int klen;
	struct skipnode *update[MAXLEVEL+1];
	struct skipnode *x;

	if (!jikukan_notfull(list))
		return 0;

	klen = strlen(key);
	x = list->hdr;
	for (i = list->level; i >= 0; i--) {
		while (x->forward[i] != NIL 
				&& cmp_lt(x->forward[i]->key, key))
			x = x->forward[i];
		update[i] = x;
	}

	x = x->forward[0];
	if (x != NIL && cmp_eq(x->key, key)) {
		x->val = val;
		x->opt = opt;
		return(1);
	}

	for (new_level = 0; rand() < RAND_MAX/2 && new_level < MAXLEVEL; new_level++);

	if (new_level > list->level) {
		for (i = list->level + 1; i <= new_level; i++)
			update[i] = NIL;

		list->level = new_level;
	}

	if ((x =_pool_alloc(list,sizeof(struct skipnode) + new_level*sizeof(struct skipnode *))) == 0)
		__PANIC("error pool_alloc(), maybe memory are not enough");

	memcpy(x->key, key, klen);
	x->val = val;
	x->opt = opt;

	for (i = 0; i <= new_level; i++) {
		x->forward[i] = update[i]->forward[i];
		update[i]->forward[i] = x;
	}
	list->count++;
	return(1);
}

int jikukan_insert_node(struct jikukan *list, struct skipnode *node)
{
	return jikukan_insert(list, node->key, node->val, node->opt);
}

struct skipnode *jikukan_lookup(struct jikukan *list, char* data) 
{
	int i;
	struct skipnode *x = list->hdr;

	for (i = list->level; i >= 0; i--) {
		while (x->forward[i] != NIL 
				&& cmp_lt(x->forward[i]->key, data))
			x = x->forward[i];
	}
	x = x->forward[0];
	if (x != NIL && cmp_eq(x->key, data)) 
		return (x);

	return NULL;
}