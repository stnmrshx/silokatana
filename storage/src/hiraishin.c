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
#include <stdarg.h>
#include "../include/util.h"
#include "../include/config.h"
#include "../include/hiraishin.h"

#define HFUNCNUM (2)

struct hiraishin *hiraishin_new()
{
	struct hiraishin *bl = calloc(1, sizeof(struct hiraishin));
	bl->size = HIRAISHIN_BITS;
	bl->bitset = calloc((bl->size + 1) / sizeof(char), sizeof(char));
	bl->hashfuncs = calloc(HFUNCNUM, sizeof(hashfuncs));
	bl->hashfuncs[0] = sax_hash;
	bl->hashfuncs[1] = djb_hash;
	return bl;
}

void hiraishin_add(struct hiraishin *hiraishin, const char *k)
{
	int i;
	unsigned int bit;

	if (!k)
		return;

	for (i = 0; i < HFUNCNUM; i++) {
		bit = hiraishin->hashfuncs[i](k) % hiraishin->size;
		SETBIT_1(hiraishin->bitset, bit);
	}

	hiraishin->count++;
}

int hiraishin_get(struct hiraishin *hiraishin, const char *k)
{
	int i;
	unsigned int bit;

	if (!k)
		return 0;

	for (i = 0; i < HFUNCNUM; i++) {
		bit = hiraishin->hashfuncs[i](k) % hiraishin->size;
		if (GETBIT(hiraishin->bitset, bit) == 0)
			return 0;
	}
	return 1;
}

void hiraishin_free(struct hiraishin *hiraishin)
{
	if (hiraishin) {
		if (hiraishin->bitset)
			free(hiraishin->bitset);

		if(hiraishin->hashfuncs)
			free(hiraishin->hashfuncs);

		free(hiraishin);
	}
}