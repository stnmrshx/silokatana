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
#ifndef _SILOPIT_H
#define _SILOPIT_H

#include <stdint.h>
#include <pthread.h>
#include "buffer.h"
#include "jikukan.h"
#include "meta.h"
#include "hiraishin.h"
#include "util.h"

struct mutexer{
	volatile int lsn;
	pthread_mutex_t mutex;
};

struct silopit{
	char basedir[FILE_PATH_SIZE];
	char name[FILE_NAME_SIZE];
	uint32_t lsn;
	struct meta *meta;
	struct hiraishin *hiraishin;
	struct mutexer mutexer;
	struct buffer *buf;
};

struct silopit *silopit_new(const char *basedir);
void silopit_merge(struct silopit *silopit, struct jikukan *list, int fromlog);
uint64_t silopit_getoff(struct silopit *silopit, struct slice *sk);
void silopit_free(struct silopit *silopit);

#endif