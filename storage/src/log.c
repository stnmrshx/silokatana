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
#include <dirent.h>

#include "../include/buffer.h"
#include "../include/log.h"
#include "../include/debug.h"

#define DB_MAGIC (2013)

int _file_exists(const char *path)
{
	int fd = n_open(path, O_RDWR);

	if (fd > -1) {
		close(fd);
		return 1;
	}
	return 0;
}

struct log *log_new(const char *basedir, int lsn, int islog)
{
	int result;
	struct log *l;
	char log_name[FILE_PATH_SIZE];
	char db_name[FILE_PATH_SIZE];
	(void)lsn;

	l = calloc(1, sizeof(struct log));
	l->islog = islog;

	memset(log_name, 0, FILE_PATH_SIZE);
	snprintf(log_name, FILE_PATH_SIZE, "%s/%d.log", basedir, 0);
	memcpy(l->name, log_name, FILE_PATH_SIZE);

	memset(l->basedir, 0, FILE_PATH_SIZE);
	memcpy(l->basedir, basedir, FILE_PATH_SIZE);

	memset(db_name, 0, FILE_PATH_SIZE);
	snprintf(db_name, FILE_PATH_SIZE, "%s/%s", basedir, DB_NAME);

	if (_file_exists(db_name)) {
		l->db_wfd = n_open(db_name, LSM_OPEN_FLAGS, 0644);
		if (l->db_wfd == -1)
			__PANIC("erro opening db");

		l->db_alloc = n_lseek(l->db_wfd, 0, SEEK_END);
	} else {
		int magic = DB_MAGIC;

		l->db_wfd = n_open(db_name, LSM_CREAT_FLAGS, 0644);
		if (l->db_wfd == -1)
			__PANIC("error creating db");

		result = write(l->db_wfd, &magic, sizeof(int));
		if (result == -1) 
			__PANIC("error write magic");

		l->db_alloc = sizeof(int);
	}
	l->buf = buffer_new(LOG_BUFFER_SIZE);
	l->db_buf = buffer_new(LOG_BUFFER_SIZE);
	return l;
}

int _log_read(char *logname, struct jikukan *list)
{
	int rem, count = 0, del_count = 0;
	int fd, size;

	fd = open(logname, O_RDWR, 0644);

	if (fd == -1) {
		__ERROR("error opening log when log read, file:<%s>", logname);
		return 0;
	}
	
	size = lseek(fd, 0, SEEK_END);
	if (size == 0) {
		__ERROR("error seeking end when log read,file:<%s>", logname);
		return 0;
	}

	rem = size;

	if (lseek(fd, 0, SEEK_SET) == -1) {
		__ERROR("seek begin when log read,file:<%s>", logname);
		return 0;
	}

	while(rem > 0) {
		int isize = 0;
		int klen;
		uint64_t off;
		char key[SILOKATANA_MAX_KEY_SIZE];
		char klenstr[4], offstr[8], optstr[4];

		memset(klenstr, 0, 4);
		memset(offstr, 0, 8);
		memset(optstr, 0, 4);
		
		if (read(fd, &klenstr, sizeof(int)) != sizeof(int)) {
			__PANIC("error when read key length");
			return -1;
		}

		klen = u32_from_big((unsigned char*)klenstr);
		isize += sizeof(int);
		
		memset(key, 0, SILOKATANA_MAX_KEY_SIZE);
		if (read(fd, &key, klen) != klen) {
			__PANIC("error when read key");
			return -1;
		}

		isize += klen;

		if (read(fd, &offstr, sizeof(uint64_t)) != sizeof(uint64_t)) {
			__PANIC("read error when read data offset");
			return -1;
		}

		off = u64_from_big((unsigned char*)offstr);
		isize += sizeof(uint64_t);

		if (read(fd, &optstr, 1) != 1) {
			__PANIC("read error when read opteration");
			return -1;
		}

		isize += 1;
		if (memcmp(optstr, "A", 1) == 0) {
			count++;
			jikukan_insert(list, key, off, ADD);
		} else {
			del_count++;
			jikukan_insert(list, key, off, DEL);
		}
		rem -= isize;
	}

	__DEBUG("recovery process count ADD#%d, DEL#%d", count, del_count);
	return 1;
}

