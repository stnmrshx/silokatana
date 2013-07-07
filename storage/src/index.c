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
#include "../include/config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "../include/silopit.h"
#include "../include/log.h"
#include "../include/buffer.h"
#include "../include/debug.h"
#include "../include/hiraishin.h"
#include "../include/index.h"

void *_merge_job(void *arg)
{
	int lsn, list_count;
	long long start, end, cost;
	struct index *indx;
	struct jikukan *list;
	struct silopit *silopit;
	struct log *log;

	indx = (struct index*)arg;
	lsn = indx->park->lsn;
	list = indx->park->list;
	list_count = list->count;
	silopit = indx->silopit;
	log = indx->log;


	if(list == NULL)
		goto merge_out;

	start = get_ustime_sec();

	pthread_mutex_lock(&indx->merge_mutex);
	silopit_merge(silopit, list, 0);
	indx->park->list = NULL;
	pthread_mutex_unlock(&indx->merge_mutex);
	
	end = get_ustime_sec();
	cost = end - start;
	if (cost > indx->max_merge_time) {
		indx->slowest_merge_count = list_count;
		indx->max_merge_time = cost;
	}

	log_remove(log, lsn);

merge_out:
	pthread_detach(pthread_self());
	pthread_exit(NULL);
}

struct index *index_new(const char *basedir, int max_mtbl_size, int tolog)
{
	char dbfile[FILE_PATH_SIZE];
	struct index *indx = calloc(1, sizeof(struct index));
	struct indx_park *park = calloc(1, sizeof(struct indx_park));

	ensure_dir_exists(basedir);
	
	indx->max_mtbl = 1;
	indx->max_mtbl_size = max_mtbl_size;
	memset(indx->basedir, 0, FILE_PATH_SIZE);
	memcpy(indx->basedir, basedir, FILE_PATH_SIZE);

	indx->silopit = silopit_new(indx->basedir);
	indx->list = jikukan_new(max_mtbl_size);
	pthread_mutex_init(&indx->merge_mutex, NULL);

	park->lsn = indx->lsn;
	indx->park = park;

	indx->log = log_new(indx->basedir, indx->lsn, tolog);

	if (log_recovery(indx->log, indx->list)) {
		__DEBUG("hold your horses, were trying to merge those log, merge counting #%d....", indx->list->count);
		silopit_merge(indx->silopit, indx->list, 1);

		remove(indx->log->log_new);
		remove(indx->log->log_old);

		indx->list = jikukan_new(indx->max_mtbl_size);
	}

	log_next(indx->log, 0);

	memset(dbfile, 0, FILE_PATH_SIZE);
	snprintf(dbfile, FILE_PATH_SIZE, "%s/%s", indx->basedir, DB_NAME);
	indx->db_rfd = open(dbfile, LSM_OPEN_FLAGS, 0644);
	if (indx->db_rfd == -1)
		__PANIC("index read fd error");

	pthread_attr_init(&indx->attr);
	pthread_attr_setdetachstate(&indx->attr, PTHREAD_CREATE_DETACHED);

	return indx;
}

int index_add(struct index *indx, struct slice *sk, struct slice *sv)
{
	uint64_t value_offset;
	struct jikukan *list, *new_list;

	value_offset = log_append(indx->log, sk, sv);
	list = indx->list;

	if (!list) {
		__PANIC("List<%d> is NULL", indx->lsn);
		return 0;
	}

	if (!jikukan_notfull(list)) {
		indx->bg_merge_count++;

		pthread_mutex_lock(&indx->merge_mutex);

		pthread_t tid;
		indx->park->list = list;
		indx->park->lsn = indx->lsn;
		pthread_mutex_unlock(&indx->merge_mutex);

		pthread_create(&tid, &indx->attr, _merge_job, indx);

		indx->mtbl_rem_count = 0;
		new_list = jikukan_new(indx->max_mtbl_size);
		indx->list = new_list;

		indx->lsn++;
		log_next(indx->log, indx->lsn);
	}
	jikukan_insert(indx->list, sk->data, value_offset, sv == NULL ? DEL : ADD);
	
	if (sv) {
		hiraishin_add(indx->silopit->hiraishin, sk->data);
	} else
		indx->mtbl_rem_count++;

	return 1;
}

