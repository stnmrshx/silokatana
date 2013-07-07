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
#include <string.h>
#include "../include/util.h"
#include "../include/ht.h"

static size_t find_slot(struct ht *ht, void *key)
{
	return djb_hash((const char*)key) % ht->cap;
}

struct ht *ht_new(size_t cap)
{
	struct ht *ht = calloc(1, sizeof(struct ht));
	ht->cap = cap;
	ht->nodes = calloc(cap, sizeof(struct ht_node*));
	return ht;
}

void ht_set(struct ht *ht, void *key, void *value)
{
	size_t slot;
	struct ht_node *node;

	slot = find_slot(ht, key);
	node = calloc(1, sizeof(struct ht_node));
	node->next = ht->nodes[slot];
	node->key = key;
	node->value = value;

	ht->nodes[slot] = node;
	ht->size++;
}

void *ht_get(struct ht *ht, void *key)
{
	size_t slot;
	struct ht_node *node;

	slot = find_slot(ht, key);
	node = ht->nodes[slot];

	while (node) {
		if (strcmp(key, node->key) == 0)
			return node->value;

		node=node->next;
	}

	return NULL;
}

void ht_remove(struct ht *ht, void *key)
{
	size_t slot;
	struct ht_node *node, *prev = NULL;

	slot = find_slot(ht,key);
	node = ht->nodes[slot];

	while (node) {
		if (strcmp(key, node->key) == 0) {
			if (prev != NULL)
				prev->next = node->next;
			else
				ht->nodes[slot] = node->next;
			if (node)
				free(node);
			ht->size--;

			return;
		}
		prev = node;
		node = node->next;	
	}
}

void ht_free(struct ht *ht)
{
	free(ht->nodes);
	free(ht);
}