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
#ifndef _INDEX_H
#define _INDEX_H

#include <pthread.h>
#include "jikukan.h"
#include "util.h"

struct indx_park{
	struct jikukan *list;
	int lsn;
};

struct index{
	int lsn;
	int db_rfd;
	int meta_lsn;
	int bg_merge_count;
	int mtbl_add_count;
	int mtbl_rem_count;
	int max_mtbl;
	int max_mtbl_size;
	int slowest_merge_count; 
	long long max_merge_time; 
	uint64_t hiraishin_hits;

	char basedir[FILE_PATH_SIZE];
	struct log *log;
	struct silopit *silopit;
	struct jikukan *list;
	struct indx_park *park;
	pthread_attr_t attr;
	pthread_mutex_t merge_mutex;
};

struct index *index_new(const char *basedir, int max_mtbl_size, int tolog);
int index_add(struct index *indx, struct slice *sk, struct slice *sv);
int index_get(struct index *indx, struct slice *sk, struct slice *sv);
uint64_t index_allcount(struct index *indx);
void index_free(struct index *indx);

#endif