void _index_flush(struct index *indx)
{
	int list_count;
	struct jikukan *list;
	long long start, cost;

	pthread_mutex_lock(&indx->merge_mutex);
	pthread_mutex_unlock(&indx->merge_mutex);

	list = indx->list;
	list_count = list->count;

	if (list && list->count > 0) {
		start = get_ustime_sec();
		silopit_merge(indx->silopit, list, 0);
		cost = get_ustime_sec() - start;

		if (cost > indx->max_merge_time) {
			indx->slowest_merge_count = list_count;
			indx->max_merge_time = cost;
		}

		log_remove(indx->log, indx->lsn);
	}

	int log_indx_fd = indx->log->indx_wfd;
	int log_db_fd = indx->log->db_wfd;

	if (log_indx_fd > 0) {
		if (fsync(log_indx_fd) == -1)
			__ERROR("fsync indx fd error when db close");
	}

	if (log_db_fd > 0) {
		if (fsync(log_db_fd) == -1)
			__ERROR("fsync db fd error when db close");
	}
}

int index_get(struct index *indx, struct slice *sk, struct slice *sv)
{
	int ret = 0, value_len, result;
	uint64_t value_off = 0UL;

	struct skipnode *node;
	struct jikukan *cur_list;
	struct jikukan *merge_list;

	ret = hiraishin_get(indx->silopit->hiraishin, sk->data);
	if (ret == 0)
		return 0;
	
	indx->hiraishin_hits++;
	cur_list = indx->list;
	node = jikukan_lookup(cur_list, sk->data);
	if (node){
		if(node->opt == DEL) {
			ret  = -1;
			goto out_get;
		}
		value_off = node->val;
	} else {
		merge_list = indx->park->list;
		if (merge_list) {
			node = jikukan_lookup(merge_list, sk->data);
			if (node && node->opt == ADD )
				value_off = node->val;
		}
	}

	if (value_off == 0UL)
		value_off = silopit_getoff(indx->silopit, sk);

	if (value_off != 0UL) {
		__be32 be32len;
		if (lseek(indx->db_rfd, value_off, SEEK_SET) == -1) {
			__ERROR("seek error when index get");
			goto out_get;
		}

		result = read(indx->db_rfd, &be32len, sizeof(int));
		if(FILE_ERR(result)) {
			ret = -1;
			goto out_get;
		}

		value_len = from_be32(be32len);
		if(result == sizeof(int)) {
			char *data = calloc(1, value_len + 1);
			result = read(indx->db_rfd, data, value_len);
			data[value_len] = 0;

			if(FILE_ERR(result)) {
				free(data);
				ret = -1;
				goto out_get;
			}
			sv->len = value_len;
			sv->data = data;
			return 1;
		}
	} else {
		return 0;
	}

out_get:
	return ret;
}

uint64_t index_allcount(struct index *indx)
{
	int i, size;
	uint64_t c = 0UL;
	
	size = indx->silopit->meta->size;
	for (i = 0; i < size; i++)
		c += indx->silopit->meta->nodes[i].count;

	return c;
}
void index_free(struct index *indx)
{
	_index_flush(indx);

	if (indx->max_merge_time > 0) {
		__INFO("max merge time:%lu sec;"
				"the slowest merge-count:%d and merge-speed:%.1f/sec"
				, indx->max_merge_time
				, indx->slowest_merge_count
				, (double) (indx->slowest_merge_count / indx->max_merge_time));
	}

	pthread_attr_destroy(&indx->attr);
	log_free(indx->log);
	close(indx->db_rfd);
	silopit_free(indx->silopit);
	free(indx->park);
	free(indx);
}