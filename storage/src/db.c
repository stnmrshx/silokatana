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
#include <string.h>
#include "../include/shiki.h"
#include "../include/index.h"
#include "../include/buffer.h"
#include "../include/util.h"
#include "../include/config.h"
#include "../include/db.h"
#include "../include/debug.h"

struct silokatana *db_open(size_t bufferpool_size, const char *basedir, int is_log_recovery)
{
	char buff_dir[FILE_PATH_SIZE];
	struct silokatana *db;
	db = malloc(sizeof(struct silokatana));
	db->lru = shiki_new(bufferpool_size);
	db->buf = buffer_new(LOG_BUFFER_SIZE);
	db->start_time = time(NULL);
	db->lru_cached = 0;
	db->lru_missing = 0;
	memset(buff_dir, 0, FILE_PATH_SIZE);
	snprintf(buff_dir, FILE_PATH_SIZE, "%s/silodbs", basedir);
	db->indx = index_new(buff_dir, MTBL_MAX_COUNT, is_log_recovery); 
	return db;
}

int db_add(struct silokatana *db, struct slice *sk, struct slice *sv)
{
	shiki_remove(db->lru, sk);
	return index_add(db->indx, sk, sv);
}

int db_get(struct silokatana *db, struct slice *sk, struct slice *sv)
{
	int ret = 0;
	char *data;
	struct slice *sv_l;

	sv_l = shiki_get(db->lru, sk);

	if (sv_l) {
		db->lru_cached++;

		data = calloc(1, sv_l->len + 1);
		memcpy(data, sv_l->data, sv_l->len);
		data[sv_l->len] = 0;

		sv->len = sv_l->len;
		sv->data = data;
		ret = 1;
	} else {
		db->lru_missing++;

		ret = index_get(db->indx, sk, sv);
		if (ret == 1) {
			shiki_set(db->lru, sk, sv);
		}

	}
	return ret;
}

int db_exists(struct silokatana *db, struct slice *sk)
{
	struct slice sv;
	int ret = index_get(db->indx, sk, &sv);

	if (ret == 1) {
		free(sv.data);
		return 1;
	}
	return 0;
}

void db_remove(struct silokatana *db, struct slice *sk)
{
	shiki_remove(db->lru, sk);
	index_add(db->indx, sk, NULL);
}

char *db_info(struct silokatana *db)
{
	int total_lru_memory_usage = (db->lru->level_new.used_size + db->lru->level_old.used_size) / (1024 * 1024);
	int total_lru_hot_memory_usage = db->lru->level_new.used_size / (1024 * 1024);
	int total_lru_cold_memory_usage = db->lru->level_old.used_size / (1024 * 1024);
	int max_allow_lru_memory_usage = (db->lru->level_old.allow_size + db->lru->level_new.allow_size) / (1024 * 1024);

	buffer_clear(db->buf);
	buffer_scatf(db->buf, 
			"# Memory\r\n"
			"Total Memory Usage : %d(MB)\r\n"
			"Total Hot Memory Usage : %d(MB)\r\n"
			"Total Cold Memory Usage : %d(MB)\r\n"
			"Max Allow Memory Usage : %d(MB)\r\n"
		,
			total_lru_memory_usage ,
			total_lru_hot_memory_usage,
			total_lru_cold_memory_usage,
			max_allow_lru_memory_usage);

	return buffer_detach(db->buf);
}

void db_close(struct silokatana *db)
{
	index_free(db->indx);
	shiki_free(db->lru);
	buffer_free(db->buf);
	free(db);
}