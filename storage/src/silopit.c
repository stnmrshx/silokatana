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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>

#include "../include/config.h"
#include "../include/silopit.h"
#include "../include/debug.h"

#define BLK_MAGIC (20111225)
#define F_CRC (2011)

struct footer{
	char key[SILOKATANA_MAX_KEY_SIZE];
	__be32 count;
	__be32 crc;
	__be32 size;
	__be32 max_len;
};

struct stats {
	int mmap_size;
	int max_len;
};

void _prepare_stats(struct skipnode *x, size_t count, struct stats *stats)
{
	size_t i;
	int real_count = 0;
	int max_len = 0;
	uint64_t min = 0UL;
	uint64_t max = 0UL;

	struct skipnode *node = x;

	memset(stats, 0, sizeof(struct stats));
	for (i = 0; i < count; i++) {
		if (node->opt == ADD) {
			int klen = strlen(node->key);
			uint64_t off = from_be64(node->val);

			real_count++;
			max_len = klen > max_len ? klen : max_len;
			min = off < min ? off : min;
			max = off > max ? off : max;
		}
		node = node->forward[0];
	}
	stats->max_len = max_len + 1;
	stats->mmap_size = (stats->max_len + sizeof(uint64_t)) * real_count ;
}

void _add_hiraishin(struct silopit *silopit, int fd, int count, int max_len)
{
	int i;
	int blk_sizes;
	struct inner_block{
		char key[max_len];
		char offset[8];
	};

	struct inner_block *blks;

	blk_sizes = count * sizeof(struct inner_block);

	blks= mmap(0, blk_sizes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (blks == MAP_FAILED) {
		__PANIC("Error:Can't mmap file when add hiraishin");
		return;
	}

	for (i = 0; i < count; i++) 
		hiraishin_add(silopit->hiraishin, blks[i].key);
	
	if (munmap(blks, blk_sizes) == -1)
		__ERROR("Error:un-mmapping the file");
}

void _silopit_load(struct silopit *silopit)
{
	int fd, result, all_count = 0;
	DIR *dd;
	struct dirent *de;

	dd = opendir(silopit->basedir);
	while ((de = readdir(dd))) {
		if (strstr(de->d_name, ".silopit")) {
			int fcount = 0, fcrc = 0;
			struct meta_node mn;
			struct footer footer;
			char silopit_file[FILE_PATH_SIZE];
			int fsize = sizeof(struct footer);

			memset(silopit_file, 0, FILE_PATH_SIZE);
			snprintf(silopit_file, FILE_PATH_SIZE, "%s/%s", silopit->basedir, de->d_name);
			
			fd = open(silopit_file, O_RDWR, 0644);
			lseek(fd, -fsize, SEEK_END);
			result = read(fd, &footer, sizeof(struct footer));
			if (result != sizeof(struct footer))
				__PANIC("read footer error");

			fcount = from_be32(footer.count);
			fcrc = from_be32(footer.crc);

			if (fcrc != F_CRC) {
				__PANIC("Crc wrong, silo file maybe broken, crc:<%d>,index<%s>", fcrc, silopit_file);
				close(fd);
				continue;
			}

			if (fcount == 0) {
				close(fd);
				continue;
			}

			_add_hiraishin(silopit, fd, fcount, from_be32(footer.max_len));

			all_count += fcount;
						
			mn.count = fcount;
			memset(mn.end, 0, SILOKATANA_MAX_KEY_SIZE);
			memcpy(mn.end, footer.key, SILOKATANA_MAX_KEY_SIZE);

			memset(mn.index_name, 0, FILE_NAME_SIZE);
			memcpy(mn.index_name, de->d_name, FILE_NAME_SIZE);
			meta_set(silopit->meta, &mn);
		
			close(fd);
		}
	}

	closedir(dd);
	__DEBUG("Load silopit,all entries count:<%d>", all_count);
}

struct silopit *silopit_new(const char *basedir)
{
	struct silopit *s;

	s = calloc(1, sizeof(struct silopit));

	s->meta = meta_new();
	memcpy(s->basedir, basedir, FILE_PATH_SIZE);

	s->hiraishin = hiraishin_new();
	s->mutexer.lsn = -1;
	pthread_mutex_init(&s->mutexer.mutex, NULL);
	s->buf= buffer_new(1024*1024*4);
	_silopit_load(s);
	return s;
}

void *_write_mmap(struct silopit *silopit, struct skipnode *x, size_t count, int need_new)
{
	int i, j, c_clone;
	int fd;
	int sizes;
	int result;
	char file[FILE_PATH_SIZE];
	struct skipnode *last;
	struct footer footer;
	struct stats stats;

	int fsize = sizeof(struct footer);
	memset(&footer, 0, fsize);

	_prepare_stats(x, count, &stats);
	sizes = stats.mmap_size;

	struct inner_block {
		char key[stats.max_len];
		char offset[8];
	};

	struct inner_block *blks;

	memset(file, 0, FILE_PATH_SIZE);
	snprintf(file, FILE_PATH_SIZE, "%s/%s", silopit->basedir, silopit->name);
	fd = open(file, O_RDWR | O_CREAT | O_TRUNC, 0644);
	if (fd == -1)
		__PANIC("error creating silopit file");

	if (lseek(fd, sizes - 1, SEEK_SET) == -1)
		__PANIC("error lseek silopit");

	result = write(fd, "", 1);
	if (result == -1)
		__PANIC("error writing empty");

	blks = mmap(0, sizes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (blks == MAP_FAILED) {
		__PANIC("error mapping block when write on process");
	}

	last = x;
	c_clone = count;
	for (i = 0, j = 0; i < c_clone; i++) {
		if (x->opt == ADD) {
			buffer_putstr(silopit->buf, x->key);
			buffer_putc(silopit->buf, 0);
			buffer_putlong(silopit->buf, x->val);
			j++;
		} else
			count--;

		last = x;
		x = x->forward[0];
	}

	char *strings = buffer_detach(silopit->buf);
	memcpy(blks, strings, sizes);

#ifdef MSYNC
	if (msync(blks, sizes, MS_SYNC) == -1) {
		__ERROR("Error Msync");
	}
#endif

	if (munmap(blks, sizes) == -1) {
		__ERROR("Un-mmapping the file");
	}
	
	footer.count = to_be32(count);
	footer.crc = to_be32(F_CRC);
	footer.size = to_be32(sizes);
	footer.max_len = to_be32(stats.max_len);
	memcpy(footer.key, last->key, strlen(last->key));

	result = write(fd, &footer, fsize);
	if (result == -1)
		__PANIC("writing the footer");

	struct meta_node mn;

	mn.count = count;
	memset(mn.end, 0, SILOKATANA_MAX_KEY_SIZE);
	memcpy(mn.end, last->key, SILOKATANA_MAX_KEY_SIZE);

	memset(mn.index_name, 0, FILE_NAME_SIZE);
	memcpy(mn.index_name, silopit->name, FILE_NAME_SIZE);
	
	if (need_new) 
		meta_set(silopit->meta, &mn);
	else 
		meta_set_byname(silopit->meta, &mn);

	close(fd);
	return x;
}

struct jikukan *_read_mmap(struct silopit *silopit, size_t count)
{
	int i;
	int fd;
	int result;
	int fcount;
	int blk_sizes;
	char file[FILE_PATH_SIZE];
	struct jikukan *merge = NULL;
	struct footer footer;
	int fsize = sizeof(struct footer);

	memset(file, 0, FILE_PATH_SIZE);
	snprintf(file, FILE_PATH_SIZE, "%s/%s", silopit->basedir, silopit->name);

	fd = open(file, O_RDWR, 0644);
	if (fd == -1)
		__PANIC("error opening silopit when read map");

	result = lseek(fd, -fsize, SEEK_END);
	if (result == -1)
		__PANIC("error lseek footer");

	result = read(fd, &footer, fsize);
	if (result != fsize) {
		__PANIC("error reading when read footer process");
	}

	struct inner_block{
		char key[from_be32(footer.max_len)];
		char offset[8];
	};

	struct inner_block *blks;

	fcount = from_be32(footer.count);
	blk_sizes = from_be32(footer.size);

	blks= mmap(0, blk_sizes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (blks == MAP_FAILED) {
		__PANIC("error map when read process");
		goto out;
	}

	merge = jikukan_new(fcount + count + 1);
	for (i = 0; i < fcount; i++) {
		jikukan_insert(merge, blks[i].key, u64_from_big((unsigned char*)blks[i].offset), ADD);
	}
	
	if (munmap(blks, blk_sizes) == -1)
		__ERROR("Un-mmapping the file");

out:
	close(fd);

	return merge;
}

uint64_t _read_offset(struct silopit *silopit, struct slice *sk)
{
	int fd;
	int fcount;
	int blk_sizes;
	int result;
	uint64_t off = 0UL;
	char file[FILE_PATH_SIZE];
	struct footer footer;
	int fsize = sizeof(struct footer);

	memset(file, 0, FILE_PATH_SIZE);
	snprintf(file, FILE_PATH_SIZE, "%s/%s", silopit->basedir, silopit->name);

	fd = open(file, O_RDWR, 0644);
	if (fd == -1) {
		__ERROR("error opening silopit when read offset process");
		return 0UL;
	}
	
	result = lseek(fd, -fsize, SEEK_END);
	if (result == -1) {
		__ERROR("error lseek when read offset process");
		close(fd);
		return off;
	}

	result = read(fd, &footer, fsize);
	if (result == -1) {
		__ERROR("error reading footer when read offset process");
		close(fd);
		return off;
	}

	int max_len = from_be32(footer.max_len);

	struct inner_block {
		char key[max_len];
		char offset[8];
	};

	struct inner_block *blks;


	fcount = from_be32(footer.count);
	blk_sizes = from_be32(footer.size);

	blks= mmap(0, blk_sizes, PROT_READ, MAP_PRIVATE, fd, 0);
	if (blks == MAP_FAILED) {
		__ERROR("Map_failed when read process");
		close(fd);
		return off;
	}

	size_t left = 0, right = fcount, i = 0;
	while (left < right) {
		i = (right -left) / 2 +left;
		int cmp = strcmp(sk->data, blks[i].key);
		if (cmp == 0) {
			off = u64_from_big((unsigned char*)blks[i].offset);	
			break ;
		}

		if (cmp < 0)
			right = i;
		else
			left = i + 1;
	}
	
	if (munmap(blks, blk_sizes) == -1)
		__ERROR("un-mmapping the file");

	close(fd);
	return off;
}

void _flush_merge_list(struct silopit *silopit, struct skipnode *x, size_t count, struct meta_node *meta)
{
	int mul;
	int rem;
	int lsn;
	int i;

	if (count <= SILOPIT_MAX_COUNT * 2) {
		if (meta) {
			lsn = meta->lsn;
			silopit->mutexer.lsn = lsn;
			pthread_mutex_lock(&silopit->mutexer.mutex);
			x = _write_mmap(silopit, x, count, 0);
			pthread_mutex_unlock(&silopit->mutexer.mutex);
			silopit->mutexer.lsn = -1;
		} else 
			x = _write_mmap(silopit, x, count, 0);
	} else {
		if (meta) {
			lsn = meta->lsn;
			silopit->mutexer.lsn = lsn;
			pthread_mutex_lock(&silopit->mutexer.mutex);
			x = _write_mmap(silopit, x, SILOPIT_MAX_COUNT, 0);
			pthread_mutex_unlock(&silopit->mutexer.mutex);
			silopit->mutexer.lsn = -1;
		} else
			x = _write_mmap(silopit, x, SILOPIT_MAX_COUNT, 0);

		mul = (count - SILOPIT_MAX_COUNT * 2) / SILOPIT_MAX_COUNT;
		rem = count % SILOPIT_MAX_COUNT;

		for (i = 0; i < mul; i++) {
			memset(silopit->name, 0, FILE_NAME_SIZE);
			snprintf(silopit->name, FILE_NAME_SIZE, "%d.silopit", silopit->meta->size); 
			x = _write_mmap(silopit, x, SILOPIT_MAX_COUNT, 1);
		}

		memset(silopit->name, 0, FILE_NAME_SIZE);
		snprintf(silopit->name, FILE_NAME_SIZE, "%d.silopit", silopit->meta->size); 

		x = _write_mmap(silopit, x, rem + SILOPIT_MAX_COUNT, 1);
	}	
}

void _flush_new_list(struct silopit *silopit, struct skipnode *x, size_t count)
{
	int mul ;
	int rem;
	int i;

	if (count <= SILOPIT_MAX_COUNT * 2) {
		memset(silopit->name, 0, FILE_NAME_SIZE);
		snprintf(silopit->name, FILE_NAME_SIZE, "%d.silopit", silopit->meta->size); 
		x = _write_mmap(silopit, x, count, 1);
	} else {
		mul = count / SILOPIT_MAX_COUNT;
		rem = count % SILOPIT_MAX_COUNT;

		for (i = 0; i < (mul - 1); i++) {
			memset(silopit->name, 0, FILE_NAME_SIZE);
			snprintf(silopit->name, FILE_NAME_SIZE, "%d.silopit", silopit->meta->size); 
			x = _write_mmap(silopit, x, SILOPIT_MAX_COUNT, 1);
		}

		memset(silopit->name, 0, FILE_NAME_SIZE);
		snprintf(silopit->name, FILE_NAME_SIZE, "%d.silopit", silopit->meta->size); 
		x = _write_mmap(silopit, x, SILOPIT_MAX_COUNT + rem, 1);
	}
}

void _flush_list(struct silopit *silopit, struct skipnode *x,struct skipnode *hdr,int flush_count)
{
	int pos = 0;
	int count = flush_count;
	struct skipnode *cur = x;
	struct skipnode *first = hdr;
	struct jikukan *merge = NULL;
	struct meta_node *meta_info = NULL;

	while(cur != first) {
		meta_info = meta_get(silopit->meta, cur->key);

		if(!meta_info){

			if(merge) {
				struct skipnode *h = merge->hdr->forward[0];
				_flush_merge_list(silopit, h, merge->count, NULL);
				jikukan_free(merge);
				merge = NULL;
			}

			_flush_new_list(silopit, x, count - pos);

			return;
		} else {
			int cmp = strcmp(silopit->name, meta_info->index_name);
			if(cmp == 0) {
				if (!merge)
					merge = _read_mmap(silopit,count);	

				jikukan_insert_node(merge, cur);
			} else {
				if (merge) {
					struct skipnode *h = merge->hdr->forward[0];

					_flush_merge_list(silopit, h, merge->count, meta_info);
					jikukan_free(merge);
					merge = NULL;
				}
				memset(silopit->name, 0, FILE_NAME_SIZE);
				memcpy(silopit->name, meta_info->index_name, FILE_NAME_SIZE);
				merge = _read_mmap(silopit, count);
				jikukan_insert_node(merge, cur);
			}

		}

		pos++;
		cur = cur->forward[0];
	}

	if (merge) {
		struct skipnode *h = merge->hdr->forward[0];
		_flush_merge_list(silopit, h, merge->count, meta_info);
		jikukan_free(merge);
	}
}

void silopit_merge(struct silopit *silopit, struct jikukan *list, int fromlog)
{
	struct skipnode *x= list->hdr->forward[0];

	if (fromlog == 1) {
		struct skipnode *cur = x;
		struct skipnode *first = list->hdr;

		__DEBUG(LEVEL_DEBUG, "adding log items to hiraishinfilter");
		while (cur != first) {
			if (cur->opt == ADD)
				hiraishin_add(silopit->hiraishin, cur->key);
			cur = cur->forward[0];
		}
	}

	if (silopit->meta->size == 0)
		 _flush_new_list(silopit, x, list->count);
	else
		_flush_list(silopit, x, list->hdr, list->count);
	jikukan_free(list);
}

uint64_t silopit_getoff(struct silopit *silopit, struct slice *sk)
{
	int lsn;
	uint64_t off = 0UL;
	struct meta_node *meta_info;

	meta_info = meta_get(silopit->meta, sk->data);
	if(!meta_info)
		return 0UL;

	memcpy(silopit->name, meta_info->index_name, FILE_NAME_SIZE);

	lsn = meta_info->lsn;
	if (silopit->mutexer.lsn == lsn) {
		pthread_mutex_lock(&silopit->mutexer.mutex);
		off = _read_offset(silopit, sk);
		pthread_mutex_unlock(&silopit->mutexer.mutex);
	} else {
		off = _read_offset(silopit, sk);
	}
	return off;
}

void silopit_free(struct silopit *silopit)
{
	if (silopit) {
		meta_free(silopit->meta);
		hiraishin_free(silopit->hiraishin);
		buffer_free(silopit->buf);
		free(silopit);
	}
}