int log_recovery(struct log *l, struct jikukan *list)
{
	DIR *dd;
	int ret = 0;
	int flag = 0;
	char new_log[FILE_PATH_SIZE];
	char old_log[FILE_PATH_SIZE];
	struct dirent *de;

	if (!l->islog)
		return 0;

	memset(new_log, 0, FILE_PATH_SIZE);
	memset(old_log, 0, FILE_PATH_SIZE);

	dd = opendir(l->basedir);
	while ((de = readdir(dd))) {
		char *p = strstr(de->d_name, ".log");
		if (p) {
			if (flag == 0) {
				memcpy(new_log, de->d_name, FILE_PATH_SIZE);
				flag |= 0x01;
			} else {
				memcpy(old_log, de->d_name, FILE_PATH_SIZE);
				flag |= 0x10;
			}
		}
	}
	closedir(dd);

	if ((flag & 0x01) == 0x01) {
		memset(l->log_new, 0, FILE_PATH_SIZE);
		snprintf(l->log_new, FILE_PATH_SIZE, "%s/%s", l->basedir, new_log);
		__DEBUG("prepare to recover from new log : %s", l->log_new);
		ret = _log_read(l->log_new, list);
		if (ret == 0)
			return ret;
	}

	if ((flag & 0x10) == 0x10) {
		memset(l->log_old, 0, FILE_PATH_SIZE);
		snprintf(l->log_old, FILE_PATH_SIZE, "%s/%s", l->basedir, old_log);
		__DEBUG("prepare to recover from old log : %s", l->log_old);
		ret = _log_read(l->log_old, list);
		if (ret == 0)
			return ret;
	}
	return ret;
}

uint64_t log_append(struct log *l, struct slice *sk, struct slice *sv)
{
	int len;
	int db_len;
	char *line;
	char *db_line;
	struct buffer *buf = l->buf;
	struct buffer *db_buf = l->db_buf;
	uint64_t db_offset = l->db_alloc;

	if (sv) {
		buffer_putint(db_buf, sv->len);
		buffer_putnstr(db_buf, sv->data, sv->len);
		db_len = db_buf->NUL;
		db_line = buffer_detach(db_buf);

		if (write(l->db_wfd, db_line, db_len) != db_len) {
			__PANIC("error on aof value when writing, length : <%d>", db_len);
			return db_offset;
		}
		l->db_alloc += db_len;
	}

	if (l->islog) {
		buffer_putint(buf, sk->len);
		buffer_putnstr(buf, sk->data, sk->len);
		buffer_putlong(buf, db_offset);

		if(sv)
			buffer_putnstr(buf, "A", 1);
		else
			buffer_putnstr(buf, "D", 1);

		len = buf->NUL;
		line = buffer_detach(buf);

		if (write(l->indx_wfd, line, len) != len)
			__PANIC("error logging aof, buffer is : %s, buffer length : <%d>", line, len);
	}
	return db_offset;
}

void log_remove(struct log *l, int lsn)
{
	char log_name[FILE_PATH_SIZE];

	memset(log_name, 0 ,FILE_PATH_SIZE);
	snprintf(log_name, FILE_PATH_SIZE, "%s/%d.log", l->basedir, lsn);
	remove(log_name);
}

void log_next(struct log *l, int lsn)
{
	char log_name[FILE_PATH_SIZE];
	memset(log_name, 0 ,FILE_PATH_SIZE);
	snprintf(log_name, FILE_PATH_SIZE, "%s/%d.log", l->basedir, lsn);
	memcpy(l->name, log_name, FILE_PATH_SIZE);

	buffer_clear(l->buf);
	buffer_clear(l->db_buf);

	close(l->indx_wfd);
	l->indx_wfd = open(l->name, LSM_CREAT_FLAGS, 0644);

	if (l->indx_wfd == -1)
		__PANIC("error creating new log");
}

void log_free(struct log *l)
{
	if (l) {
		buffer_free(l->buf);
		buffer_free(l->db_buf);
		close(l->indx_wfd);
		free(l);
	}
}