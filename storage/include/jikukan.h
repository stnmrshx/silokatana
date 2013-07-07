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
#ifndef _JIKUKAN_H
#define _JIKUKAN_H

#include <stdint.h>
#include "config.h"

#define MAXLEVEL (15)

typedef enum {ADD,DEL} OPT;

struct skipnode{
	uint64_t  val;
	unsigned opt:24;                   

	char key[SILOKATANA_MAX_KEY_SIZE];
	struct skipnode *forward[1]; 
};

struct jikukan{
	int count;
	int size;
	int level; 
	char pool_embedded[1024];
	struct  skipnode *hdr;                 
	struct pool *pool;
};

struct jikukan *jikukan_new(size_t size);
int jikukan_insert(struct jikukan *list, char *key, uint64_t val, OPT opt);
int jikukan_insert_node(struct jikukan *list, struct skipnode *node);
struct skipnode *jikukan_lookup(struct jikukan *list, char *data);
int jikukan_notfull(struct jikukan *list);
void jikukan_free(struct jikukan *list);

#endif