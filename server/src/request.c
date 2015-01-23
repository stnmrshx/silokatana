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
#include <ctype.h>
#include "../include/request.h"
#include "../../storage/include/debug.h"

unsigned char _table[256]={
	0,       0,       0,       0,       0,       0,       0,       0,
	0,       0,       3,       0,       0,       3,       0,       0,
	0,       0,       0,       0,       0,       0,       0,       0,
	0,       0,       0,       0,       0,       0,       0,       0,
	0,       0,       0,       0,       3,       0,       0,       0,
	0,       0,       3,       0,       0,       0,       0,       0,
	2,       2,       2,       2,       2,       2,       2,       2,
	2,       2,       0,       0,       0,       0,       0,       0,
	0,       0,       0,       0,       0,       1,       0,       1,
	0,       0,       0,       0,       0,       0,       0,       0,
	0,       0,       0,       0,       1,       0,       0,       0,
	0,       0,       0,       0,       0,       0,       0,       0,
	0,       0,       0,       0,       0,       0,       0,       0,
	0,       0,       0,       0,       0,       0,       0,       0,
	0,       0,       0,       0,       0,       0,       0,       0,
	0,       0,       0,       0,       0,       0,       0,       0 
};

#define BUF_SIZE (1024*10)

static const struct cmds _cmds[]=
{
	{"ping", CMD_PING},
	{"get", CMD_GET},
	{"mget", CMD_MGET},
	{"set", CMD_SET},
	{"mset", CMD_MSET},
	{"del", CMD_DEL},
	{"info", CMD_INFO},
	{"exists", CMD_EXISTS},
	{"shutdown", CMD_SHUTDOWN},
	{"unknow cmd", CMD_UNKNOW}
};

enum {
	STATE_CONTINUE,
	STATE_FAIL
};

struct request *request_new()
{
	struct request *req;

	req = calloc(1, sizeof(struct request));
	memset(req->querybuf, 0, REQ_MAX_BUFFSIZE);
	req->pos = 0;
	req->multilen = 0;
	req->lastlen = 0;
	req->len = 0;
	req->idx = 0;

	return req;
}

int req_state_len(struct request *req, char *sb)
{
	int term = 0, first = 0;
	char c;
	int i = req->pos;
	int pos = i;

	while ((c = req->querybuf[i]) != '\0') {
		first++;
		pos++;
		switch (c) {
			case '\r':
				term = 1;
				break;

			case '\n':
				if (term) {
					req->pos = pos;
					return STATE_CONTINUE;
				} else
					return STATE_FAIL;
			default:
				if (first == 1) {
					if (_table[(unsigned char)c] != 3)
						return STATE_FAIL;
				} else {
					if (_table[(unsigned char)c] == 2) {
						*sb = c;
						sb++;
					} else
						return STATE_FAIL;
				}
				break;
		}
		i++;
	}

	return STATE_FAIL;
}

void _reset_request(struct request *req) 
{
	req->multilen = 0;
	req->lastlen = 0;
	req->pos = 0;
	req->idx = 0;
	req->len = 0;
}

int request_append(struct request *req, const char *buf, int n)
{
	if ((req->len + n) > REQ_MAX_BUFFSIZE){
		__ERROR("req->len #%d", req->len);
		return -1;
	}

	memcpy(req->querybuf + req->len, buf, n); 
	req->len += n;

	return 1;
}

int request_parse(struct request *req)
{
	int l;
	char sb[BUF_SIZE] = {0};

	if (req->multilen == 0) {
		if (req_state_len(req, sb) != STATE_CONTINUE) {
			__ERROR("argc error,buffer:%s", req->querybuf);
			return -1;
		}
		req->argc = atoi(sb);
		req->multilen = req->argc;

		req->argv = (struct term**)calloc(req->argc, sizeof(struct term*));
		int i;

		for (i = 0; i < req->argc; i++) {
			struct term *t = calloc(1, sizeof(struct term));
			req->argv[i] = t;
		}
	}

	while (req->multilen) {
		int argv_len;
		char *v;

		if (req->lastlen == 0) {
			memset(sb, 0, BUF_SIZE);
			if (req_state_len(req, sb) != STATE_CONTINUE) {
				__ERROR("argv's len error,packet:%s\n", sb);
				return -1;
			}
			argv_len = atoi(sb);
		} else {
			argv_len = req->lastlen;
		}

		if ((req->pos + argv_len) < req->len) {
			v = (char*)calloc(argv_len + 1, sizeof(char));
			memcpy(v, req->querybuf + req->pos, argv_len);

			if (req->idx == 0) {

				int k, cmd_size;
				for (k = 0; k < argv_len; k++) {
					if (v[k] >= 'A' && v[k] <= 'Z')
						v[k] += 32;
				}

				cmd_size = sizeof(_cmds) / sizeof(struct cmds);
				req->cmd = CMD_UNKNOW;
				for (l = 0; l < cmd_size; l++) {
					if (strcmp(v, _cmds[l].method) == 0) {
						req->cmd = _cmds[l].cmd;
						break;
					}
				}
			}

			req->argv[req->idx]->len = argv_len;
			req->argv[req->idx++]->data = v;	
			req->pos += (argv_len + 2);

			req->lastlen = 0;
			req->multilen--;
		} else {
			req->lastlen = argv_len;
			return 0;
		}
	}

	if(req->pos < req->len) {
		char bufclone[REQ_MAX_BUFFSIZE];
		int buflen = req->len - req->pos;

		memset(bufclone, 0, REQ_MAX_BUFFSIZE);
		memcpy(bufclone, req->querybuf + req->pos, buflen);

		memset(req->querybuf, 0, REQ_MAX_BUFFSIZE);
		memcpy(req->querybuf, bufclone, buflen);
		req->len = buflen;
		req->pos = 0;
	} else {
		memset(req->querybuf, 0, REQ_MAX_BUFFSIZE);
		_reset_request(req);
	}

	return 1;
}

void request_dump(struct request *req)
{
	int i;

	if (!req)
		return;

	printf("request-dump--->{");
	printf("argc:<%d>\n", req->argc);
	printf("\t\tcmd:<%s>\n", _cmds[req->cmd].method);

	for (i = 0; i < req->argc; i++) {
		printf("\t\targv[%d]:<%s>\n", i, req->argv[i]->data);
	}

	printf("}\n\n");
}

void request_clean(struct request *req)
{
	if (req->argv)
		free(req->argv);
}

void request_free_value(struct request *req)
{
	int i;
	if (req) {
		for (i = 0; i < req->argc; i++) {
			if (req->argv[i]) {
				if (req->argv[i]->data)
					free(req->argv[i]->data);
				free(req->argv[i]);
			}
		}
	}
}

void request_free(struct request *req)
{
	if (req) {
		free(req);
	}
}